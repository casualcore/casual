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
#include "common/server/context.h"
#include "common/transaction/context.h"
#include "common/call/context.h"
#include "common/algorithm.h"
#include "common/string.h"

//
// std
//
#include <fstream>
#include <map>
#include <iostream>

#include <thread>



namespace casual
{
   namespace common
   {
      namespace log
      {


         namespace thread
         {
            std::mutex Safe::m_mutex;

         } // thread

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
                     const std::string basename{ file::name::base( process::path())};

                     // We don't need this - thread::Safe proxy
                     //std::lock_guard< std::mutex> lock( m_streamMutex);

                     m_output << chronology::local()
                        << '|' << environment::domain::name()
                        << '|' << call::Context::instance().execution()
                        << '|' << transaction::Context::instance().current().trid
                        << '|' << process::id()
                        << '|' << std::this_thread::get_id()
                        << '|' << basename
                        << "|" << category
                        << '|' << call::Context::instance().service()
                        << "|" << message << std::endl;
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
                        std::cerr << process::path() << " - Could not open log-file: " << logfileName << std::endl;
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
                     { log::category::Type::casual_ipc, { bufferFactory< log::category::Type::casual_ipc>}},
                     { log::category::Type::casual_queue, { bufferFactory< log::category::Type::casual_queue>}},
                     { log::category::Type::casual_buffer, { bufferFactory< log::category::Type::casual_buffer>}},

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
                  std::vector< std::string> environment;

                  range::transform(
                        common::string::split( common::environment::variable::get( "CASUAL_LOG", ""), ','),
                        environment,
                        string::trim);


                  auto found = range::find( environment, log::category::name( category));

                  if( ! found.empty())
                  {
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
                  case Type::casual_ipc: return "casual.ipc"; break;
                  case Type::casual_queue: return "casual.queue"; break;
                  case Type::casual_buffer: return "casual.buffer"; break;

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

            internal::Stream debug{ local::getActiveBuffer( category::Type::casual_debug)};

            internal::Stream trace{ local::getActiveBuffer( category::Type::casual_trace)};

            internal::Stream transaction{ local::getActiveBuffer( category::Type::casual_transaction)};

            internal::Stream ipc{ local::getActiveBuffer( category::Type::casual_ipc)};

            internal::Stream queue{ local::getActiveBuffer( category::Type::casual_queue)};

            internal::Stream buffer{ local::getActiveBuffer( category::Type::casual_buffer)};


         } // internal

         internal::Stream debug{ local::getActiveBuffer( category::Type::debug)};

         internal::Stream trace{ local::getActiveBuffer( category::Type::trace)};

         internal::Stream parameter{ local::getActiveBuffer( category::Type::parameter)};

         internal::Stream information{ local::getActiveBuffer( category::Type::information)};

         internal::Stream warning{ local::getActiveBuffer( category::Type::warning)};

         //
         // Always on
         //
         internal::Stream error{ local::getBuffer( log::category::Type::error).factory()};




         namespace local
         {
            namespace
            {
               std::ostream& stream( category::Type category)
               {
                  static std::map< category::Type, std::ostream&> streams{
                     { category::Type::casual_debug, internal::debug },
                     { category::Type::casual_trace, internal::trace },
                     { category::Type::casual_transaction, internal::transaction },
                     { category::Type::casual_queue, internal::queue },
                     { category::Type::casual_buffer, internal::buffer },
                     { category::Type::debug, debug },
                     { category::Type::trace, trace },
                     { category::Type::parameter, parameter },
                     { category::Type::information, information },
                     { category::Type::warning, warning },
                  };

                  return streams.at( category);
               }
            } // <unnamed>
         } // local


         bool active( category::Type category)
         {
            return local::getActiveBuffer( category) != nullptr;
         }


         void activate( category::Type category)
         {
            local::stream( category).rdbuf( local::getBuffer( category).factory());
         }

         void deactivate( category::Type category)
         {
            local::stream( category).rdbuf( nullptr);
         }




         void write( category::Type category, const char* message)
         {
            local::stream( category) << message;
         }

         void write( category::Type category, const std::string& message)
         {
            local::stream( category) << message;
         }
      } // log

   } // common
} // casual

