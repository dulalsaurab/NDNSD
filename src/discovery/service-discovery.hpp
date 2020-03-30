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

#include <PSync/full-producer.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>

using namespace ndn::time_literals;

class ServiceDiscovery
{

public:
  // // for consumer
  // ServiceDiscovery(const std::vector<std::string>& services, const std::list<char* >& flags);
  
  // for producer
  ServiceDiscovery(const ndn::Name& syncPrefix, const std::string& userPrefix);

  void
  run();

private:
  void 
  doUpdate(const ndn::Name& prefix);

  void
  processSyncUpdate(const std::vector<psync::MissingDataInfo>& updates);
   
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;

  // psync::FullProducer m_fullProducer;

  int m_numDataStreams;
  uint64_t m_maxNumPublish;

  ndn::random::RandomNumberEngine& m_rng;
  std::uniform_int_distribution<> m_rangeUniformRandom;

};

#endif // NDNSD_SERVICE_DISCOVERY_HPP