//!
//! casual_logger.cpp
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!

#include "common/logger.h"
#include "common/environment.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/chronology.h"
#include "common/server_context.h"

// temp
#include <iostream>
#include <fstream>



namespace casual
{
   namespace common
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

                     void log( const int priority, const std::string& message)
                     {

                        if( ! m_output.good())
                        {
                           open( priority, message);
                           return;
                        }

                        log( m_output, priority, message);

                     }

                     bool active( int priority) const
                     {
                        return ( m_mask & priority) == priority;
                     }

                  private:
                     ~Context()
                     {

                     }

                     Context()
                           : m_mask( 0)
                     {
                        if( common::environment::variable::exists( "CASUAL_LOG"))
                        {
                           const std::string log = common::environment::variable::get( "CASUAL_LOG");

                           if( log.find( "debug") != std::string::npos)
                              m_mask |= common::platform::cLOG_debug;
                           if( log.find( "information") != std::string::npos)
                              m_mask |= common::platform::cLOG_info;
                           if( log.find( "warning") != std::string::npos)
                              m_mask |= common::platform::cLOG_warning;
                        }

                        m_mask |= common::platform::cLOG_error;

                        open();
                     }

                     void log( std::ostream& out, const int priority, const std::string& message)
                     {
                        //syslog( priority, "%s - %s", m_prefix.c_str(), message.c_str());

                        out <<
                           common::chronology::local() <<
                           '|' << common::environment::getDomainName() <<
                           '|' << common::calling::Context::instance().callId().string() <<
                           '|' << common::process::id() <<
                           '|' << common::file::basename( common::environment::file::executable()) <<
                           '|' << common::calling::Context::instance().currentService() <<
                           "|";

                        // TODO: Temp while we roll our own...
                        switch( priority)
                        {
                           case common::platform::cLOG_debug:
                           {
                              out << "debug - ";
                              break;
                           }
                           case common::platform::cLOG_info:
                           {
                              out << "information - ";
                              break;
                           }
                           case common::platform::cLOG_warning:
                           {
                              out << "warning - ";
                              break;
                           }
                           case common::platform::cLOG_error:
                           {
                              out << "error - ";
                              break;
                           }
                        }

                        out << message << std::endl;
                     }


                     //
                     // Open log
                     //
                     bool open()
                     {
                        static const std::string logfileName = common::environment::directory::domain() + "/casual.log";

                        m_output.open( logfileName, std::ios::app | std::ios::out);

                        if( m_output.fail())
                        {
                           //
                           // We don't want to throw... Or do we?
                           //
                           std::cerr << environment::file::executable() << " - Could not open log-file: " << logfileName << std::endl;
                           //throw exception::xatmi::SystemError( "Could not open the log-file: " + logfileName);
                           return false;
                        }
                        return true;

                     }

                     void open( const int priority, const std::string& message)
                     {
                        if( open())
                        {
                           log( m_output, priority, message);
                        }
                        else
                        {
                           log( std::cerr,  priority, message);
                        }
                     }

                     std::ofstream m_output;
                     int m_mask;

                  };

               }

            }

            Proxy::Proxy( int priority)
                  : m_priority( priority), m_log( local::Context::instance().active( priority))
            {
            }

            //
            // We can't rely on RVO, so we have to release logging-responsibility for
            // rhs.
            //
            Proxy::Proxy( Proxy&& rhs)
               : // TODO: Not implemented in gcc m_message( std::move( rhs.m_message)),
                 m_message( rhs.m_message.str()),
                 m_priority( std::move( rhs.m_priority)),
                 m_log( std::move( rhs.m_log))
            {
               rhs.m_log = false;
            }

            //
            // Will be called when the full expression has completed, and this rvalue
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

         internal::basic_logger< common::platform::cLOG_debug> debug;

         internal::basic_logger< common::platform::cLOG_debug> trace;

         internal::basic_logger< common::platform::cLOG_info> information;

         internal::basic_logger< common::platform::cLOG_warning> warning;

         internal::basic_logger< common::platform::cLOG_error> error;

      } // logger
   } // utility
} // casual

