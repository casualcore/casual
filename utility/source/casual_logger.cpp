//!
//! casual_logger.cpp
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!


#include "casual_logger.h"

#include "casual_utility_environment.h"

#include "casual_utility_platform.h"

#include "casual_exception.h"


// temp
#include <fstream>


namespace casual
{
	namespace logger
	{
		namespace internal
		{
			namespace local
			{
				namespace
				{
					struct Context
					{
						static Context& instance()
						{
							static Context singleton;
							return singleton;
						}

						void log( int priority, const std::string& message)
						{
							//syslog( priority, "casual: %s", message.c_str());
						   m_output << message << std::endl;
						}

					private:
						~Context()
						{
							//closelog();
						}

						Context() : m_domain( utility::environment::getDomainName())
						{
							//
							// Open log
							//
						   m_output.open( ( utility::environment::getRootPath() + "/casual.log").c_str(), std::ios::app | std::ios::out);

						   if( m_output.fail())
						   {
						      throw exception::NotReallySureWhatToNameThisException();
						   }

							//openlog( m_domain.c_str(), LOG_PID, LOG_USER);

						}
						std::ofstream m_output;
						std::string m_domain;
					};


				}

			}


			Proxy::Proxy( int priority) : m_priority( priority), m_log( true) {}

			//
			// We can't rely on RVO, so we have to release logging-responsibility for
			// rhs.
			//
			Proxy::Proxy( const Proxy& rhs) : m_message( rhs.m_message.str()), m_priority( rhs.m_priority), m_log( true)
			{
				rhs.m_log = false;
			}

			//
			// Will be called when the full expression has "run", and this rvalue
			// will be destructed.
			//
			Proxy::~Proxy()
			{
				if( m_log)
				{
					local::Context::instance().log( m_priority, m_message.str());
				}
			}

		} // internal

      internal::basic_logger< utility::platform::cLOG_debug> debug;

      internal::basic_logger< utility::platform::cLOG_warning> warning;

      internal::basic_logger< utility::platform::cLOG_error> error;



	} // logger
} // casual


