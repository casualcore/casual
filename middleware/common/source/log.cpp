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
#include "common/execution.h"
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

                  void log( const std::string& category, const char* message)
                  {
                     const std::string basename{ file::name::base( process::path())};

                     m_output << chronology::local()
                        << '|' << environment::domain::name()
                        << '|' << execution::id()
                        << '|' << transaction::Context::instance().current().trid
                        << '|' << process::id()
                        << '|' << std::this_thread::get_id()
                        << '|' << basename
                        << '|' << execution::parent::service()
                        << '|' << execution::service()
                        << '|' << category
                        << '|' << message << std::endl;
                  }

                  void log( const std::string& category, const std::string& message)
                  {
                     log( category, message.c_str());
                  }

               private:
                  File()
                  {
                     open();
                  }

                  bool open( const std::string& file)
                  {
                     m_output.open( file, std::ios::app | std::ios::out);

                     if( m_output.fail())
                     {
                        //
                        // We don't want to throw... Or do we?
                        //
                        std::cerr << process::path() << " - could not open log-file: " << file << std::endl;
                        return false;
                     }
                     return true;

                  }

                  void open()
                  {
                     if( ! open( common::environment::directory::domain() + "/casual.log"))
                     {
                        std::cerr << "using current directory - ./casual.log" << std::endl;
                        open( "./casual.log");
                     }
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

               namespace buffer
               {
                  std::map< log::category::Type, buffer_holder>& mapping()
                  {
                     static std::map< log::category::Type, buffer_holder> buffers;
                     return buffers;
                  }


                  const buffer_holder& get( log::category::Type category)
                  {
                     return mapping().at( category);
                  }

                  namespace active
                  {
                     logger_buffer* get( log::category::Type category)
                     {
                        std::vector< std::string> environment;

                        range::transform(
                              common::string::split( common::environment::variable::get( "CASUAL_LOG", ""), ','),
                              environment,
                              string::trim);


                        auto found = range::find( environment, log::category::name( category));

                        if( ! found.empty() || ! range::find( environment, "%").empty())
                        {
                           return buffer::get( category).factory();
                        }

                        return nullptr;
                     }
                  } // active
               } // buffer

            } // unnamed

         } // local


         namespace category
         {

            namespace
            {

               std::map< Type, std::string>& mapping()
               {
                  static std::map< Type, std::string> singleton;
                  return singleton;
               }

            } // <unnamed>

            const std::string& name( Type type)
            {
               return mapping().at( type);
            }

         } // category

         namespace stream
         {
            namespace
            {
               std::map< category::Type, internal::Stream*>& mapping()
               {
                  static std::map< category::Type, internal::Stream*> singleton;
                  return singleton;
               }
            } // unnamed

            internal::Stream& get( category::Type category)
            {
               return *mapping().at( category);
            }
         } // stream


         namespace registration
         {
            namespace
            {
               template< category::Type type>
               category::Type logger( std::string name, internal::Stream& stream)
               {
                  category::mapping()[ type] = std::move( name);

                  local::buffer::mapping()[ type] = { local::bufferFactory< type>};

                  stream::mapping()[ type] = &stream;

                  return type;
               }

               template< category::Type type>
               local::logger_buffer* active( std::string name, internal::Stream& stream)
               {
                  return local::buffer::active::get( registration::logger< type>( std::move( name), stream));
               }

            } // <unnamed>

         } // register



         namespace internal
         {

            internal::Stream debug{ registration::active< category::Type::casual_debug>( "casual.debug", debug)};

            internal::Stream trace{ registration::active< category::Type::casual_trace>( "casual.trace", trace)};

            internal::Stream transaction{ registration::active< category::Type::casual_transaction>( "casual.transaction", transaction)};

            internal::Stream gateway{ registration::active< category::Type::casual_gateway>( "casual.gateway", gateway)};

            internal::Stream ipc{ registration::active< category::Type::casual_ipc>( "casual.ipc", ipc)};

            internal::Stream queue{ registration::active< category::Type::casual_queue>( "casual.queue", queue)};

            internal::Stream buffer{ registration::active< category::Type::casual_buffer>( "casual.buffer", buffer)};


         } // internal

         internal::Stream debug{ registration::active< category::Type::debug>( "debug", debug)};

         internal::Stream trace{ registration::active< category::Type::trace>( "trace", trace)};

         internal::Stream parameter{ registration::active< category::Type::parameter>( "parameter", parameter)};

         internal::Stream information{ registration::active< category::Type::information>( "information", information)};

         internal::Stream warning{ registration::active< category::Type::warning>( "warning", warning)};

         //
         // Always on
         //
         internal::Stream error{ local::buffer::get( registration::logger< category::Type::error>( "error", error)).factory()};





         bool active( category::Type category)
         {
            return local::buffer::active::get( category) != nullptr;
         }


         void activate( category::Type category)
         {
            stream::get( category).rdbuf( local::buffer::get( category).factory());
         }

         void deactivate( category::Type category)
         {
            stream::get( category).rdbuf( nullptr);
         }




         void write( category::Type category, const char* message)
         {
            stream::get( category) << message << std::endl;
         }

         void write( category::Type category, const std::string& message)
         {
            stream::get( category) << message << std::endl;
         }

         void write( const std::string& category, const std::string& message)
         {
            thread::Safe guard{ std::cout};
            local::File::instance().log( category, message);
         }
      } // log

   } // common
} // casual

