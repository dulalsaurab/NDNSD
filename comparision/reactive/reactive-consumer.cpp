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

NDN_LOG_INIT(ndnsd.comparision.reactive_consumer);

class ReactiveConsumer
{
public:
  ReactiveConsumer(const ndn::Name& discoveryPrefix, const std::string& serviceType)
  : m_discoveryPrefix(discoveryPrefix)
  , m_serviceType(serviceType)
  {
    ndn::Interest i1(m_discoveryPrefix);
    
    expressInterest(i1);
    m_face.processEvents();
  }

  void
  expressInterest(ndn::Interest& interest);

  // we dont expect any data on notification interest
  void
  onData(const ndn::Interest& interest, const ndn::Data& data);
  
  void
  onTimeout(const ndn::Interest& interest)
  {
    NDN_LOG_INFO("Interest: " << interest.getName() << " time out"); 
  }

private:
  ndn::Face m_face;
  ndn::Name m_discoveryPrefix;
  std::string m_serviceType;
  std::vector<std::string> m_receivedServices;

};

void
ReactiveConsumer::expressInterest(ndn::Interest& interest)
{
  
  std::string names = "|";
  for (auto item : m_receivedServices)
    names = names +  item + "|";

  const std::string serviceNames = names;
  NDN_LOG_INFO("Excluded names: " << serviceNames);
  interest.setApplicationParameters(reinterpret_cast<const uint8_t*>(serviceNames.c_str()), serviceNames.size());
  
  // interest.setCanBePrefix(true);
  // interest.setMustBeFresh(true);
 
  NDN_LOG_INFO("Sending interest: "<< interest);
  m_face.expressInterest(interest,
                          ndn::bind(&ReactiveConsumer::onData, this, _1, _2),
                          ndn::bind(&ReactiveConsumer::onTimeout, this, _1),
                          ndn::bind(&ReactiveConsumer::onTimeout, this, _1));
}

void
ReactiveConsumer::onData(const ndn::Interest& interest, const ndn::Data& data)
{
  NDN_LOG_INFO("Recevied data for interest: " << interest.getName());
  // process data content, and see the service name
}

int main()
{
  ReactiveConsumer rconsumer ("/uofm/discovery/printer", "printer");
}
