/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  The University of Memphis
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

namespace tlv {

enum {
  DiscoveryData = 128,
  ServiceInfo = 129,
  ServiceStatus = 130
};

}
/*
 map: stores data from producer to serve on demand
 first arg: string: prefix name, second arg: parameters (service name, 
  publish timestamps, lifetime, serviceInfo)
*/

enum {
  PRODUCER = 0,
  CONSUMER = 1,
};

// service status
enum {
  EXPIRED = 0,
  ACTIVE = 1,
};

struct Details
{
  ndn::Name serviceName;
  ndn::time::system_clock::TimePoint timeStamp;
  ndn::time::milliseconds prefixExpirationTime;
  std::string serviceInfo;
  int status;
};

typedef struct Details Details;
std::map<ndn::Name, Details> servicesDetails;
typedef std::function<void(const Details& servoceUpdates)> DiscoveryCallback;

class ServiceDiscovery
{

public:

  /* ctor for Consumer 
    serviceName: Service consumer is interested on. e.g. = /<prefix>/discovery/printer
    timeStamp: when was the service requested.
  */
  // consumer
  ServiceDiscovery(const ndn::Name& serviceName,
                   const std::map<char, uint8_t>& pFlags,
                   const ndn::time::system_clock::TimePoint& timeStamp,
                   const DiscoveryCallback& discoveryCallback);

  /* ctor for Producer 
    serviceName: Service producer is willing to publish. syncPrefix will be 
    constructed out of service name
    e.g. serviceName printer, syncPrefix = /<prefix>/discovery/printer
    userPrefix: Application prefix name
    timeStamp: when the userPrefix was updated the last time. When combine 
    with prefixExpTime, the prefix will expire from that time onward.
    The assumption here is that the machines are loosely time synchronized.
    serviceInfo: detail information about the service, this can also be a Json (later)
  */

  ServiceDiscovery(const ndn::Name& serviceName, const std::string& userPrefix,
                   const std::map<char, uint8_t>& pFlags,
                   const std::string &serviceInfo,
                   const ndn::time::system_clock::TimePoint& timeStamp,
                   const ndn::time::milliseconds& prefixExpirationTime,
                   const DiscoveryCallback& discoveryCallback);

  void
  run();

  /*
    Cancel all pending operations, close connection to forwarder
    and stop the ioService.
  */
  void 
  stop();

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
  processFalgs();

  ndn::Name
  makeSyncPrefix(ndn::Name& service);

private:
  void 
  doUpdate(const ndn::Name& prefix);

  void
  processSyncUpdate(const std::vector<ndnsd::SyncDataInfo>& updates);

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
  
  void
  onData(const ndn::Interest& interest, const ndn::Data& data);

  void
  onTimeout(const ndn::Interest& interest);

  void
  expressInterest(const ndn::Name& interest);

  template<ndn::encoding::Tag TAG>
  size_t
  wireEncode(ndn::EncodingImpl<TAG>& block, const std::string& info, int status) const;

  const ndn::Block&
  wireEncode(const std::string& info, int status);

  void
  wireDecode(const ndn::Block& wire);

  // ndn::Data&
  // addContent(const ndn::Block& block)

  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  ndn::KeyChain m_keyChain;

  ndn::Name m_serviceName;
  std::string m_userPrefix;
  std::map<char, uint8_t> m_Flags;
  std::string m_serviceInfo;
  ndn::time::system_clock::TimePoint m_publishTimeStamp;
  ndn::time::milliseconds m_prefixLifeTime;
  uint8_t m_appType;
  uint8_t m_counter;
  DiscoveryCallback m_discoveryCallback;
  Details m_consumerReply;

  uint32_t m_syncProtocol;
  SyncProtocolAdapter m_syncAdapter;
  static const ndn::Name DEFAULT_CONSUMER_ONLY_NAME;
  mutable ndn::Block m_wire;
};
}}
#endif // NDNSD_SERVICE_DISCOVERY_HPP