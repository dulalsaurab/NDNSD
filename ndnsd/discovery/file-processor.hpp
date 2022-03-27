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

#ifndef NDNSD_FILE_PROCESSOR_HPP
#define NDNSD_FILE_PROCESSOR_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

namespace ndnsd {
namespace discovery {

class ServiceInfoFileProcessor
{
public:

  ServiceInfoFileProcessor() = default;
  explicit ServiceInfoFileProcessor(std::string  filename);

  ndn::Name&
  getServiceName()
  {
    return m_serviceName;
  }

  ndn::Name&
  getAppPrefix()
  {
    return m_applicationPrefix;
  }

  ndn::time::seconds
  getServiceLifetime()
  {
    return m_serviceLifeTime;
  }

  std::map<std::string, std::string>&
  getServiceMeta()
  {
    return m_serviceMetaInfo;
  }

  void
  processFile();

private:
  const std::string m_filename;
  ndn::Name m_serviceName;
  ndn::Name m_applicationPrefix;
  std::map<std::string, std::string> m_serviceMetaInfo;
  ndn::time::seconds m_serviceLifeTime{};
};

} // namespace discovery
} // namespace ndnsd

#endif // NDNSD_FILE_PROCESSOR_HPP
