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

#include "file-processor.hpp"

#include<iostream>

#include <ndn-cxx/util/logger.hpp>

NDN_LOG_INIT(ndnsd.FileProcessor);

namespace ndnsd {
namespace discovery {

ServiceInfoFileProcessor::ServiceInfoFileProcessor(const std::string filename)
 : m_filename(filename)
{
  processFile();
}

// need to implement sanity check, 1. correct file is edited, required section is not modified
// and so on

void
ServiceInfoFileProcessor::processFile()
{
  try
  {
    NDN_LOG_INFO("Reading file: "<< m_filename);
    boost::property_tree::ptree pt;
    read_info(m_filename, pt);
    for (auto& block: pt)
    {
      if (block.first == "required")
      {
        for (auto& requiredElement: block.second)
        {
          const auto& val = requiredElement.second.get_value<std::string >();

          if (requiredElement.first == "serviceName")
          {
            m_serviceName = val;
          }
          if (requiredElement.first == "appPrefix")
          {
            m_applicationPrefix = val;
          }
          if (requiredElement.first == "lifetime")
          {
              uint32_t lifetime = std::stoi(val);
              m_serviceLifeTime = ndn::time::seconds(lifetime);
          }
        }
      }

      if (block.first == "details")
      {
        m_serviceMetaInfo.clear();
        for (auto& details: block.second)
        {
          const auto& key = details.first; //get_value<std::string >();
          const auto& val = details.second.get_value<std::string >();

          NDN_LOG_INFO("Reading file: "<< val);

          m_serviceMetaInfo.insert(std::pair<std::string, std::string>(key, val));
        }
      }
    }
    NDN_LOG_INFO("Successfully updated the file content: ");
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << std::endl;
    NDN_LOG_INFO("Error reading file: " << m_filename);
    throw e;
  }
}

} // namespace discovery
} // namespace ndnsd