/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  The University of Memphis
 *
 * This file is part of NDNSD.
 * Author: Saurab Dulal (sdulal@memphis.edu)
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
#include <string>
#include <iostream>

using namespace ndn::time_literals;

namespace ndnsd {
namespace discovery {

// consumer
ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName,
                                   const std::map<char, uint8_t>& pFlags,
                                   const DiscoveryCallback& discoveryCallback)
  : m_serviceName(serviceName)
  , m_appType(processFalgs(pFlags, 't'))
  , m_counter(0)
  , m_syncProtocol(processFalgs(pFlags, 'p'))
  , m_syncAdapter(m_face, m_syncProtocol, makeSyncPrefix(m_serviceName),
                  "/defaultName", 1600_ms,
                  std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
  , m_discoveryCallback(discoveryCallback)
{
}

// producer
ServiceDiscovery::ServiceDiscovery(const std::string& filename,
                                   const std::map<char, uint8_t>& pFlags,
                                   const DiscoveryCallback& discoveryCallback)
  : m_filename(filename)
  , m_fileProcessor(m_filename)
  , m_appType(processFalgs(pFlags, 't'))
  , m_syncProtocol(processFalgs(pFlags, 'p'))
  , m_syncAdapter(m_face, m_syncProtocol, makeSyncPrefix(m_fileProcessor.getServiceName()),
                  m_fileProcessor.getAppPrefix(), 1600_ms,
                  std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
  , m_discoveryCallback(discoveryCallback)
{
  setUpdateProducerState();
  setInterestFilter(m_producerState.applicationPrefix);

  // listen on reload prefix as well.
  setInterestFilter(ndnsd::discovery::NDNSD_RELOAD_PREFIX);
}

void
ServiceDiscovery::setUpdateProducerState(bool update)
{
  NDNSD_LOG_INFO("Setting/Updating producers state: ");
  if (update)
  {
    NDNSD_LOG_INFO("reached here");
    m_fileProcessor.processFile();
  }

  m_producerState.serviceName = m_fileProcessor.getServiceName();
  m_producerState.applicationPrefix = m_fileProcessor.getAppPrefix();
  m_producerState.serviceLifetime = m_fileProcessor.getServiceLifetime();
  m_producerState.publishTimestamp = ndn::time::system_clock::now();
  m_producerState.serviceMetaInfo = m_fileProcessor.getServiceMeta();
  m_producerState.status = ACTIVE;

}

ndn::Name
ServiceDiscovery::makeSyncPrefix(ndn::Name& service)
{
  ndn::Name sync("/discovery");
  sync.append(service);
  return sync;
}

uint8_t
ServiceDiscovery::processFalgs(const std::map<char, uint8_t>& flags, const char type)
{
  auto key = flags.find(type);
  if (key != flags.end()) 
  {
    return flags.find(type)->second;
  } 
  else 
  {
    NDN_THROW(Error("Flag type not found!"));
    NDNSD_LOG_ERROR("Flag type not found!");
  }
}

void
ServiceDiscovery::producerHandler()
{
  auto& prefix = m_producerState.applicationPrefix;
  NDNSD_LOG_INFO("Advertising service under Name: " << prefix);

  // store service
  // Details d = {m_serviceName, m_publishTimeStamp, m_prefixLifeTime, m_serviceInfo, ACTIVE};
  // serviceDetails.emplace(m_userPrefix, d);

  doUpdate(prefix);
  run();
}

void
ServiceDiscovery::consumerHandler()
{
  NDNSD_LOG_INFO("Requesting service: " << m_serviceName);
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
  auto interestName = interest.getName();

  // check if the interest is for service detail or to update the service
  if (interestName == NDNSD_RELOAD_PREFIX)
  {
    NDNSD_LOG_INFO("receive request to reload");
    // reload file.
    m_fileProcessor.processFile();
    setUpdateProducerState(true);
    // send back the response
    static const std::string content("Update Successful");
    // Create Data packet
    auto data = make_shared<ndn::Data>(interest.getName());
    data->setFreshnessPeriod(10_s);
    data->setContent(reinterpret_cast<const uint8_t*>(content.data()), content.size());

    m_keyChain.sign(*data);
    m_face.put(*data);
  }
  else
  {
    sendData(interest.getName());
  }
}

std::string
ServiceDiscovery::makeDataContent()
{
  // |serviceName|applicationPrefix|key|val|key|val|...and so on
  std::string dataContent = m_producerState.serviceName.toUri();
  dataContent += "|";
  dataContent += m_producerState.applicationPrefix.toUri();

  for (auto const& item : m_producerState.serviceMetaInfo)
  {
    NDNSD_LOG_INFO("first"<< item.first << "second::"<< item.second);
    dataContent += "|";
    dataContent += item.first;
    dataContent += "|";
    dataContent += item.second;
  }
  NDNSD_LOG_INFO("dataContent::::"<< dataContent);
  return dataContent;
}

void
ServiceDiscovery::sendData(const ndn::Name& name)
{
  NDNSD_LOG_INFO("Sending data for: " << name);
  auto timeDiff = ndn::time::system_clock::now() - m_producerState.publishTimestamp;
  m_producerState.status = (timeDiff >= m_producerState.serviceLifetime*1000)
                          ? EXPIRED : ACTIVE;

  std::shared_ptr<ndn::Data> replyData = std::make_shared<ndn::Data>(name);
  replyData->setFreshnessPeriod(1_s);

  auto dataContent = makeDataContent();
  auto& data = wireEncode(dataContent, m_producerState.status);
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
  wireDecode(data.getContent().get(tlv::DiscoveryData));
  m_discoveryCallback(m_consumerReply);

  m_counter--;
  if (m_counter <= 0)
    stop();
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
}

void
ServiceDiscovery::onRegistrationSuccess(const ndn::Name& name)
{
  NDNSD_LOG_DEBUG("Successfully registered prefix: " << name);
}

void
ServiceDiscovery::doUpdate(const ndn::Name& prefix)
{
  m_syncAdapter.publishUpdate(prefix);
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
    m_consumerReply.serviceMetaInfo = "Application prefix " + m_producerState.applicationPrefix.toUri() + " updated";
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

  if (m_wire.hasWire())
    return m_wire;

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
    m_consumerReply.serviceMetaInfo = readString(*it);
  }
  else {
    NDNSD_LOG_DEBUG("Service information not available");
  }

}
} // namespace discovery
} // namespace ndnsd
