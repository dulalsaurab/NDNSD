/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  The University of Memphis
 *
 * This file is originally from NLSR and is modified here per the use.
 * See NLSR's AUTHORS.md for complete list of NLSR authors and contributors.
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

  @ most part of sync-adapter code is taken from NLSR/communication/

 **/

#include "sync-adapter.hpp"

#include <ndn-cxx/util/logger.hpp>

#include <iostream>

NDN_LOG_INIT(ndnsd.SyncProtocolAdapter);

namespace ndnsd {

const auto CHRONOSYNC_FIXED_SESSION = ndn::name::Component::fromNumber(0);

SyncProtocolAdapter::SyncProtocolAdapter(ndn::Face& face,
                                         uint8_t syncProtocol,
                                         const ndn::Name& syncPrefix,
                                         const ndn::Name& userPrefix,
                                         ndn::time::milliseconds syncInterestLifetime,
                                         const SyncUpdateCallback& syncUpdateCallback)
  : m_syncProtocol(syncProtocol)
  , m_syncUpdateCallback(syncUpdateCallback)
{
  if (m_syncProtocol == SYNC_PROTOCOL_CHRONOSYNC) {
    NDNSD_LOG_DEBUG("Using ChronoSync");
    m_chronoSyncLogic = std::make_shared<chronosync::Logic>(face,
                          syncPrefix,
                          userPrefix,
                          std::bind(&SyncProtocolAdapter::onChronoSyncUpdate, this, _1),
                          chronosync::Logic::DEFAULT_NAME,
                          chronosync::Logic::DEFAULT_VALIDATOR,
                          chronosync::Logic::DEFAULT_RESET_TIMER,
                          chronosync::Logic::DEFAULT_CANCEL_RESET_TIMER,
                          chronosync::Logic::DEFAULT_RESET_INTEREST_LIFETIME,
                          syncInterestLifetime,
                          chronosync::Logic::DEFAULT_SYNC_REPLY_FRESHNESS,
                          chronosync::Logic::DEFAULT_RECOVERY_INTEREST_LIFETIME,
                          CHRONOSYNC_FIXED_SESSION);
  }
  else {
    NDNSD_LOG_DEBUG("Using PSync");
    m_psyncLogic = std::make_shared<psync::FullProducer>(80,
                     face,
                     syncPrefix,
                     userPrefix,
                     std::bind(&SyncProtocolAdapter::onPSyncUpdate, this, _1),
                     syncInterestLifetime);
  }
}

void
SyncProtocolAdapter::addUserNode(const ndn::Name& userPrefix)
{
  if (m_syncProtocol == SYNC_PROTOCOL_CHRONOSYNC) {
    m_chronoSyncLogic->addUserNode(userPrefix, chronosync::Logic::DEFAULT_NAME, CHRONOSYNC_FIXED_SESSION);
  }
  else {
    m_psyncLogic->addUserNode(userPrefix);
  }
}

void
SyncProtocolAdapter::publishUpdate(const ndn::Name& userPrefix)
{
  NDN_LOG_INFO("Publishing update for Sync Prefix " << userPrefix);
  if (m_syncProtocol == SYNC_PROTOCOL_CHRONOSYNC) {
    auto seq = m_chronoSyncLogic->getSeqNo(userPrefix) + 1;
    NDN_LOG_INFO("SeqNumber :" << seq);
    m_chronoSyncLogic->updateSeqNo(seq, userPrefix);
  }
  else {
    m_psyncLogic->publishName(userPrefix);
  }
}

void
SyncProtocolAdapter::onChronoSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates)
{
  NDN_LOG_INFO("Received ChronoSync update event");
  std::vector<ndnsd::SyncDataInfo> dinfo;

  for (const auto& update : updates) {
    // Remove CHRONOSYNC_FIXED_SESSION
    SyncDataInfo di;
    di.prefix = update.session.getPrefix(-1);
    di.highSeq = update.high;
    dinfo.insert(dinfo.begin(), di);
  }
  m_syncUpdateCallback(dinfo);
}

void
SyncProtocolAdapter::onPSyncUpdate(const std::vector<psync::MissingDataInfo>& updates)
{
  NDN_LOG_INFO("Received PSync update event");
  std::vector<ndnsd::SyncDataInfo> dinfo;

  for (const auto& update : updates) {
    SyncDataInfo di;
    di.prefix = update.prefix;
    di.highSeq = update.highSeq;
    dinfo.insert(dinfo.begin(), di);
  }
  m_syncUpdateCallback(dinfo);
}

} // namespace ndnsd