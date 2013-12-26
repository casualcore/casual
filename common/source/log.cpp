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


               logger_buffer* getBuffer( log::category::Type category)
               {
                  switch( category)
                  {
                     case log::category::Type::debug:
                           return buffer< log::category::Type::debug>(); break;
                     case log::category::Type::trace:
                           return buffer< log::category::Type::trace>(); break;
                     case log::category::Type::parameter:
                           return buffer< log::category::Type::parameter>();  break;
                     case log::category::Type::information:
                           return buffer< log::category::Type::information>(); break;
                     case log::category::Type::warning:
                           return buffer< log::category::Type::warning>();  break;
                     case log::category::Type::error:
                        return buffer< log::category::Type::error>();  break;
                  }
                  return nullptr;
               }


               logger_buffer* getActiveBuffer( log::category::Type category)
               {
                  const std::string log = common::environment::variable::get( "CASUAL_LOG");

                  switch( category)
                  {
                     case log::category::Type::debug:
                        if( log.find( "debug") != std::string::npos)
                           return getBuffer( category); break;
                     case log::category::Type::trace:
                        if( log.find( "trace") != std::string::npos)
                           return getBuffer( category); break;
                     case log::category::Type::parameter:
                        if( log.find( "parameter") != std::string::npos)
                           return getBuffer( category); break;
                     case log::category::Type::information:
                        if( log.find( "information") != std::string::npos)
                           return getBuffer( category); break;
                     case log::category::Type::warning:
                        if( log.find( "warning") != std::string::npos)
                           return getBuffer( category); break;
                     case log::category::Type::error:
                        return getBuffer( category); break;
                  }
                  return nullptr;
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
               }
               return "";
            }

         } // category


         std::ostream debug{ local::getActiveBuffer( category::Type::debug)};

         std::ostream trace{ local::getActiveBuffer( category::Type::trace)};

         std::ostream parameter{ local::getActiveBuffer( category::Type::parameter)};

         std::ostream information{ local::getActiveBuffer( category::Type::information)};

         std::ostream warning{ local::getActiveBuffer( category::Type::warning)};

         //
         // Always on
         //
         std::ostream error{ local::buffer< log::category::Type::error>()};


         bool active( category::Type category)
         {
            return local::getActiveBuffer( category) != nullptr;
         }

         namespace local
         {
            namespace
            {
               std::map< category::Type, std::ostream&> initializeStreams()
               {
                  std::map< category::Type, std::ostream&> result;

                  result.emplace( category::Type::debug, debug);
                  result.emplace( category::Type::trace, trace);
                  result.emplace( category::Type::parameter, parameter);
                  result.emplace( category::Type::information, information);
                  result.emplace( category::Type::warning, warning);
                  //result.emplace( category::Type::error, error);

                  return result;
               }

               std::ostream& getStream( category::Type category)
               {
                  static std::map< category::Type, std::ostream&> streams = initializeStreams();

                  return streams.at( category);
               }
            } //
         } // local


         void activate( category::Type category)
         {
            std::ostream& stream = local::getStream( category);
            stream.rdbuf( local::getBuffer( category));
         }

         void deactivate( category::Type category)
         {
            std::ostream& stream = local::getStream( category);
            stream.rdbuf( nullptr);
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

