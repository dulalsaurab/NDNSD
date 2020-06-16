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

#include <ndn-cxx/util/logger.hpp>

using namespace ndn::time_literals;

NDN_LOG_INIT(ndnsd.ServiceDiscovery);

namespace ndnsd {
namespace discovery {

// consumer
ServiceDiscovery::ServiceDiscovery(const ndn::Name& serviceName,
                                   const std::map<char, uint8_t>& pFlags,
                                   const DiscoveryCallback& discoveryCallback)
  : m_serviceName(serviceName)
  , m_appType(processFalgs(pFlags, 't', false))
  , m_counter(0)
  , m_syncProtocol(processFalgs(pFlags, 'p', false))
  , m_syncAdapter(m_face, m_syncProtocol, makeSyncPrefix(m_serviceName),
                  "/defaultName", 1600_ms,
                  std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
  , m_discoveryCallback(discoveryCallback)
{
  // all the optional flag contDiscovery should be set here TODO:
  m_contDiscovery = processFalgs(pFlags, 'c', true);
}

// producer
ServiceDiscovery::ServiceDiscovery(const std::string& filename,
                                   const std::map<char, uint8_t>& pFlags,
                                   const DiscoveryCallback& discoveryCallback)
  : m_filename(filename)
  , m_fileProcessor(m_filename)
  , m_appType(processFalgs(pFlags, 't', false))
  , m_syncProtocol(processFalgs(pFlags, 'p', false))
  , m_syncAdapter(m_face, m_syncProtocol, makeSyncPrefix(m_fileProcessor.getServiceName()),
                  m_fileProcessor.getAppPrefix(), 1600_ms,
                  std::bind(&ServiceDiscovery::processSyncUpdate, this, _1))
  , m_discoveryCallback(discoveryCallback)
{

  setUpdateProducerState();
  // service is ACTIVE at this point
  m_serviceStatus = ACTIVE;

  setInterestFilter(m_producerState.applicationPrefix);

  // listen on reload prefix as well.
  setInterestFilter(ndnsd::discovery::NDNSD_RELOAD_PREFIX);
}

void
ServiceDiscovery::setUpdateProducerState(bool update)
{
  NDN_LOG_INFO("Setting/Updating producers state: ");
  if (update)
  {
    m_fileProcessor.processFile();
     // should restrict updating service name and application prefix
    if (m_producerState.serviceName != m_fileProcessor.getServiceName())
      NDN_LOG_ERROR("Service Name cannot be changed while application is running");
    if (m_producerState.applicationPrefix != m_fileProcessor.getAppPrefix())
      NDN_LOG_ERROR("Application Prefix cannot be changed while application is running");
  }
  else
  {
    m_producerState.serviceName = m_fileProcessor.getServiceName();
    m_producerState.applicationPrefix = m_fileProcessor.getAppPrefix();
  }
  // update
  m_producerState.serviceLifetime = m_fileProcessor.getServiceLifetime();
  m_producerState.publishTimestamp = ndn::time::system_clock::now();
  m_producerState.serviceMetaInfo = m_fileProcessor.getServiceMeta();

  // Reset the content of the wire after each reload.
  // This could have been better, but leaving as it is for now.
  if(m_wire.hasWire())
    m_wire.reset();
}

ndn::Name
ServiceDiscovery::makeSyncPrefix(ndn::Name& service)
{
  ndn::Name sync("/discovery");
  sync.append(service);
  return sync;
}

uint8_t
ServiceDiscovery::processFalgs(const std::map<char, uint8_t>& flags,
                               const char type, bool optional)
{
  if (flags.count(type) > 0)
    return flags.find(type)->second;
  else
  {
    if (!optional)
    {
      NDN_THROW(Error("Required flag type not found!"));
      NDN_LOG_ERROR("Required flag type not found!");
    }
    return 0;
  }
}

void
ServiceDiscovery::producerHandler()
{
  auto& prefix = m_producerState.applicationPrefix;
  NDN_LOG_INFO("Advertising service under Name: " << prefix);
  doUpdate(prefix);
  run();
}

void
ServiceDiscovery::consumerHandler()
{
  NDN_LOG_INFO("Requesting service: " << m_serviceName);
  run();
}

void
ServiceDiscovery::run()
{
  try {
    m_face.processEvents();
  }
  catch (const std::exception& ex)
  {
    NDN_THROW(Error(ex.what()));
    NDN_LOG_ERROR("Face error: " << ex.what());
  }
}

void
ServiceDiscovery::stop()
{
  NDN_LOG_DEBUG("Shutting down face: ");
  m_face.shutdown();
  m_face.getIoService().stop();
}

void
ServiceDiscovery::setInterestFilter(const ndn::Name& name, const bool loopback)
{
  NDN_LOG_INFO("Setting interest filter on: " << name);
  m_face.setInterestFilter(ndn::InterestFilter(name).allowLoopback(false),
                           std::bind(&ServiceDiscovery::processInterest, this, _1, _2),
                           std::bind(&ServiceDiscovery::onRegistrationSuccess, this, _1),
                           std::bind(&ServiceDiscovery::registrationFailed, this, _1));
}

void
ServiceDiscovery::processInterest(const ndn::Name& name, const ndn::Interest& interest)
{

  NDN_LOG_INFO("Interest received: " << interest.getName());

  // check if the interest is for service detail or to update the service
  if (interest.getName().getSubName(0, 2) == NDNSD_RELOAD_PREFIX)
  {
    NDN_LOG_INFO("Receive request to reload service");
    // reload file and update state
    setUpdateProducerState(true);

    // if change is detected, call doUpdate to notify sync about the update
    doUpdate(m_producerState.applicationPrefix);
    // send back the response
    static const std::string content("Update Successful");
    // Create Data packet
    ndn::Data data(interest.getName());
    data.setFreshnessPeriod(1_s);
    data.setContent(reinterpret_cast<const uint8_t*>(content.data()), content.size());

    m_keyChain.sign(data);
    m_face.put(data);
    NDN_LOG_DEBUG("Data sent for reload interest :" << interest.getName());
  }
  else
  {
    sendData(interest.getName());
  }
}

void
ServiceDiscovery::sendData(const ndn::Name& name)
{
  NDN_LOG_INFO("Sending data for: " << name);

  auto timeDiff = ndn::time::system_clock::now() - m_producerState.publishTimestamp;
  auto timeToExpire = ndn::time::duration_cast<ndn::time::seconds>(timeDiff);

  m_serviceStatus = (timeToExpire > m_producerState.serviceLifetime) ? EXPIRED : ACTIVE;

  ndn::Data replyData(name);
  replyData.setFreshnessPeriod(5_s);
  replyData.setContent(wireEncode());
  m_keyChain.sign(replyData);
  m_face.put(replyData);
  NDN_LOG_DEBUG("Data sent for :" << name);
}

void
ServiceDiscovery::expressInterest(const ndn::Name& name)
{
  NDN_LOG_INFO("Sending interest: " << name);
  ndn::Interest interest(name);
  interest.setCanBePrefix(false);
  interest.setMustBeFresh(true); //set true if want data explicit from producer.
  interest.setInterestLifetime(160_ms);

  m_face.expressInterest(interest,
                         bind(&ServiceDiscovery::onData, this, _1, _2),
                         bind(&ServiceDiscovery::onTimeout, this, _1),
                         bind(&ServiceDiscovery::onTimeout, this, _1));
}

void
ServiceDiscovery::onData(const ndn::Interest& interest, const ndn::Data& data)
{
  NDN_LOG_INFO("Data received for: " << interest.getName());
  // modify this section and pass data.getContent to wiredecode
  data.getContent().parse();
  auto consumerReply = wireDecode(data.getContent().get(tlv::DiscoveryData));
  m_discoveryCallback(consumerReply);
  m_counter--;

  // if continuous discovery is unset (i.e. OPTIONAL) consumer will be stopped
    if (m_counter <= 0 && m_contDiscovery == OPTIONAL)
      stop();
}

void
ServiceDiscovery::onTimeout(const ndn::Interest& interest)
{
  if (m_appType == CONSUMER)
  {
    m_counter--;
  }
  NDN_LOG_INFO("Interest: " << interest.getName() << "timed out");
}

void
ServiceDiscovery::registrationFailed(const ndn::Name& name)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix " << name << " in local hub's daemon");
}

void
ServiceDiscovery::onRegistrationSuccess(const ndn::Name& name)
{
  NDN_LOG_DEBUG("Successfully registered prefix: " << name);
}

void
ServiceDiscovery::doUpdate(const ndn::Name& prefix)
{
  m_syncAdapter.publishUpdate(prefix);
  NDN_LOG_INFO("Publish: " << prefix);
}

void
ServiceDiscovery::processSyncUpdate(const std::vector<ndnsd::SyncDataInfo>& updates)
{
  m_counter = updates.size();
  if (m_appType == CONSUMER)
  {
    for (auto item: updates)
    {
      unsigned int seq;
      std::stringstream ss;
      ss << std::hex << item.highSeq;
      ss >> seq;
      auto interest = item.prefix.appendNumber(item.highSeq);
      NDN_LOG_INFO("Fetching data for prefix:" << interest);
      expressInterest(interest);
    }
  }
  else
  {
    Reply consumerReply;
    for (auto item: updates)
    {
      consumerReply.serviceDetails.insert(std::pair<std::string, std::string>
                                          ("prefix", item.prefix.toUri()));
      // Service is just published, so the status returned to producer will be active
      consumerReply.status = ACTIVE;
      m_discoveryCallback(consumerReply);
    }
    
  }
}

template<ndn::encoding::Tag TAG>
size_t
ServiceDiscovery::wireEncode(ndn::EncodingImpl<TAG>& encoder, const std::string& info) const
{
  size_t totalLength = 0;
  totalLength += prependStringBlock(encoder, tlv::ServiceInfo, info);
  totalLength += prependNonNegativeIntegerBlock(encoder, tlv::ServiceStatus, m_serviceStatus);
  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::DiscoveryData);

  return totalLength;
}

const ndn::Block&
ServiceDiscovery::wireEncode()
{
  if (m_wire.hasWire())
  {
    // check if status has changed or not
    ndn::Block wire(m_wire);
    wire.parse();
    auto it = wire.elements_begin();
    if (it != wire.elements_end() && it->type() == tlv::ServiceStatus)
    {
      NDN_LOG_DEBUG("No change in Block content, returning old Block");
      if (ndn::readNonNegativeInteger(*it) == m_serviceStatus)
        return m_wire;
    }
    m_wire.reset();
  }

  // |service-name|<applicationPrefix>|<key>|<val>|<key>|<val>|...and so on
  std::string dataContent = "service-name";
  dataContent += "|";
  dataContent += m_producerState.applicationPrefix.toUri();

  for (auto const& item : m_producerState.serviceMetaInfo)
  {
    dataContent += "|";
    dataContent += item.first;
    dataContent += "|";
    dataContent += item.second;
  }

  ndn::EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator, dataContent);

  ndn::EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer, dataContent);
  m_wire = buffer.block();

  return m_wire;
}

std::map<std::string, std::string>
processData(std::string reply)
{
  std::map<std::string, std::string> keyVal;
  std::vector<std::string> items;
  boost::split(items, reply, boost::is_any_of("|"));
  for (size_t i = 0; i < items.size(); i += 2)
  {
    keyVal.insert(std::pair<std::string, std::string>(items[i], items[i+1]));
  }
  return keyVal;
}

Reply
wireDecode(const ndn::Block& wire)
{
  Reply consumerReply;
  auto blockType = wire.type();

  if (wire.type() != tlv::DiscoveryData)
  {
    NDN_LOG_ERROR("Expected DiscoveryData Block, but Block is of type: #"
                   << ndn::to_string(blockType));
  }

  wire.parse();
  ndn::Block::element_const_iterator it = wire.elements_begin();

  if (it != wire.elements_end() && it->type() == tlv::ServiceStatus) {
    consumerReply.status = ndn::readNonNegativeInteger(*it);
    ++it;
  }
  else {
    NDN_LOG_DEBUG("Service status is missing");
  }

  if (it != wire.elements_end() && it->type() == tlv::ServiceInfo) {
    auto serviceMetaInfo = readString(*it);
    consumerReply.serviceDetails = processData(readString(*it));
  }
  else {
    NDN_LOG_DEBUG("Service information not available");
  }

  return consumerReply;
}

} // namespace discovery
} // namespace ndnsd
