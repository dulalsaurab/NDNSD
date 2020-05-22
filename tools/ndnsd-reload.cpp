/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  The University of Memphis
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

#define _GNU_SOURCE

#include<iostream>

#include<ndn-cxx/interest.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

const char* NDNSD_RELOAD_PREFIX = "/ndnsd/reload";

class UpdateState
{
public:

  UpdateState(int threshold)
  : m_threshold(threshold)
  // , m_face(face)
  {
    expressInterest();
  }

  void
  expressInterest()
  {
    ndn::Interest interest(NDNSD_RELOAD_PREFIX);
    interest.setCanBePrefix(false);
    interest.setMustBeFresh(true);

    std::cout << "Sending the reload interest: "<< interest << std::endl;
    m_face.expressInterest(interest,
                           ndn::bind(&UpdateState::onData, this, _1, _2),
                           ndn::bind(&UpdateState::onTimeout, this, _1),
                           ndn::bind(&UpdateState::onTimeout, this, _1));
  }

  void
  run()
  {
    m_face.processEvents();
    expressInterest();
  }

private:
  void
  onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    std::cout << "Update Successful" << data << std::endl;
    // exit application
    exit(0);
  }

  void onTimeout(const ndn::Interest& interest)
  {
    if (m_threshold < 0)
    {
      // we have reached maximum retry
      std::cout << "Update failed, please try again later" << std::endl;
      exit(0);
    }
    expressInterest();
    m_threshold--;
  }

private:
  int m_threshold;
  ndn::Face m_face;
};

int main (int argv, char* argc[])
{

  int threshold = 2;

  // Face face;
  UpdateState us(threshold);
  us.run();

}
