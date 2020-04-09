//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/log/stream.h"

#include "common/environment.h"
#include "casual/platform.h"
#include "common/process.h"
#include "common/chronology.h"
#include "common/server/context.h"
#include "common/transaction/context.h"
#include "common/service/call/context.h"
#include "common/execution.h"
#include "common/algorithm.h"
#include "common/string.h"
#include "common/domain.h"
#include "common/instance.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <fstream>
#include <map>
#include <iostream>
#include <memory>
#include <thread>

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

                  template< typename M>
                  void log( const std::string& category, M&& message)
                  {
                     m_output << platform::time::clock::type::now()
                        << '|' << common::domain::identity().name
                        << '|' << execution::id()
                        << '|' << process::id()
                        << '|' << std::this_thread::get_id()
                        << '|' << m_alias
                        << '|' << transaction::Context::instance().current().trid
                        << '|' << execution::service::parent::name()
                        << '|' << execution::service::name()
                        << '|' << category
                        << '|' << message
                        << std::endl; // we need to flush the line.
                  }

                  void reopen() 
                  {
                     m_output = File::open();
                  }

               private:
                  File() : m_output{ File::open()} {}

                  static std::ofstream open()
                  {
                     auto path = common::environment::log::path();

                     // make sure we got the directory
                     common::directory::create( common::directory::name::base( path));                           

                     std::ofstream file{ path, std::ios::app};

                     if( ! file)
                        code::raise::error( code::casual::invalid_path, "failed to open log file: ", path);

                     return file;
                  }

                  std::ofstream m_output;
                  std::string m_alias = instance::alias();
               };

               namespace user
               {
                  std::regex filter()
                  {
                     return std::regex{ common::environment::variable::get( common::environment::variable::name::log::pattern, "error")};
                  }
               } // user

               namespace error
               {
                  const std::regex& filter()
                  {
                     static std::regex filter{ "error"};
                     return filter;
                  }

               } // error

               namespace stream
               {

                  struct buffer : public std::streambuf
                  {
                     using base_type = std::streambuf;

                     buffer( std::string category)
                        : m_category( std::move( category))
                     {

                     }

                     int overflow( int value) override
                     {
                        if( value != base_type::traits_type::eof())
                        {
                           if( value == '\n')
                              log();
                           else
                              m_buffer.push_back( base_type::traits_type::to_char_type( value));
                        }
                        return value;
                     }

                  private:

                     void log()
                     {
                        File::instance().log(  m_category, m_buffer);
                        m_buffer.clear();
                     }

                     using buffer_type = std::string;
                     buffer_type m_buffer;

                     const std::string m_category;
                  };


                  struct holder
                  {
                     holder( std::string category, log::Stream& stream)
                       : category{ std::move( category)}, stream{ stream} {}

                     std::string category;
                     std::reference_wrapper< log::Stream> stream;
                     std::unique_ptr< stream::buffer> buffer;
                  };

                  std::vector< holder>& streams()
                  {
                     static std::vector< holder> streams;
                     return streams;
                  }


                  stream::buffer* activate( holder& holder)
                  {
                     holder.buffer = std::make_unique<stream::buffer>( holder.category);
                     return holder.buffer.get();
                  }

                  stream::buffer* activate( holder& holder, const std::regex& filter)
                  {
                     if( std::regex_match( holder.category, error::filter()) || std::regex_match( holder.category, filter))
                        return activate( holder);
                     else
                        holder.buffer.reset( nullptr);

                     return holder.buffer.get();
                  }

                  namespace find
                  {
                     stream::holder* holder( const std::string& category)
                     {
                        auto is_category = [&]( auto& holder)
                        { 
                           return holder.category == category;
                        };

                        if( auto found = algorithm::find_if( stream::streams(), is_category))
                           return &( *found);

                        return nullptr;
                     }

                  } // find
               } // stream



               namespace registration
               {
                  stream::buffer* stream( log::Stream& stream, std::string category)
                  {
                     if( ! stream::find::holder( category))
                     {
                        stream::streams().emplace_back( std::move( category), stream);

                        return stream::activate( stream::streams().back(), user::filter());
                     }
                     return nullptr;
                  }

               } // registration
            } // unnamed
         } // local

         namespace stream
         {

            namespace thread
            {
               std::mutex Lock::m_mutex;

            } // thread


            Stream& get( const std::string& category)
            {
               if( auto holder = local::stream::find::holder( category))
                  return holder->stream;

               static Stream inactive;
               return inactive;
            }


            bool active( const std::string& category)
            {
               return static_cast< bool>( stream::get( category));
            }


            void activate( const std::string& category)
            {
               auto holder = local::stream::find::holder( category);

               if( holder && ! holder->stream.get())
                  holder->stream.get().rdbuf( local::stream::activate( *holder));
            }

            void deactivate( const std::string& category)
            {
               auto holder = local::stream::find::holder( category);

               if( holder)
               {
                  holder->stream.get().rdbuf( nullptr);
                  holder->buffer.reset( nullptr);
               }
            }

            void write( const std::string& category, const std::string& message)
            {
               thread::Lock lock;
               local::File::instance().log( category, message);
            }

            void reopen()
            {
               local::File::instance().reopen(); 
            }
         } // stream


         Stream::Stream( std::string category)
           : std::ostream{ local::registration::stream( *this, std::move( category))}
         {

         }

         Stream::Stream() : std::ostream{ nullptr} {}


      } // log
   } // common
} // casual


