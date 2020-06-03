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

/*! \file logger.hpp
 * \brief Define macros and auxiliary functions for logging.
 *
 * This file defines the macros that NDNSD uses for logging
 * messages. An intrepid hacker could replace this system cleanly by
 * providing a system that redefines all of the _LOG_* macros with an
 * arbitrary system, as long as the underlying system accepts strings.
 */

#ifndef NDNSD_LOGGER_HPP
#define NDNSD_LOGGER_HPP

#include <ndn-cxx/util/logger.hpp>

#define INIT_LOGGER(name) NDN_LOG_INIT(ndnsd.name)

#define NDNSD_LOG_TRACE(x) NDN_LOG_TRACE(x)
#define NDNSD_LOG_DEBUG(x) NDN_LOG_DEBUG(x)
#define NDNSD_LOG_INFO(x) NDN_LOG_INFO(x)
#define NDNSD_LOG_WARN(x) NDN_LOG_WARN(x)
#define NDNSD_LOG_ERROR(x) NDN_LOG_ERROR(x)
#define NDNSD_LOG_FATAL(x) NDN_LOG_FATAL(x)

#endif // NDNSD_LOGGER_HPP
