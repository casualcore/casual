//!
//! casual_logger.cpp
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!

#include "common/log.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/chronology.h"
#include "common/server_context.h"
#include "common/transaction_context.h"
#include "common/algorithm.h"
#include "common/string.h"

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
                        << '|' << common::environment::domain::name()
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
               logger_buffer* bufferFactory()
               {
                  static logger_buffer result( category);
                  return &result;
               }

               struct buffer_holder
               {
                  std::function< logger_buffer*()> factory;
               };


               const buffer_holder& getBuffer( log::category::Type category)
               {
                  static const std::map< log::category::Type, buffer_holder> buffers{
                     { log::category::Type::casual_debug, { bufferFactory< log::category::Type::casual_debug>}},
                     { log::category::Type::casual_trace, { bufferFactory< log::category::Type::casual_trace>}},
                     { log::category::Type::casual_transaction, { bufferFactory< log::category::Type::casual_transaction>}},

                     { log::category::Type::debug, { bufferFactory< log::category::Type::debug>}},
                     { log::category::Type::trace, { bufferFactory< log::category::Type::trace>}},
                     { log::category::Type::parameter, { bufferFactory< log::category::Type::parameter>}},
                     { log::category::Type::information, { bufferFactory< log::category::Type::information>}},
                     { log::category::Type::warning, { bufferFactory< log::category::Type::warning>}},
                     { log::category::Type::error, { bufferFactory< log::category::Type::error>}},
                  };

                  return buffers.at( category);
               }


               logger_buffer* getActiveBuffer( log::category::Type category)
               {
                  auto environment = common::string::split( common::environment::variable::get( "CASUAL_LOG"), ',');

                  auto found = range::find( range::make( environment), log::category::name( category));

                  if( ! found.empty())
                  {
                     //std::cerr << std::to_string( process::id()) + " - " + log::category::name( category) << std::endl;
                     return getBuffer( category).factory();
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
                  case Type::casual_debug: return "casual.debug"; break;
                  case Type::casual_trace: return "casual.trace"; break;
                  case Type::casual_transaction: return "casual.transaction"; break;

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

         namespace internal
         {
            std::ostream debug{ local::getActiveBuffer( category::Type::casual_debug)};

            std::ostream trace{ local::getActiveBuffer( category::Type::casual_trace)};

            std::ostream transaction{ local::getActiveBuffer( category::Type::casual_transaction)};

         } // internal

         std::ostream debug{ local::getActiveBuffer( category::Type::debug)};

         std::ostream trace{ local::getActiveBuffer( category::Type::trace)};

         std::ostream parameter{ local::getActiveBuffer( category::Type::parameter)};

         std::ostream information{ local::getActiveBuffer( category::Type::information)};

         std::ostream warning{ local::getActiveBuffer( category::Type::warning)};

         //
         // Always on
         //
         std::ostream error{ local::getBuffer( log::category::Type::error).factory()};


         bool active( category::Type category)
         {
            return local::getActiveBuffer( category) != nullptr;
         }

         namespace local
         {
            namespace
            {
               std::ostream& getStream( category::Type category)
               {
                  static std::map< category::Type, std::ostream&> streams{
                     { category::Type::casual_debug, internal::debug },
                     { category::Type::casual_trace, internal::trace },
                     { category::Type::casual_transaction, internal::transaction },
                     { category::Type::debug, debug },
                     { category::Type::trace, trace },
                     { category::Type::parameter, parameter },
                     { category::Type::information, information },
                     { category::Type::warning, warning },
                  };

                  return streams.at( category);
               }
            } //
         } // local


         void activate( category::Type category)
         {
            local::getStream( category).rdbuf( local::getBuffer( category).factory());
         }

         void deactivate( category::Type category)
         {
            local::getStream( category).rdbuf( nullptr);
         }


         void write( category::Type category, const char* message)
         {
            local::getStream( category) << message;
         }

         void write( category::Type category, const std::string& message)
         {
            local::getStream( category) << message;
         }
      } // log

   } // common
} // casual

