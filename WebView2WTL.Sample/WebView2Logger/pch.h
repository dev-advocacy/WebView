// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#define BOOST_USE_WINAPI_VERSION	0x0601

#include <iostream>
#include <string>
#include <boost/log/utility/ipc/reliable_message_queue.hpp>
#include <boost/log/utility/ipc/object_name.hpp>
#include <boost/log/utility/open_mode.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

#endif //PCH_H
