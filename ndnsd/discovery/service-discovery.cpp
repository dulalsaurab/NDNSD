/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  The University of Memphis
 *
 * This file is part of NDNSD.
 * See AUTHORS.md for complete list of NDNSD authors and contributors.
 *
 * NDNSD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NDNSD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * NDNSD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "service-discovery.hpp"
#include <iostream>

INIT_LOGGER(ServiceDiscovery);

using namespace ndn::time_literals;

namespace ndnsd {
namespace discovery {

// consumer
ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName,
                                   const std::map<char, std::string>& pFlags,
                                   const ndn::time::system_clock::TimePoint& timeStamp,
                                   const DiscoveryCallback& discoveryCallback)
: m_scheduler(m_face.getIoService())
, m_serviceName(serviceName)
, m_Flags(pFlags)
, m_publishTimeStamp(timeStamp)
, m_syncProtocol(SYNC_PROTOCOL_PSYNC)
, m_syncAdapter(m_face, getSyncProtocol(), makeSyncPrefix(m_serviceName),
                "/defaultName", 1600_ms,
                std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
, m_appType(1)
, m_counter(0)
, m_discoveryCallback(discoveryCallback)
{
}

// producer
ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName, const std::string& userPrefix,
                                   const std::map<char, std::string>& pFlags,
                                   const std::string& serviceInfo,
                                   const ndn::time::system_clock::TimePoint& timeStamp,
                                   ndn::time::milliseconds prefixExpirationTime,
                                   const DiscoveryCallback& discoveryCallback)
: m_scheduler(m_face.getIoService())
, m_serviceName(serviceName)
, m_userPrefix(userPrefix)
, m_Flags(pFlags)
, m_serviceInfo(serviceInfo)
, m_publishTimeStamp(timeStamp)
, m_prefixLifeTime(prefixExpirationTime)
, m_syncProtocol(SYNC_PROTOCOL_PSYNC)
, m_syncAdapter(m_face, getSyncProtocol(), makeSyncPrefix(m_serviceName),
                m_userPrefix, 1600_ms,
                std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
, m_appType(0)
, m_discoveryCallback(discoveryCallback)
{
  setInterestFilter(m_userPrefix);
}

ndn::Name
ServiceDiscovery::makeSyncPrefix(ndn::Name& service)
{
  ndn::Name sync("/discovery");
  sync.append(service);
  return sync;
}

void
ServiceDiscovery::processFalgs()
{
   setSyncProtocol(m_Flags.find('p')->second);
}

void
ServiceDiscovery::producerHandler()
{
  std::cout << "Advertising service under Name: " << m_userPrefix << std::endl;
  processFalgs();

  // store service
  Details d = {m_serviceName, m_publishTimeStamp, m_prefixLifeTime, m_serviceInfo};
  servicesDetails.emplace(m_userPrefix, d);

  doUpdate(m_userPrefix);
  run();
}

void
ServiceDiscovery::consumerHandler()
{
  std::cout << "Requesting service: " << m_serviceName << std::endl;
  processFalgs();
  run();
}


void
ServiceDiscovery::run()
{
  m_face.processEvents();
}

void 
ServiceDiscovery::stop()
{
  m_face.shutdown();
  m_face.getIoService().stop();
}

void
ServiceDiscovery::setInterestFilter(const ndn::Name& name, const bool loopback)
{
  NDNSD_LOG_INFO("Setting interest filter on: " << name);
  std::cout << "Listening on interest: " << name << std::endl;
  m_face.setInterestFilter(ndn::InterestFilter(name).allowLoopback(false),
                           std::bind(&ServiceDiscovery::processInterest, this, _1, _2),
                           std::bind(&ServiceDiscovery::onRegistrationSuccess, this, _1),
                           std::bind(&ServiceDiscovery::registrationFailed, this, _1));
}

void
ServiceDiscovery::processInterest(const ndn::Name& name, const ndn::Interest& interest)
{
  std::cout << "Interest received: " << interest.getName() << std::endl;
  auto details = servicesDetails.find("/printer1")->second;
  sendData(interest.getName(), details);
}

void
ServiceDiscovery::sendData(const ndn::Name& name, const struct Details& serviceDetail)
{
  static const std::string content("Hello, world!");
  std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(name);
  data->setFreshnessPeriod(1_s);
  data->setContent(reinterpret_cast<const uint8_t*>(content.data()), content.size());

  m_keyChain.sign(*data);
  try
  {
    m_face.put(*data);
  }
  catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
  }
}

void
ServiceDiscovery::expressInterest(const ndn::Name& name)
{
  ndn::Interest interest(name);
  interest.setCanBePrefix(false);
  interest.setMustBeFresh(true);
  interest.setInterestLifetime(160_ms);

  m_face.expressInterest(interest,
                         bind(&ServiceDiscovery::onData, this, _1, _2),
                         bind(&ServiceDiscovery::onTimeout, this, _1),
                         bind(&ServiceDiscovery::onTimeout, this, _1));

}

void
ServiceDiscovery::onData(const ndn::Interest& interest, const ndn::Data& data)
{
  std::cout << "Sending data for interest: " << interest << std::endl;
  // auto content = data.getContent().value();
  auto content = std::string(reinterpret_cast<const char*>(data.getContent().value()));
  // std::string temp = content;
  m_discoveryCallback(content);

  m_counter--;
  if (m_counter <= 0) { stop(); }

}

void
ServiceDiscovery::onTimeout(const ndn::Interest& interest)
{
  std::cout << "i got something onTimeout" << std::endl;
}

void
ServiceDiscovery::registrationFailed(const ndn::Name& name)
{
  NDNSD_LOG_ERROR("ERROR: Failed to register prefix " << name << " in local hub's daemon");
  // BOOST_THROW_EXCEPTION(Error("Error: Prefix registration failed"));
}

void
ServiceDiscovery::onRegistrationSuccess(const ndn::Name& name)
{
  NDNSD_LOG_DEBUG("Successfully registered prefix: " << name);
}

void
ServiceDiscovery::doUpdate(const ndn::Name& prefix)
{
  m_syncAdapter.publishUpdate(m_userPrefix);
  NDNSD_LOG_INFO("Publish: " << prefix);
}

void
ServiceDiscovery::processSyncUpdate(const std::vector<ndnsd::SyncDataInfo>& updates)
{
  m_counter = updates.size();
  std::cout << "size: " << updates.size();
  if (m_appType == CONSUMER)
  {
    for (auto item: updates)
    {
      std::cout << "Fetching data for prefix:" << item.prefix << std::endl;
      expressInterest(item.prefix);
    }
  }
  else
  {
    m_discoveryCallback("Application prefix updated: ");
  }

}
}}
