#include<iostream>
#include "ndnsd/discovery/service-discovery.hpp"
#include <list>

class Consumer 
{
public:
    Consumer(const ndn::Name& serviceName, const std::map<char, std::string>& pFlags)
    
    : m_serviceDiscovery(serviceName, pFlags, ndn::time::system_clock::now())
    {
    }

void 
execute ()
{
  m_serviceDiscovery.consumerHandler();
}

private:
    ndnsd::discovery::ServiceDiscovery m_serviceDiscovery;

};


int
main(int argc, char* argv[])
{
  // if (argc != 4) {
  //   std::cout << "usage: " << argv[0] << " <service-name> <user-prefix> "
  //             << " <service-info>"
  //             << std::endl;
  //   return 1;
  // }

  // std::map<char, std::string> flags;
  // flags.insert(std::pair<char, std::string>('p', "sync"));

  // try {
  //   Producer producer(argv[1], argv[2], argv[3], flags);
  //   producer.execute();
  // }
  // catch (const std::exception& e) {

  //   // NDN_LOG_ERROR(e.what());
  // }
}