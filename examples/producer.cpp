#include<iostream>
#include "src/discovery/service-discovery.hpp"
#include <list>

class Producer 
{
public:
    Producer(const ndn::Name& serviceName, const std::string& userPrefix,
             const std::string &serviceInfo,
             const std::list<std::string>& pFlags)
    
    : m_serviceDiscovery(serviceName, userPrefix, pFlags, serviceInfo,
                        ndn::time::system_clock::now(), 10_s)
    {
    }

private:
    ndnsd::discovery::ServiceDiscovery m_serviceDiscovery;
    // std::list<std::string> m_flags;

};


int
main(int argc, char* argv[])
{
  if (argc != 4) {
    std::cout << "usage: " << argv[0] << " <service-name> <user-prefix> "
              << " <service-info>"
              << std::endl;
    return 1;
  }

  std::list<std::string> flags;
  flags.push_back("psync");

  try {
    std::cout << argv[1] << ":" << argv[2] << ":" <<  argv[3] << std::endl;
    Producer producer(argv[1], argv[2], argv[3], flags);
    // producer.run();
  }
  catch (const std::exception& e) {

    // NDN_LOG_ERROR(e.what());
  }
}