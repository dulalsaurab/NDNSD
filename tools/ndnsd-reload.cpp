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

// #define _GNU_SOURCE

#include<iostream>

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

NDN_LOG_INIT(ndnsd.Tools.Reload);

const char* NDNSD_RELOAD_PREFIX = "/ndnsd/reload";
const int DEFAULT_RELOAD_COUNT = -1234;
const int ONE_TIME_RELOAD = 1;


static void
usage(const boost::program_options::options_description& options)
{
  std::cout << "Usage: ndnsd-reload [options] \n"
            << options;
   exit(2);
}

class UpdateState
{
public:

  UpdateState(int threshold, ndn::time::milliseconds reloadInterval, int reloadCount)
  : m_threshold(threshold)
  , m_reloadCount(reloadCount)
  , m_reloadInterval(reloadInterval)
  , m_scheduler(m_face.getIoService())
  {
    expressInterest();
  }

  void
  expressInterest()
  {
    ndn::Name reloadPrefix(NDNSD_RELOAD_PREFIX);
    // append timestamp
    reloadPrefix.append(boost::lexical_cast<std::string>(ndn::time::system_clock::now()));
    ndn::Interest interest(reloadPrefix);
    interest.setCanBePrefix(false);
    interest.setMustBeFresh(true);
    
    NDN_LOG_INFO("Sending reload interest: "<< interest);
    m_face.expressInterest(interest,
                           ndn::bind(&UpdateState::onData, this, _1, _2),
                           ndn::bind(&UpdateState::onTimeout, this, _1),
                           ndn::bind(&UpdateState::onTimeout, this, _1));
  }

  void
  run()
  {
    try
    {
      m_face.processEvents();
    }
    catch (const std::exception& ex)
    {
      std::cerr << ex.what() << std::endl;
      NDN_LOG_ERROR("Face error: " << ex.what());
    }
    expressInterest();
  }

private:
  void
  onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    NDN_LOG_INFO("Update Successful" << data);
    // exit application
    if (m_reloadCount == DEFAULT_RELOAD_COUNT)
    {
      m_nextPingEvent = m_scheduler.schedule(m_reloadInterval, [this] { expressInterest();});
    }
    else
    {
      m_reloadCount--;
      if(m_reloadCount <= 1)
        exit(0);
      m_nextPingEvent = m_scheduler.schedule(m_reloadInterval, [this] { expressInterest();});
    }
  }

  void onTimeout(const ndn::Interest& interest)
  {
    if (m_threshold < 0)
    {
      // we have reached maximum retry
      NDN_LOG_INFO("Update failed, please try again later");
      exit(0);
    }
    expressInterest();
    m_threshold--;
  }

private:
  int m_threshold;
  ndn::Face m_face;
  int m_reloadCount;
  ndn::time::milliseconds m_reloadInterval;
  ndn::Scheduler m_scheduler;
  ndn::scheduler::ScopedEventId m_nextPingEvent;
};

void
raiseError(const std::string& message)
{
  std::cerr << message << std::endl;
  NDN_LOG_ERROR(message);
}

int
main (int argc, char* argv[])
{
  int interval;
  int nPings = -1;
  ndn::time::milliseconds reloadInterval(0);

  namespace po = boost::program_options;
  po::options_description visibleOptDesc("Options");

  visibleOptDesc.add_options()
    ("help,h",      "print this message and exit")
    ("count,c", po::value<int>(&nPings), "number of reload to sent")
    ("interval,i",  po::value<int>(&interval), "reload interval, in milliseconds")
  ;

  try
  {
    po::variables_map optVm;
    po::store(po::parse_command_line(argc, argv, visibleOptDesc), optVm);
    po::notify(optVm);

    if (optVm.count("count") && optVm.count("interval"))
    {
      if (nPings <=0 || interval <= 0) {
          raiseError("ERROR: Reload and interval must be positive");
          usage(visibleOptDesc);
        }
      reloadInterval  = ndn::time::milliseconds(interval);
    }
    else if (!optVm.count("count") && optVm.count("interval")) 
    {
      if (interval <= 0) {
        raiseError("ERROR: Interval must be positive");
        usage(visibleOptDesc);
      }
      nPings = DEFAULT_RELOAD_COUNT;
      reloadInterval  = ndn::time::milliseconds(interval);
    }
    else if (optVm.count("count") && !optVm.count("interval"))
    {
      if (nPings > 0) {
        raiseError("ERROR: Interval is required when 'c' positive");
        usage(visibleOptDesc);
      }
      NDN_LOG_INFO("'c' must positive, processing for one time reload");
      nPings = ONE_TIME_RELOAD;
      reloadInterval = ndn::time::milliseconds(1000);
    }
    else
    {
      nPings = ONE_TIME_RELOAD;
      //when nPings in 1, this won't reloadInterval doesn't matter
      // TODO: eliminate reloadInterval when nPings is 1
      reloadInterval = ndn::time::milliseconds(1000);
    }
  }
  catch (const po::error& e) {
    raiseError(e.what());
  }

  int threshold = 2;
  UpdateState us(threshold, reloadInterval, nPings);
  us.run();
}
