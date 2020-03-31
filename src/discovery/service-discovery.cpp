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

using namespace ndn::time_literals;

namespace ndnsd {
namespace discovery {

ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName, const std::string& userPrefix,
                                   const std::list<std::string>& pFlags,
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

{
  ProcessFalgs();

  // ,m_syncLogic(m_syncFace, m_confParam.getSyncProtocol(), m_confParam.getSyncPrefix(),
  //               m_nameLsaUserPrefix, m_confParam.getSyncInterestLifetime(),
  //               std::bind(&SyncLogicHandler::processUpdate, this, _1, _2))

  // SyncProtocolAdapter(ndn::Face& facePtr,
  //                     int32_t syncProtocol,
  //                     const ndn::Name& syncPrefix,
  //                     const ndn::Name& userPrefix,
  //                     ndn::time::milliseconds syncInterestLifetime,
  //                     const SyncUpdateCallback& syncUpdateCallback);


}

void
ServiceDiscovery::ProcessFalgs()
{
  // for( auto const& a : m_producerFlags )
  // {
  //     std::cout << a << std::endl;
  // }
}

void
ServiceDiscovery::run()
{
  m_face.processEvents();
}

void
ServiceDiscovery::doUpdate(const ndn::Name& prefix)
{
  // m_fullProducer.publishName(prefix);

  // uint64_t seqNo = m_fullProducer.getSeqNo(prefix).value();
  // NDN_LOG_INFO("Publish: " << prefix << "/" << seqNo);

  // if (seqNo < m_maxNumPublish) {
  //   m_scheduler.schedule(ndn::time::milliseconds(m_rangeUniformRandom(m_rng)),
  //                        [this, prefix] { doUpdate(prefix); });
  // }
}

void
ServiceDiscovery::processSyncUpdate(const std::vector<psync::MissingDataInfo>& updates)
{
  // for (const auto& update : updates) {
  //   for (uint64_t i = update.lowSeq; i <= update.highSeq; i++) {
  //     NDN_LOG_INFO("Update " << update.prefix << "/" << i);
  //   }
  // }
}

// void
// ndnsd::printUsage()
// {
//   std::cout << "Usage:\n" << "ProgramName"  << " [-h] [-V] COMMAND [<Command Options>]\n"
//     "       -h print usage and exit\n"
//     "       -V print version and exit\n"
//     "\n"
//     "   COMMAND can be one of the following:\n"
//     "       lsdb\n"
//     "           display NLSR lsdb status\n"
//     "       routing\n"
//     "           display routing table status\n"
//     "       status\n"
//     "           display all NLSR status (lsdb & routingtable)\n"
//     "       advertise name\n"
//     "           advertise a name prefix through NLSR\n"
//     "       advertise name save\n"
//     "           advertise and save the name prefix to the conf file\n"
//     "       withdraw name\n"
//     "           remove a name prefix advertised through NLSR\n"
//     "       withdraw name delete\n"
//     "           withdraw and delete the name prefix from the conf file"
//     << std::endl;
// }

}}
