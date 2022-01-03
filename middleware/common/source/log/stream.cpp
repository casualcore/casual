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
#include <atomic>

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
               namespace global::file
               {
                  //! This is conceptually tied to `File` below, but we can't 
                  //! keep it in that state, since this variable is set from a 
                  //! signal-handler, and we have no guaranties when it will be 
                  //! invoked and set. We cant instansiate the File singleton just
                  //! to set the state from within a signal-handler.
                  //! Hence, this has to be a 'global' outside `File`.
                  std::atomic< bool> reopen{ false};
               } // global::file

               class File
               {
               public:
                  static File& instance()
                  {
                     static File singleton;
                     return singleton;
                  }

                  template< typename C, typename M>
                  void log( C&& category, M&& message)
                  {
                     if( local::global::file::reopen.load())
                        reopen();

                     common::stream::write( m_output, platform::time::clock::type::now(),
                        '|', common::domain::identity().name,
                        '|', execution::id(),
                        '|', process::id(),
                        '|', std::this_thread::get_id(),
                        '|', m_alias,
                        '|', transaction::Context::instance().current().trid,
                        '|', execution::service::parent::name(),
                        '|', execution::service::name(),
                        '|', category,
                        '|', message,
                        '\n');
                        m_output.flush(); // we need to flush the line.
                  }

               private:
                  File() : m_output{ File::open()} {}

                  void reopen() 
                  {
                     m_output = File::open();
                     log( "casual.log", "logfile reopened");
                  }

                  static std::ofstream open()
                  {
                     local::global::file::reopen.store( false);
                     auto path = common::environment::log::path();

                     // make sure we got the directory
                     directory::create( path.parent_path());

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
               local::global::file::reopen.store( true);
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


