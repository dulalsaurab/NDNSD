#include <iostream>
#include <algorithm>

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

NDN_LOG_INIT(ndnsd.comparision.reactive_consumer);

class ReactiveConsumer
{
public:
  ReactiveConsumer(const ndn::Name& discoveryPrefix, const std::string& serviceType,
                   ndn::time::milliseconds periodicInterval)
  : m_scheduler(m_face.getIoService())
  , m_discoveryPrefix(discoveryPrefix)
  , m_serviceType(serviceType)
  , m_periodicInterval(periodicInterval)
  {
    // std::this_thread::sleep_for (std::chrono::seconds(55));
    expressInterest(makeInterest(m_discoveryPrefix));
    // schedule periodic interest for each second
    // total number of publicaiton 30, each at 10s. so lets run the loop for 350 sec
    std::vector <ndn::scheduler::ScopedEventId> scheduledInterest;
    for (int i = 1; i < 360; i++)
    {
      NDN_LOG_INFO("periodic interest: " << i << " scheduled at: " << i*m_periodicInterval);
      auto sendId = m_scheduler.schedule(i*m_periodicInterval,
                                  [this] {
                                    NDN_LOG_INFO("Sending new periodic interest");
                                    expressInterest(makeInterest(m_discoveryPrefix));
                                    });

      scheduledInterest.push_back(sendId); //not sure if we need this
    }
    m_face.processEvents();
  }

  ndn::Interest&
  makeInterest(ndn::Name& name)
  {
    std::string names = "|";
    for (auto item : m_receivedServices) {
      name.append(item);
      names = names +  item + "|"; //not used except for printing
    }

    const std::string serviceNames = names;
    NDN_LOG_INFO("Excluded names: " << serviceNames);
    // interest.setApplicationParameters(reinterpret_cast<const uint8_t*>(serviceNames.c_str()), serviceNames.size());
    // interest.refreshNonce();
    ndn::Interest i1(name);
    i1.setCanBePrefix(true);
    m_discoveryInterest = i1;
    return m_discoveryInterest;
  }

  void
  expressInterest(ndn::Interest& interest);

  // we dont expect any data on notification interest
  void
  onData(const ndn::Interest& interest, const ndn::Data& data);
  
  void
  onTimeout(const ndn::Interest& interest)
  {
    // under stable network condition, after all the services are fetched we will receive timeout
    NDN_LOG_INFO("Interest: " << interest.getName() << " time out");


    // NDN_LOG_INFO("New periodic interest will be scheduled in: " << m_periodicInterval);
    m_receivedServices.clear();
    // m_scheduledSyncInterestId.cancel();
    // m_scheduledSyncInterestId = m_scheduler.schedule(m_periodicInterval,
    //                               [this] {expressInterest(makeInterest(m_discoveryPrefix));});

  }

private:
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  ndn::Name m_discoveryPrefix;
  std::string m_serviceType;
  std::vector<std::string> m_receivedServices;

  ndn::time::milliseconds m_periodicInterval;
  ndn::scheduler::ScopedEventId m_scheduledSyncInterestId;
  ndn::Interest m_discoveryInterest;
};

void
ReactiveConsumer::expressInterest(ndn::Interest& interest)
{
  interest.setMustBeFresh(true);
  NDN_LOG_INFO("Sending interest: "<< interest);
  m_face.expressInterest(interest,
                          std::bind(&ReactiveConsumer::onData, this, _1, _2),
                          std::bind(&ReactiveConsumer::onTimeout, this, _1),
                          std::bind(&ReactiveConsumer::onTimeout, this, _1));
  
  ndn::Name temp("/uofm/discovery/printer");
  m_discoveryPrefix = temp; //reset prefix
}

void
ReactiveConsumer::onData(const ndn::Interest& interest, const ndn::Data& data)
{
  NDN_LOG_INFO("Recevied data for interest: " << interest.getName());

  std::string _params(reinterpret_cast<const char*>(data.getContent().value()));
  NDN_LOG_INFO("Data content: " << _params);
  std::string delimiter = "|";
  size_t pos = 0;
  std::string token;
  pos = _params.find(delimiter);
  token = _params.substr(0, pos);
  NDN_LOG_INFO("Producer prefix: " << token);
  // insert the name into the received vector if not present already

  if (!(std::find(m_receivedServices.begin(), m_receivedServices.end(), token) != m_receivedServices.end()))
    m_receivedServices.push_back(token);
  // send another interest
  expressInterest(makeInterest(m_discoveryPrefix));

}

int main()
{
  ndn::time::milliseconds periodicInterval(1000);
  ReactiveConsumer rconsumer ("/uofm/discovery/printer", "printer", periodicInterval);
}
