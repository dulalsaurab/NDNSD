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

// NDN_LOG_INIT(examples.FullSyncApp);

using namespace ndn::time_literals;


// ServiceDiscovery::ServiceDiscovery(const std::vector<std::string>& services, 
//                                    const std::list<char* >& flags)
// : m_scheduler(m_face.getIoService())
// {

// } 

ServiceDiscovery::ServiceDiscovery(const ndn::Name& syncPrefix, const std::string& userPrefix)


// }
  // ServiceDiscovery(const ndn::Name& syncPrefix, const std::string& userPrefix,
  //          int numDataStreams, int maxNumPublish)
    : m_scheduler(m_face.getIoService())
    , m_rng(ndn::random::getRandomNumberEngine())
{
  //   , m_fullProducer(80, m_face, syncPrefix, userPrefix,
  //                    std::bind(&Producer::processSyncUpdate, this, _1),
  //                    1600_ms, 1600_ms)
  //   , m_numDataStreams(numDataStreams)
  //   , m_maxNumPublish(maxNumPublish)
  //   , m_rangeUniformRandom(0, 60000)
  //   // Add user prefixes and schedule updates for them in specified interval
  //   for (int i = 0; i < m_numDataStreams; i++) {
      ndn::Name prefix(userPrefix + "-" + ndn::to_string(1));
  //     m_fullProducer.addUserNode(prefix);
      m_scheduler.schedule(ndn::time::milliseconds(m_rangeUniformRandom(m_rng)),
                           [this, prefix] { 
                            std::cout << "hello world" << std::endl;
                            // doUpdate(prefix); 
                            
                            });

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

