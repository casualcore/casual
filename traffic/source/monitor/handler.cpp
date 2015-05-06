/*
 * casual_statisticshandler.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "traffic/monitor/serviceentryvo.h"

#include "common/queue.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/server/handle.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/error.h"
#include "common/log.h"
#include "common/trace.h"
#include "common/environment.h"
#include "common/chronology.h"


#include <vector>
#include <string>
#include <chrono>

//temp
#include <iostream>

#include "traffic/monitor/handler.h"

using namespace casual::common;

namespace casual
{
namespace traffic
{
namespace monitor
{

	namespace handle
	{
		void Notify::operator () ( const message_type& message)
		{
		   trace::internal::Scope trace( "handle::Notify::dispatch");

			message >> m_database;
		}
	}

} // monitor
} // traffic
} // casual

