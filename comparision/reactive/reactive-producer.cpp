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

NDN_LOG_INIT(ndnsd.comparision.reactive_prod);

int MAX_UPDATES = 10;

class ReactiveDiscovery
{
  // servicePrefix is applicationPrefix
public:
  ReactiveDiscovery(const ndn::Name& servicePrefix,  const std::string& serviceType,
                                  ndn::time::milliseconds publicationInterval)
  : m_scheduler(m_face.getIoService())
  , m_servicePrefix(servicePrefix) //should be routable
  , m_serviceType(serviceType)
  , m_discoveryPrefix("/uofm/discovery/printer")
  , m_currentUpdateCounter(0)
  , m_periodicInterval(publicationInterval)
  {
    setInterestFilter(m_discoveryPrefix);
    std::this_thread::sleep_for (std::chrono::seconds(1));

    // schedule service updated, service is updated in every 1s
    updateService(); 

    m_face.processEvents();
  }

  void
  updateService();

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
  int m_currentUpdateCounter;
  ndn::time::milliseconds m_periodicInterval;
  ndn::scheduler::ScopedEventId m_scheduledSyncInterestId;
};

void
ReactiveDiscovery::updateService()
{
  ++m_currentUpdateCounter;
  if(m_currentUpdateCounter >= MAX_UPDATES)
  {
    NDN_LOG_INFO("Reached maximum number of updated, exiting");
    exit(0);
  }

  m_scheduledSyncInterestId = m_scheduler.schedule(m_periodicInterval,
                         [this] { updateService(); });

}

void
ReactiveDiscovery::setInterestFilter(ndn::Name name)
{
  // service prefix should be routable in the network
  ndn::Interest interest(name);
  NDN_LOG_INFO("Setting interest filter on: " << name);
  m_face.setInterestFilter(ndn::InterestFilter(name).allowLoopback(false),
                           std::bind(&ReactiveDiscovery::processInterest, this, _1, _2),
                           std::bind(&ReactiveDiscovery::onRegistrationSuccess, this, _1),
                           std::bind(&ReactiveDiscovery::OnRegistrationFailed, this, _1));
}

void
ReactiveDiscovery::processInterest(const ndn::Name& name, const ndn::Interest& interest)
{
  bool send = true;
  NDN_LOG_INFO("Received interest: " << name);
  // check if service name is excluded in the application parameter
  auto params = interest.getApplicationParameters();
  NDN_LOG_INFO("app params: " << params);
  
  std::string s(reinterpret_cast<const char*>(params.value()));

  NDN_LOG_INFO("Excluded names: " << s);
  std::string delimiter = "|";
  size_t pos = 0;
  std::string token;

  while ((pos = s.find(delimiter)) != std::string::npos) {
      token = s.substr(0, pos);
      if (token == m_servicePrefix)
          send = false;
        // name is excluded, we have already send reply for this, so dont send anything
      s.erase(0, pos + delimiter.length());
  }
  // finally check one more time
  if (s == m_servicePrefix)
    send = false;

  if (send)
  {
    // send service-info data for the interest received
    NDN_LOG_INFO("Sending data for: " << name);
    ndn::Data replyData(interest.getName());
    ndn::time::milliseconds freshnessPeriod(2);
    replyData.setFreshnessPeriod(freshnessPeriod);
    // attached current update counter so that consumer will know if all the publication is fetched or not
    const std::string c =  m_servicePrefix.toUri() + "|" + std::to_string(m_currentUpdateCounter) + "|";
    replyData.setContent(reinterpret_cast<const uint8_t*>(c.c_str()), c.size());
    m_keychain.sign(replyData); 
    m_face.put(replyData);
    NDN_LOG_INFO("Data sent for :" << name);
  } 
}

void
ReactiveDiscovery::OnRegistrationFailed(const ndn::Name& name)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix " << name << " in local hub's daemon");
}

void
ReactiveDiscovery::onRegistrationSuccess(const ndn::Name& name)
{
  NDN_LOG_DEBUG("Successfully registered prefix: " << name);
}

int main()
{
  ndn::time::milliseconds interval(1000);
  ReactiveDiscovery rdiscovery("/uofm/printer1/", "printer", interval);
}