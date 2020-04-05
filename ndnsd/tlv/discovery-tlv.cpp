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


#include "discovery-tlv.hpp"
#include <iostream>

#include <ndn-cxx/util/ostream-joiner.hpp>

namespace ndnsd {
namespace discovery {

template<ndn::encoding::Tag TAG>
size_t
DiscoveryTLV::wireEncode(ndn::EncodingImpl<TAG>& block) const
{


}

DiscoveryTLV::DiscoveryTLV(const ndn::Block& block)
{
  wireEncode(block);
}


const ndn::Block&
DiscoveryTLV::wireEncode()
{

}

void
DiscoveryTLV::wireDecode(const ndn::Block& wire)
{

}

} // namespace discovery
} // namespace ndnsd