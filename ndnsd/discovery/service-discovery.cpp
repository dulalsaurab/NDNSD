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

/*
 map: stores data from producer to serve on demand
 first arg: string: prefix name, second arg: parameters (service name, 
  publish timestamps, lifetime, serviceInfo)
*/
struct Details
{
  ndn::Name serviceName;
  ndn::time::system_clock::TimePoint timeStamp;
  ndn::time::milliseconds prefixExpirationTime;
  std::string serviceInfo;
};

std::map<ndn::Name, Details> servicesDetails;

ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName, const std::string& userPrefix,
                                   const std::map<char, std::string>& pFlags,
                                   const std::string& serviceInfo,
                                   const ndn::time::system_clock::TimePoint& timeStamp,
                                   ndn::time::milliseconds prefixExpirationTime)
: m_scheduler(m_face.getIoService())
, m_serviceName(serviceName)
, m_userPrefix(userPrefix)
, m_producerFlags(pFlags)
, m_serviceInfo(serviceInfo)
, m_publishTimeStamp(timeStamp)
, m_prefixLifeTime(prefixExpirationTime)
, m_syncProtocol(SYNC_PROTOCOL_PSYNC)
, m_syncAdapter(m_face, getSyncProtocol(), makeSyncPrefix(m_serviceName),
                m_userPrefix, 1600_ms,
                std::bind(&ServiceDiscovery::processSyncUpdate, this, _1, _2))

{
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
   setSyncProtocol(m_producerFlags.find('p')->second);
}

void
ServiceDiscovery::run()
{
  std::cout << "inside run " << m_userPrefix << std::endl;
  processFalgs();

  // store service
  Details d = {m_serviceName, m_publishTimeStamp, m_prefixLifeTime, m_serviceInfo};
  servicesDetails.emplace(m_userPrefix, d);
  
  doUpdate(m_userPrefix);
  setInterestFilter(m_userPrefix);

  m_face.processEvents();
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
  std::cout << "I received an interest: " << interest.getName().get(-3).toUri() << std::endl;

  auto abc = servicesDetails.find("/printer1")->second;

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
  NDNSD_LOG_INFO("Publish: " << prefix );
}

void
ServiceDiscovery::processSyncUpdate(const ndn::Name& updateName, uint64_t seqNo)
{
  std::cout << updateName << seqNo << "hello world" << std::endl;
  // for (const auto& update : updates) {
  //   for (uint64_t i = update.lowSeq; i <= update.highSeq; i++) {
  //     NDN_LOG_INFO("Update " << update.prefix << "/" << i);
  //   }
  // }
}

}}
