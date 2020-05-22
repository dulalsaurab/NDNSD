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

#include<iostream>
#include "ndnsd/discovery/service-discovery.hpp"
#include <list>

class Consumer
{
public:
  Consumer(const ndn::Name& serviceName, const std::map<char, uint8_t>& pFlags)
   : m_serviceDiscovery(serviceName, pFlags, std::bind(&Consumer::processCallback, this, _1))
  {
  }

  void
  execute ()
  {
    m_serviceDiscovery.consumerHandler();
  }

private:
  void
  processCallback(const ndnsd::discovery::Reply& callback)
  {
    auto abc = (callback.status == ndnsd::discovery::ACTIVE)? "ACTIVE": "EXPIRED";
    std::cout << "Name: " << callback.serviceMetaInfo << std::endl;
    // std::cout << "Name: " << callback.serviceName << "\n"
    //           << "Status: " << abc << "\n"
    //           << "Info: " << callback.serviceInfo << "\n" << std::endl;
  }

private:
  ndnsd::discovery::ServiceDiscovery m_serviceDiscovery;

};

int
main(int argc, char* argv[])
{
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <service-name> "
              << std::endl;
    return 1;
  }

  std::map<char, uint8_t> flags;
  flags.insert(std::pair<char, uint8_t>('p', ndnsd::SYNC_PROTOCOL_CHRONOSYNC)); //protocol choice
  flags.insert(std::pair<char, uint8_t>('t', ndnsd::discovery::CONSUMER)); //c: consumer - 0, p:producer - 1

  try {
    Consumer consumer(argv[1], flags);
    consumer.execute();
  }
  catch (const std::exception& e) {
  }
}