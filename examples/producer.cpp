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

#include "ndnsd/discovery/service-discovery.hpp"
#include <ndn-cxx/util/logger.hpp>

#include <iostream>
#include <cstdlib>

// #include <list>

NDN_LOG_INIT(ndnsd.examples.ProducerApp);

inline bool
isFile(const std::string& fileName)
{
  return boost::filesystem::exists(fileName);
}

class Producer
{
public:

  Producer(const std::string& filename, std::string abePolicy, const std::map<char, uint8_t>& pFlags)
    : m_serviceDiscovery(filename, pFlags, abePolicy, std::bind(&Producer::processCallback, this, _1))
  {
  }
  void
  execute ()
  {
    m_serviceDiscovery.producerHandler();
  }

private:
  void
  processCallback(const ndnsd::discovery::Reply& callback)
  {
    NDN_LOG_INFO("Service publish callback received");
    auto status = (callback.status == ndnsd::discovery::ACTIVE)? "ACTIVE": "EXPIRED";
    NDN_LOG_INFO("Status: " << status);
    for (auto& item : callback.serviceDetails)
    {
      NDN_LOG_INFO("Callback: " << item.first << ":" << item.second);
    }
  }

private:
  ndnsd::discovery::ServiceDiscovery m_serviceDiscovery;
};

int
main(int argc, char* argv[])
{
  int syncProtocol = ndnsd::SYNC_PROTOCOL_PSYNC;
  // this is causing seg fault, need to look.
  // if (argc > 2)
  //   syncProtocol = atoi(argv[2]);

  std::map<char, uint8_t> flags;
  std::string abePolicy;
  flags.insert(std::pair<char, uint8_t>('p', syncProtocol)); //protocol choice
  flags.insert(std::pair<char, uint8_t>('t', ndnsd::discovery::PRODUCER)); //type producer: 1

  try {
    NDN_LOG_INFO("Starting producer application: " << "with abe policy: " << argv[2]);
    Producer producer(argv[1], argv[2], flags);
    producer.execute();
  }
  catch (const std::exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    NDN_LOG_ERROR("Cannot execute producer, try again later: " << e.what());
  }
}
