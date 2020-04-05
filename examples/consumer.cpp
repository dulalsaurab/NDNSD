#include<iostream>
#include "ndnsd/discovery/service-discovery.hpp"
#include <list>

class Consumer 
{
public:
  Consumer(const ndn::Name& serviceName, const std::map<char, std::string>& pFlags)
  
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
  processCallback(const std::string& callback)
  {
      std::cout << "callback: " << callback.c_str() <<  std::endl;
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

  std::map<char, std::string> flags;
  flags.insert(std::pair<char, std::string>('p', "sync"));
  flags.insert(std::pair<char, std::string>('t', "c"));

  try {
    Consumer consumer(argv[1], flags);
    consumer.execute();
  }
  catch (const std::exception& e) {

    // NDN_LOG_ERROR(e.what());
  }
}