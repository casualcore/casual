//!
//! casual_logger.cpp
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!

#include "common/log.h"
#include "common/environment.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/chronology.h"
#include "common/server_context.h"
#include "common/transaction_context.h"

//
// std
//
#include <fstream>
#include <map>
#include <iostream>

#include <mutex>



namespace casual
{
   namespace common
   {
      namespace log
      {

         namespace local
         {
            namespace
            {

               class File
               {
               public:
                  static File& instance()
                  {
                     static File singleton;
                     return singleton;
                  }

                  void log( const std::string& category, const std::string& message)
                  {
                     static const std::string basename{ common::file::basename( common::environment::file::executable())};

                     std::lock_guard< std::mutex> lock( m_streamMutex);

                     m_output << common::chronology::local()
                        << '|' << common::environment::getDomainName()
                        << '|' << common::calling::Context::instance().callId().string()
                        << '|' << common::transaction::Context::instance().currentTransaction().xid.stringGlobal()
                        << '|' << common::process::id()
                        << '|' << basename
                        << '|' << common::calling::Context::instance().currentService()
                        << "|" << category << "|" <<  message << std::endl;
                  }

               private:
                  File()
                  {
                     open();
                  }

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

                  std::ofstream m_output;
                  std::mutex m_streamMutex;
               };


               class logger_buffer : public std::streambuf
               {
               public:

                  using base_type = std::streambuf;

                  logger_buffer( log::category::Type category)
                     : m_category( log::category::name( category))
                  {

                  }

                  int overflow( int value) override
                  {
                     if( value != base_type::traits_type::eof())
                     {
                        if( value == '\n')
                        {
                           log();
                        }
                        else
                        {
                           m_buffer.push_back( base_type::traits_type::to_char_type( value));
                        }

                     }
                     return value;
                  }

               private:

                  void log()
                  {
                     File::instance().log(  m_category, m_buffer);
                     m_buffer.clear();
                  }

                  typedef std::string buffer_type;
                  buffer_type m_buffer;

                  const std::string m_category;
               };


               template< log::category::Type category>
               logger_buffer* buffer()
               {
                  static logger_buffer result( category);
                  return &result;
               }



               std::map< log::category::Type, logger_buffer*> initializeActiveBuffers()
               {
                  std::map< log::category::Type, logger_buffer*> result;

                  // Always error
                  result[ log::category::Type::error] = buffer< log::category::Type::error>();

                  const std::string log = common::environment::variable::get( "CASUAL_LOG");

                  if( log.find( "debug") != std::string::npos)
                     result[ log::category::Type::debug] = buffer< log::category::Type::debug>();
                  if( log.find( "trace") != std::string::npos)
                     result[ log::category::Type::trace] = buffer< log::category::Type::trace>();
                  if( log.find( "parameter") != std::string::npos)
                     result[ log::category::Type::parameter] = buffer< log::category::Type::parameter>();
                  if( log.find( "information") != std::string::npos)
                     result[ log::category::Type::information] = buffer< log::category::Type::information>();
                  if( log.find( "warning") != std::string::npos)
                     result[ log::category::Type::warning] = buffer< log::category::Type::warning>();

                  return result;
               }


               logger_buffer* getBuffer( log::category::Type category)
               {
                  static std::map< log::category::Type, logger_buffer*> buffers = initializeActiveBuffers();

                  logger_buffer* result = nullptr;

                  auto found = buffers.find( category);
                  if( found != std::end( buffers))
                  {
                     return found->second;
                  }
                  return result;
               }



            } // unnamed

         } // local


         namespace category
         {

            const char* name( Type type)
            {
               switch( type)
               {
                  case Type::debug: return "debug"; break;
                  case Type::trace: return "trace"; break;
                  case Type::parameter: return "parameter"; break;
                  case Type::information: return "information"; break;
                  case Type::warning: return "warning"; break;
                  case Type::error: return "error"; break;
                  case Type::none: break;
               }
               return "";
            }

         } // category


         std::ostream debug{ local::getBuffer( category::Type::debug)};

         std::ostream trace{ local::getBuffer( category::Type::trace)};

         std::ostream parameter{ local::getBuffer( category::Type::parameter)};

         std::ostream information{ local::getBuffer( category::Type::information)};

         std::ostream warning{ local::getBuffer( category::Type::warning)};

         std::ostream error{ local::getBuffer( category::Type::error)};


         bool active( category::Type category)
         {
            return local::getBuffer( category) != nullptr;
         }

         void write( const std::string category, const std::string& message)
         {
            local::File::instance().log(  category, message);
         }


         void write( category::Type category, const char* message)
         {
            write( category::name( category), message);
         }

         void write( category::Type category, const std::string& message)
         {
            write( category::name( category), message);
         }
      } // log
   } // common
} // casual

