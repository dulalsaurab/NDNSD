#include<iostream>
#include "ndnsd/discovery/service-discovery.hpp"
#include <list>

class Consumer 
{
public:
  Consumer(const ndn::Name& serviceName, const std::map<char, uint8_t>& pFlags)
  
  : m_serviceDiscovery(serviceName, pFlags, ndn::time::system_clock::now(),
                       std::bind(&Consumer::processCallback, this, _1))
  {
  }

  void 
  execute ()
  {
    m_serviceDiscovery.consumerHandler();
  }

private:

  void
  processCallback(const ndnsd::discovery::Details& callback)
  {
      auto abc = (callback.status == ndnsd::discovery::ACTIVE)? "ACTIVE": "EXPIRED";
      std::cout << "Name: " << callback.serviceName << "\n"
                << "Status: " << abc << "\n"
                << "Info: " << callback.serviceInfo << "\n" << std::endl;
  }

private:
  ndnsd::discovery::ServiceDiscovery m_serviceDiscovery;

};

int
main(int argc, char* argv[])
{
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <service-name> "
              << std::endl;
    return 1;
  }

  std::map<char, uint8_t> flags;
  flags.insert(std::pair<char, uint8_t>('p', ndnsd::SYNC_PROTOCOL_CHRONOSYNC)); //protocol choice
  flags.insert(std::pair<char, uint8_t>('t', ndnsd::discovery::CONSUMER)); //c: consumer - 0, p:producer - 1

  try {
    Consumer consumer(argv[1], flags);
    consumer.execute();
  }
  catch (const std::exception& e) {
  }
}