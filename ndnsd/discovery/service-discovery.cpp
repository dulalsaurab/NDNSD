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

#include "service-discovery.hpp"
#include <iostream>

INIT_LOGGER(ServiceDiscovery);

using namespace ndn::time_literals;

namespace ndnsd {
namespace discovery {

// consumer
ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName,
                                   const std::map<char, uint8_t>& pFlags,
                                   const ndn::time::system_clock::TimePoint& timeStamp,
                                   const DiscoveryCallback& discoveryCallback)
: m_scheduler(m_face.getIoService())
, m_serviceName(serviceName)
, m_Flags(pFlags)
, m_publishTimeStamp(timeStamp)
, m_syncProtocol(m_Flags.find('p')->second)
, m_syncAdapter(m_face, m_syncProtocol, makeSyncPrefix(m_serviceName),
                "/defaultName", 1600_ms,
                std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
, m_appType(m_Flags.find('t')->second)
, m_counter(0)
, m_discoveryCallback(discoveryCallback)
{
}

// producer
ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName, const std::string& userPrefix,
                                   const std::map<char, uint8_t>& pFlags,
                                   const std::string& serviceInfo,
                                   const ndn::time::system_clock::TimePoint& timeStamp,
                                   const ndn::time::milliseconds& prefixExpirationTime,
                                   const DiscoveryCallback& discoveryCallback)
: m_scheduler(m_face.getIoService())
, m_serviceName(serviceName)
, m_userPrefix(userPrefix)
, m_Flags(pFlags)
, m_serviceInfo(serviceInfo)
, m_publishTimeStamp(timeStamp)
, m_prefixLifeTime(prefixExpirationTime)
, m_syncProtocol(m_Flags.find('p')->second)
, m_syncAdapter(m_face, m_syncProtocol, makeSyncPrefix(m_serviceName),
                m_userPrefix, 1600_ms,
                std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
, m_appType(m_Flags.find('t')->second)
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
  // this function will process flags as needed. not used for now.
}

void
ServiceDiscovery::producerHandler()
{
  NDNSD_LOG_INFO("Advertising service under Name: " << m_userPrefix);
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
  NDNSD_LOG_INFO("Requesting service: " << m_serviceName);
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
  m_face.setInterestFilter(ndn::InterestFilter(name).allowLoopback(false),
                           std::bind(&ServiceDiscovery::processInterest, this, _1, _2),
                           std::bind(&ServiceDiscovery::onRegistrationSuccess, this, _1),
                           std::bind(&ServiceDiscovery::registrationFailed, this, _1));
}

void
ServiceDiscovery::processInterest(const ndn::Name& name, const ndn::Interest& interest)
{
  NDNSD_LOG_INFO("Interest received: " << interest.getName());
  auto details = servicesDetails.find(interest.getName())->second;
  sendData(interest.getName(), details);
}

void
ServiceDiscovery::sendData(const ndn::Name& name, const struct Details& serviceDetail)
{

  NDNSD_LOG_INFO("Sending data for: " << name);
  auto timeDiff = ndn::time::system_clock::now() - serviceDetail.timeStamp;
  int status = (timeDiff >= serviceDetail.prefixExpirationTime*1000)
                ? EXPIRED : ACTIVE;

  std::shared_ptr<ndn::Data> replyData = std::make_shared<ndn::Data>(name);
  replyData->setFreshnessPeriod(1_s);

  auto& data = wireEncode(serviceDetail.serviceInfo, status);
  replyData->setContent(data);
  m_keyChain.sign(*replyData);

  try
  {
    m_face.put(*replyData);
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
  data.getContent().parse();
  m_consumerReply.serviceName = data.getName();
  wireDecode(data.getContent().get(tlv::DiscoveryData));
  m_discoveryCallback(m_consumerReply);

  m_counter--;
  if (m_counter <= 0) { stop(); }

}

void
ServiceDiscovery::onTimeout(const ndn::Interest& interest)
{
  if (m_appType == CONSUMER)
  {
    m_counter--;
  }
  NDNSD_LOG_INFO("Interest: " << interest.getName() << "timeout");
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
  if (m_appType == CONSUMER)
  {
    for (auto item: updates)
    {
      NDNSD_LOG_INFO("Fetching data for prefix:" << item.prefix);
      expressInterest(item.prefix);
    }
  }
  else
  {
      m_consumerReply.serviceInfo = "Application prefix " + m_userPrefix+ " updated";
      // m_consumerReply.serviceName = m_userPrefix;
      m_discoveryCallback(m_consumerReply);
  }

}

template<ndn::encoding::Tag TAG>
size_t
ServiceDiscovery::wireEncode(ndn::EncodingImpl<TAG>& encoder,
                             const std::string& info, int status) const
{
  size_t totalLength = 0;

  totalLength += prependStringBlock(encoder, tlv::ServiceInfo, info);
  totalLength += prependNonNegativeIntegerBlock(encoder, tlv::ServiceStatus, status);
  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::DiscoveryData);

  return totalLength;
}

const ndn::Block&
ServiceDiscovery::wireEncode(const std::string& info, int status)
{

  if (m_wire.hasWire()) {
    return m_wire;
  }
  ndn::EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator, info, status);

  ndn::EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer, info, status);
  m_wire = buffer.block();

  return m_wire;
}

void
ServiceDiscovery::wireDecode(const ndn::Block& wire)
{
  auto blockType = wire.type();
  
  if (wire.type() != tlv::DiscoveryData)
  {
    NDNSD_LOG_ERROR("Expected DiscoveryData Block, but Block is of type: #"
              << ndn::to_string(blockType));
  }

  wire.parse();
  m_wire = wire;

  ndn::Block::element_const_iterator it = m_wire.elements_begin();

  if (it != m_wire.elements_end() && it->type() == tlv::ServiceStatus) {
    m_consumerReply.status = ndn::readNonNegativeInteger(*it);
    ++it;
  }
  else {
    NDNSD_LOG_DEBUG("Service status is missing");
  }

  if (it != m_wire.elements_end() && it->type() == tlv::ServiceInfo) {
    m_consumerReply.serviceInfo = readString(*it);
  }
  else {
    NDNSD_LOG_DEBUG("Service information not available");
  }

}
}
}
