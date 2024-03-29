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

NDN_LOG_INIT(ndnsd.comparision.proactive_con);

class ProactiveConsumer
{
public:
  ProactiveConsumer(const ndn::Name& discoveryPrefix)
  : m_discoveryPrefix(discoveryPrefix)
  {
    setInterestFilter(discoveryPrefix);
    m_face.processEvents();
  }

  void
  setInterestFilter(const ndn::Name& name);

  void
  expressInterest(ndn::Interest& interest);

  void
  onData(const ndn::Interest& interest, const ndn::Data& data);

  void
  onTimeout(const ndn::Interest& interest);

  void
  processInterest(const ndn::Name& name, const ndn::Interest& interest);

  void
  onRegistrationSuccess(const ndn::Name& name);

  void
  OnRegistrationFailed(const ndn::Name& name);

private:
  ndn::Face m_face;
  ndn::Name m_discoveryPrefix;
  std::vector<ndn::Name> m_obtainedNames;
  
};

void
ProactiveConsumer::expressInterest(ndn::Interest& interest)
{
  interest.setCanBePrefix(true);
  // interest.setMustBeFresh(false);  
  NDN_LOG_INFO("Sending interest: "<< interest);
  m_face.expressInterest(interest,
                          std::bind(&ProactiveConsumer::onData, this, _1, _2),
                          std::bind(&ProactiveConsumer::onTimeout, this, _1),
                          std::bind(&ProactiveConsumer::onTimeout, this, _1));
}

  void
  ProactiveConsumer::onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    NDN_LOG_INFO("Received data for interest: " << interest.getName());
    std::string _content(reinterpret_cast<const char*>(data.getContent().value()));
    NDN_LOG_INFO("Data content received: ");
    // print data content as well
  }

  void
  ProactiveConsumer::onTimeout(const ndn::Interest& interest)
  {
    NDN_LOG_INFO("Interest: " << interest.getName() << " time out"); 
  }

void
ProactiveConsumer::setInterestFilter(const ndn::Name& name)
{
  ndn::Interest interest(name);
  NDN_LOG_INFO("Setting interest filter on: " << name);
  m_face.setInterestFilter(ndn::InterestFilter(name).allowLoopback(false),
                           std::bind(&ProactiveConsumer::processInterest, this, _1, _2),
                           std::bind(&ProactiveConsumer::onRegistrationSuccess, this, _1),
                           std::bind(&ProactiveConsumer::OnRegistrationFailed, this, _1));
}

void
ProactiveConsumer::processInterest(const ndn::Name& name, const ndn::Interest& interest)
{
  NDN_LOG_INFO("Received interest: " << interest.getName() << " name: " << name);

  auto tempName =  interest.getName().getSubName(-3, 3);

  // check if this name was already fetched, if so don't fetch it again
  for (auto sname : m_obtainedNames)
    {
      if (sname == tempName){
        NDN_LOG_INFO("This name was already received before, dont fetch");
        return;
      }
    }
  m_obtainedNames.push_back(tempName);

  ndn::Name applicationPrefix(tempName); //.append("service-info"));
  ndn::Interest dataInterest(applicationPrefix);

  NDN_LOG_INFO("Sending interest: " << applicationPrefix << " to fetch service info");
  expressInterest(dataInterest);
}

void
ProactiveConsumer::OnRegistrationFailed(const ndn::Name& name)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix " << name << " in local hub's daemon");
}

void
ProactiveConsumer::onRegistrationSuccess(const ndn::Name& name)
{
  NDN_LOG_DEBUG("Successfully registered prefix: " << name);
}

int
main()
{
  NDN_LOG_INFO("Starting proactive consumer");
  ProactiveConsumer pcon("/uofm/discovery/printer");
}