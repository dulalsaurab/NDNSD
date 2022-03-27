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

#include <iostream>

#include <ndn-cxx/util/logger.hpp>

NDN_LOG_INIT(ndnsd.FileProcessor);

namespace ndnsd {
namespace discovery {

ServiceInfoFileProcessor::ServiceInfoFileProcessor(std::string filename)
 : m_filename(std::move(filename))
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
      namespace pt = boost::property_tree;
      using ConfigSection = boost::property_tree::ptree;
      NDN_LOG_INFO("Reading file: "<< m_filename);
      std::ifstream ifstream(m_filename.c_str());
      ConfigSection section;
      pt::read_info(m_filename, section);
      auto a = section.get_child("service-info-namespace");

      ndn::Name root, scope, type, identifier;
      for (const auto &item: section.get_child("service-info-namespace")) {
          std::string key = item.first;
          ndn::Name value = item.second.get_value<ndn::Name>();
          if (key == "root") {
              root = value;
          } else if (key == "scope") {
              scope = value;
          } else if (key == "type") {
              type = value;
          } else if (key == "identifier") {
              identifier = value;
          }
      }
      m_serviceName.append(root).append(scope).append(type).append(identifier);

      for (const auto &item: section.get_child("required-service-detail")) {
          std::string key = item.first;
          std::string value = item.second.get_value<std::string>();
          if (key == "name") {
              m_applicationPrefix = value;
          } else if (key == "lifetime") {
              uint32_t lifetime = std::stoi(value);
              m_serviceLifeTime = ndn::time::seconds(lifetime);
          }
      }

      if (section.get_child_optional("optional-service-detail")) {
          for (const auto &item: section.get_child("optional-service-detail")) {
              std::string key = item.first;
              std::string value = item.second.get_value<std::string>();

              m_serviceMetaInfo.insert(std::pair<std::string, std::string>(key, value));
          }
      }

      ifstream.close();
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
