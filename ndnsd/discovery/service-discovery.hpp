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


#ifndef NDNSD_SERVICE_DISCOVERY_HPP
#define NDNSD_SERVICE_DISCOVERY_HPP

#include "ndnsd/communication/sync-adapter.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

#include <iostream>
#include "logger.hpp"

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

typedef struct Details Details;
std::map<ndn::Name, Details> servicesDetails;

class ServiceDiscovery
{

public:

  /* ctor for Consumer 
    serviceName: Service consumer is interested on. e.g. = /<prefix>/discovery/printer
    timeStamp: when was the service requested.
  */
  // consumer
  ServiceDiscovery(const ndn::Name& serviceName,
                   const std::map<char, std::string>& pFlags,
                   const ndn::time::system_clock::TimePoint& timeStamp);

  /* ctor for Producer 
    serviceName: Service producer is willing to publish. syncPrefix will be 
    constructed out of service name,
    e.g. serviceName printer, syncPrefix = /<prefix>/discovery/printer
    userPrefix: Application prefix name
    timeStamp: when the userPrefix was updated the last time. When combine 
    with prefixExpTime, the prefix will expire from that time onward.
    The assumption here is that the machines are loosely time synchronized.
    serviceInfo: detail information about the service, this can also be a Json (later)
  */

  ServiceDiscovery(const ndn::Name& serviceName, const std::string& userPrefix,
                   const std::map<char, std::string>& pFlags,
                   const std::string &serviceInfo,
                   const ndn::time::system_clock::TimePoint& timeStamp,
                   ndn::time::milliseconds prefixExpirationTime);

  void
  run();

  void
  producerHandler();

  void
  consumerHandler();

  uint32_t
  getSyncProtocol() const
  {
    return m_syncProtocol;
  }

  void
  setSyncProtocol(std::string syncProtocol)
  {
    m_syncProtocol = (syncProtocol.compare("psync"))
                     ? SYNC_PROTOCOL_CHRONOSYNC
                     : SYNC_PROTOCOL_PSYNC;
  }

  void
  processFalgs();

  ndn::Name
  makeSyncPrefix(ndn::Name& service);

private:
  void 
  doUpdate(const ndn::Name& prefix);

  void
  processSyncUpdate(const ndn::Name& updateName, uint64_t seqNo);

  void
  processInterest(const ndn::Name& name, const ndn::Interest& interest);

  void
  sendData(const ndn::Name& name, const struct Details& serviceDetail);

  void
  setInterestFilter(const ndn::Name& prefix, const bool loopback = false);

  void
  registrationFailed(const ndn::Name& name);

  void
  onRegistrationSuccess(const ndn::Name& name);
  

  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  ndn::KeyChain m_keyChain;
  // ndn::security::SigningInfo& m_signingInfo;

  ndn::Name m_serviceName;

  std::string m_userPrefix;
  std::map<char, std::string> m_producerFlags;
  std::string m_serviceInfo;
  ndn::time::system_clock::TimePoint m_publishTimeStamp;
  ndn::time::milliseconds m_prefixLifeTime;

  uint32_t m_syncProtocol;
  SyncProtocolAdapter m_syncAdapter;
  static const ndn::Name DEFAULT_CONSUMER_ONLY_NAME;
};
}}
#endif // NDNSD_SERVICE_DISCOVERY_HPP