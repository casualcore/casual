/*
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */
#include "traffic/log/file.h"

#include "common/trace.h"
#include "common/environment.h"
#include "common/chronology.h"


#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>

//
// TODO: Use casual exception
//
#include <stdexcept>

namespace casual
{
namespace traffic
{
namespace log
{

namespace local
{
namespace
{
   std::pair<std::string, std::string> time_divider(const common::platform::time_point& timepoint)
   {
      auto time_count = std::chrono::time_point_cast<std::chrono::nanoseconds>(timepoint).time_since_epoch().count();
      std::string text = std::to_string( time_count);
      return std::make_pair( text.substr(0,10), text.substr(10));
   }
}
}

	File::File( std::ofstream& stream) : m_stream{stream}
	{
		static const std::string cMethodname("File::File(...)");
		common::Trace trace(cMethodname);
	}

	File::~File( )
	{
		static const std::string cMethodname("File::~File");
		common::Trace trace(cMethodname);
	}

	Transaction::Transaction(File& file) : m_file( file)
	{
		static const std::string cMethodname("Transaction::Transaction()");
		common::Trace trace(cMethodname);
	}

	Transaction::~Transaction()
	{
		static const std::string cMethodname("Transaction::~Transaction()");
		common::Trace trace(cMethodname);
	}

	void File::store( const common::message::traffic::monitor::Notify& message)
	{
		static const std::string cMethodname("File::store");
		common::Trace trace(cMethodname);

		std::pair<std::string, std::string> start = local::time_divider( message.start);
		std::pair<std::string, std::string> end = local::time_divider( message.end);
		m_stream << "@" << message.service << " ";
		m_stream << message.pid << "       ";
		m_stream << start.first << "   " << start.second << "    ";
		m_stream << end.first << "   " << end.second << "   ";
		m_stream << std::endl;
	}

}
}
}
void operator>>( const casual::common::message::traffic::monitor::Notify& message, casual::traffic::log::File& file)
{
	file.store( message);
}


