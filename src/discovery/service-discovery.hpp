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

#include "src/communication/sync-adapter.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>

#include <iostream>

using namespace ndn::time_literals;

namespace ndnsd {
namespace discovery {

class ServiceDiscovery
{

public:

  /* ctor for Producer 
    serviceName: syncPrefix will be constructed out of service name,
    e.g. serviceName printer, syncPrefix = /<prefix>/discovery/printer
    userPrefix: Application prefix name
    timeStamp: when the userPrefix was updated the last time. When combine 
    with prefixExpTime, the prefix will expire from that time onward.
    The assumption here is that the machines are loosely synchronized.
    serviceInfo: detail information about the service, this can also be a Json (later)
  */

  ServiceDiscovery(const ndn::Name& serviceName, const std::string& userPrefix,
                   const std::list<std::string>& pFlags,
                   const std::string &serviceInfo,
                   const ndn::time::system_clock::TimePoint& timeStamp,
                   ndn::time::milliseconds prefixExpirationTime);

  void
  run();

  uint32_t
  getSyncProtocol() const
  {
    return m_syncProtocol;
  }

  void
  setSyncProtocol(std::string syncProtocol)
  {
    
    m_syncProtocol = (syncProtocol.compare("chronosync")) 
                     ? SYNC_PROTOCOL_CHRONOSYNC 
                     : SYNC_PROTOCOL_PSYNC;
  }

  void
  ProcessFalgs();

private:
  void 
  doUpdate(const ndn::Name& prefix);

  void
  processSyncUpdate(const std::vector<psync::MissingDataInfo>& updates);
   
   ndn::Name m_serviceName;
   std::string m_userPrefix;
   std::list<std::string> m_producerFlags;
   std::string m_serviceInfo;
   ndn::time::system_clock::TimePoint m_publishTimeStamp;
   ndn::time::milliseconds m_prefixLifeTime;
   
   ndn::Face m_face;
   ndn::Scheduler m_scheduler;

   // ndnsd::SyncProtocolAdapter m_syncProtocolAdapter;
   uint32_t m_syncProtocol;
};
}}
#endif // NDNSD_SERVICE_DISCOVERY_HPP