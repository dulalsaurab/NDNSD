#include <iostream>

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

NDN_LOG_INIT(ndnsd.comparision.proactive_prod);

int MAX_UPDATES = 1;

class ProactiveDiscovery
{
  // servicePrefix is applicationPrefix
public:
  ProactiveDiscovery(const ndn::Name& servicePrefix,  const std::string& serviceType,
                                   ndn::time::milliseconds publicationInterval)
  : m_scheduler(m_face.getIoService())
  , m_servicePrefix(servicePrefix) //should be routable
  , m_serviceType(serviceType)
  , m_discoveryPrefix("/uofm/discovery/printer")
  , m_periodicInterval(publicationInterval)
  , m_currentUpdateCounter(0)
  {
    /* 
      e.g. m_servicePrefic = /uofm/printer1
      serviceInfoPrefix = /uofm/printer1/service-info
    */
    auto serviceName = m_servicePrefix;
    // serviceName.append("service-info");
    setInterestFilter(serviceName);
    std::this_thread::sleep_for (std::chrono::seconds(1));
    /* send notification interest under /uofm/printers/discovery
       /<domain>/<other-info>/discovery/<service-type>, second-last component is always service type
      application parameter: servicePrefix name
    */
    // send periodic notification 500ms each
    m_scheduler.schedule(m_periodicInterval/2,
                         [this] { sendNotificationInterest();});

    // sendNotificationInterest();

    // publish updates, we will publish 300 updates 1 second each
    m_scheduler.schedule(m_periodicInterval,
                         [this] { publishUpdates();});
    // publishUpdates();

    m_face.processEvents();
  }

  void
  sendNotificationInterest();

  void publishUpdates();

  void
  expressInterest(ndn::Interest& interest);

  // we dont expect any data on notification interest
  void
  onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    NDN_LOG_INFO("Surprising, we receive data for interest: " << interest.getName());
  }

  void
  onTimeout(const ndn::Interest& interest)
  {
    NDN_LOG_INFO("Interest: " << interest.getName() << " time out"); 
  }

  // interest handeling
  void
  processInterest(const ndn::Name& name, const ndn::Interest& interest);

  void
  onRegistrationSuccess(const ndn::Name& name);

  void
  OnRegistrationFailed(const ndn::Name& name);

  // interest filter basics
  void
  setInterestFilter(const ndn::Name name);
 
private: 
  ndn::Face m_face;
  ndn::security::KeyChain m_keychain;
  ndn::Scheduler m_scheduler;

  ndn::Name m_servicePrefix;
  std::string m_serviceType;
  ndn::Name m_discoveryPrefix;

  ndn::time::milliseconds m_periodicInterval;
  ndn::scheduler::ScopedEventId m_scheduledSyncInterestId;
  int m_currentUpdateCounter;
};

void
ProactiveDiscovery::sendNotificationInterest()
{
  auto name = m_discoveryPrefix;
  name.append(m_servicePrefix).appendNumber(m_currentUpdateCounter);

  ndn::Interest interest(name);
  
  // const std::string serviceInfoName =  m_servicePrefix.toUri() + "/service-info/"; // -- is delimiter, dirty way of avoiding encoding procedure :(
  // interest.setApplicationParameters(reinterpret_cast<const uint8_t*>(serviceInfoName.c_str()), serviceInfoName.size());

  NDN_LOG_INFO("Sending periodic notification: " << name << " Current update counter: " << m_currentUpdateCounter);
  expressInterest(interest);
  NDN_LOG_INFO("Notification interest: "  << name << " sent");
  // schedule another notification interest
  NDN_LOG_INFO("Next notification interest is scheduled on: " << m_periodicInterval);

  m_scheduledSyncInterestId = m_scheduler.schedule(m_periodicInterval/2,
                         [this] { sendNotificationInterest();});
}

void
ProactiveDiscovery::publishUpdates()
{
  // if notification is scheduled, cancle it
  try {
    m_scheduledSyncInterestId.cancel();
  }
  catch (const  std::exception& e) {
    NDN_LOG_ERROR("Encountered some error, " << e.what());
  }

  // the service will be updated MAX_UPDATES times  
  ++m_currentUpdateCounter; 
  if (m_currentUpdateCounter >= MAX_UPDATES)
  {
    NDN_LOG_INFO("Reached maximum number of updated, exiting");
    exit(0);   
  }
  auto name = m_discoveryPrefix;
  name.append(m_servicePrefix).appendNumber(m_currentUpdateCounter);

  ndn::Interest interest(name);
  
  // const std::string serviceInfoName =  m_servicePrefix.toUri() + "/service-info/";  // -- is delimiter, dirty way of avoiding encoding procedure :(
  // interest.setApplicationParameters(reinterpret_cast<const uint8_t*>(serviceInfoName.c_str()), serviceInfoName.size());

  NDN_LOG_INFO("Sending new publication interest: " << name << " Current update counter: " << m_currentUpdateCounter);
  expressInterest(interest);
  NDN_LOG_INFO("Publish interest: "  << name << " sent");
  // schedule another interest
  NDN_LOG_INFO("Next publication is scheduled on: " << m_periodicInterval);

  auto scheduledId = m_scheduler.schedule(m_periodicInterval,
                                  [this] {publishUpdates();});

  // schedule notification now
  m_scheduledSyncInterestId = m_scheduler.schedule(m_periodicInterval/2,
                         [this] { sendNotificationInterest();}); 
 
}

void
ProactiveDiscovery::expressInterest(ndn::Interest& interest)
{
  // ndn::Interest interest(interestName);
  interest.setCanBePrefix(true);
  // interest.setMustBeFresh(true);
  
  NDN_LOG_INFO("Sending interest: "<< interest);
  m_face.expressInterest(interest,
                          ndn::bind(&ProactiveDiscovery::onData, this, _1, _2),
                          ndn::bind(&ProactiveDiscovery::onTimeout, this, _1),
                          ndn::bind(&ProactiveDiscovery::onTimeout, this, _1));
}

void
ProactiveDiscovery::setInterestFilter(ndn::Name name)
{
  // service prefix should be routable in the network
  ndn::Interest interest(name);
  NDN_LOG_INFO("Setting interest filter on: " << name);
  m_face.setInterestFilter(ndn::InterestFilter(name).allowLoopback(false),
                           std::bind(&ProactiveDiscovery::processInterest, this, _1, _2),
                           std::bind(&ProactiveDiscovery::onRegistrationSuccess, this, _1),
                           std::bind(&ProactiveDiscovery::OnRegistrationFailed, this, _1));
}

void
ProactiveDiscovery::processInterest(const ndn::Name& name, const ndn::Interest& interest)
{
  NDN_LOG_INFO("Received interest: " << interest.getName());

  // send service-info data for the interest received
  NDN_LOG_INFO("Sending data for: " << name);
  // ndn::Data replyData(interest.getName());
  ndn::Data replyData(interest.getName());

  const std::string c =  "update" + std::to_string(m_currentUpdateCounter);
  
  ndn::time::milliseconds freshnessPeriod(100);
  replyData.setFreshnessPeriod(freshnessPeriod);

  replyData.setContent(reinterpret_cast<const uint8_t*>(c.c_str()), c.size());
  m_keychain.sign(replyData);
  m_face.put(replyData);
  NDN_LOG_INFO("Data sent for :" << name);
}

void
ProactiveDiscovery::OnRegistrationFailed(const ndn::Name& name)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix " << name << " in local hub's daemon");
}

void
ProactiveDiscovery::onRegistrationSuccess(const ndn::Name& name)
{
  NDN_LOG_DEBUG("Successfully registered prefix: " << name);
}


int main(int argc, char* argv[])
{
  std::string serviceInfoPrefix = argv[1];
  ndn::time::milliseconds interval(1000);
  ProactiveDiscovery pdiscovery(serviceInfoPrefix, "printer", interval);
}