#include<iostream>
#include "ndnsd/discovery/service-discovery.hpp"
#include <list>

class Producer 
{
public:
  Producer(const ndn::Name& serviceName, const std::string& userPrefix,
           const std::string &serviceInfo,
           const std::map<char, std::string>& pFlags)
  
  : m_serviceDiscovery(serviceName, userPrefix, pFlags, serviceInfo,
                      ndn::time::system_clock::now(), 10_ms,
                      std::bind(&Producer::processCallback, this, _1))
  {
  }

  void 
  execute ()
  {
    m_serviceDiscovery.producerHandler();
  }

private:
  void
  processCallback(const std::string& callback)
  {
      // std::cout << callback << std::endl;
  }

  ndnsd::discovery::ServiceDiscovery m_serviceDiscovery;

};


int
main(int argc, char* argv[])
{

  std::cout << ndn::time::system_clock::now()<< std::endl;

  if (argc != 4) {
    std::cout << "usage: " << argv[0] << " <service-name> <user-prefix> "
              << " <service-info>"
              << std::endl;
    return 1;
  }

  std::map<char, std::string> flags;
  flags.insert(std::pair<char, std::string>('p', "sync"));
  flags.insert(std::pair<char, std::string>('t', "p"));

  try {
    Producer producer(argv[1], argv[2], argv[3], flags);
    producer.execute();
  }
  catch (const std::exception& e) {

    // NDN_LOG_ERROR(e.what());
  }
}