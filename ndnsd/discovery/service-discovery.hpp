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

#ifndef NDNSD_SERVICE_DISCOVERY_HPP
#define NDNSD_SERVICE_DISCOVERY_HPP

#include "ndnsd/communication/sync-adapter.hpp"

#include "file-processor.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

using namespace ndn::time_literals;

namespace ndnsd {
namespace discovery {
namespace tlv {

enum {
  DiscoveryData = 128,
  ServiceInfo = 129,
  ServiceStatus = 130
};

} // namespace tlv

enum {
  PRODUCER = 0,
  CONSUMER = 1,
};

enum {
  OPTIONAL = 0,
  REQUIRED = 1,
};

// service status
enum {
  EXPIRED = 0,
  ACTIVE = 1,
};

/**
  each node will list on NDNSD_RELOAD_PREFIX, and will update their service once the
  interest is received.
**/
const char* NDNSD_RELOAD_PREFIX = "/ndnsd/reload";

struct Details
{
  /**
    ndn::Name: serviceName: Service producer is willing to publish under.
     syncPrefix will be constructed from the service type
     e.g. serviceType printer, syncPrefix = /<prefix>/discovery/printer
    map: serviceMetaInfo Detail information about the service, key-value map
    ndn::Name applicationPrefix service provider application name
    ndn::time timeStamp When the userPrefix was updated for the last time, default = now()
    ndn::time prefixExpTime Lifetime of the service
  **/

  ndn::Name serviceName;
  ndn::Name applicationPrefix;
  ndn::time::seconds serviceLifetime;
  ndn::time::system_clock::TimePoint publishTimestamp;
  std::map<std::string, std::string> serviceMetaInfo;
};

struct Reply
{
  std::map<std::string, std::string> serviceDetails;
  int status;
};

class Error : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

typedef struct Details Details;
typedef struct Reply Reply;

typedef std::function<void(const Reply& serviceUpdates)> DiscoveryCallback;

class ServiceDiscovery
{

public:

  /**
    @brief constructor for consumer

    Creates a sync prefix from service type, fetches service name from sync,
    iteratively fetches service info for each name, and sends it back to the consumer

    @param serviceName Service consumer is interested on. e.g. = printers
    @param pFlags List of flags, i.e. sync protocol, application type etc
    @param discoveryCallback
  **/
  ServiceDiscovery(const ndn::Name& serviceName,
                   const std::map<char, uint8_t>& pFlags,
                   const DiscoveryCallback& discoveryCallback);

  /**
    @brief Constructor for producer

    Creates a sync prefix from service type, stores service info, sends publication
    updates to sync, and listen on user-prefix to serve incoming requests

    @param filename Info file to load service details, sample below

    required
    {
      serviceName printer
      appPrefix /printer1/service-info
      lifetime 100
    }
    details
    {
      description "Hp Ledger Jet"
      make "2016"
    }
    ; all the keys in required field needs to have value
    ; the details can have as many key-values are needed

    @param pFlags List of flags, i.e. sync protocol, application type etc
  */

  ServiceDiscovery(const std::string& filename,
                   const std::map<char, uint8_t>& pFlags,
                   const DiscoveryCallback& discoveryCallback);

  void
  run();

  /*
    @brief Cancel all pending operations, close connection to forwarder
    and stop the ioService.
  */
  void
  stop();
  /*
    @brief  Handler exposed to producer application. Used to start the
     discovery process
  */
  void
  producerHandler();

  /*
  @brief  Handler exposed to producer application. Used to start the
     discovery process
  */
  void
  consumerHandler();

  uint32_t
  getSyncProtocol() const
  {
    return m_syncProtocol;
  }

  /*
   @brief Process flags send by consumer and producer application.
  */
  uint8_t
  processFalgs(const std::map<char, uint8_t>& flags,
               const char type, bool optional);

  ndn::Name
  makeSyncPrefix(ndn::Name& service);

  std::string
  makeDataContent();

  std::map<std::string, std::string>
  processData(std::string reply);

  friend class ServiceInfoFileProcessor;

private:
  void
  doUpdate(const ndn::Name& prefix);

  void
  setUpdateProducerState(bool update = false);

  void
  processSyncUpdate(const std::vector<ndnsd::SyncDataInfo>& updates);

  void
  processInterest(const ndn::Name& name, const ndn::Interest& interest);

  void
  sendData(const ndn::Name& name);

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

  Reply
  wireDecode(const ndn::Block& wire);

private:
  ndn::Face m_face;
  ndn::KeyChain m_keyChain;

  const std::string m_filename;
  ServiceInfoFileProcessor m_fileProcessor;
  ndn::Name m_serviceName;
  std::map<char, uint8_t> m_Flags;

  Details m_producerState;
  Reply m_consumerReply;

  uint8_t m_appType;
  uint8_t m_counter;

  uint32_t m_syncProtocol;
  uint32_t m_contDiscovery;
  SyncProtocolAdapter m_syncAdapter;
  static const ndn::Name DEFAULT_CONSUMER_ONLY_NAME;
  mutable ndn::Block m_wire;
  DiscoveryCallback m_discoveryCallback;

  bool syncUpdateFlag;

};
} //namespace discovery
} //namespace ndnsd
#endif // NDNSD_SERVICE_DISCOVERY_HPP