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
#include "common/file.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "casual/assert.h"

#include <fstream>
#include <map>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>

namespace casual
{
   namespace common::log
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
               //! invoked and set. We cant instantiate the File singleton just
               //! to set the state from within a signal-handler.
               //! Hence, this has to be a 'global' outside `File`.
               std::atomic< bool> reopen{ false};
            } // global::file

            namespace file
            {
               struct Output : common::file::Output
               {
                  Output( std::filesystem::path path) 
                     : common::file::Output{ create_parent( std::move( path)), std::ios::app}
                  {}

                  Output& operator = ( Output&&) = default;

                  void reopen()
                  {
                     close();
                     open( path(), std::ios::app);
                     if( ! *this)
                        code::raise::error( code::casual::invalid_path, "failed to reopen log file: ", path());
                  }

               private:

                  inline std::filesystem::path create_parent( std::filesystem::path path)
                  {
                     directory::create( path.parent_path());
                     return path;
                  }
               };
               static_assert( std::is_move_assignable_v< Output>);
            } // file

            struct File
            {
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

               void relocate( std::filesystem::path path)
               {
                  log( "casual.log", string::compose( "relocate logfile to: ", path));
                  m_output = file::Output{ std::move( path)};
               }

               inline auto& path() const noexcept { return m_output.path();}

            private:
               File() : m_output( common::environment::log::path())
               {}

               void reopen() 
               {
                  local::global::file::reopen.store( false);
                  m_output.reopen();

                  log( "casual.log", string::compose( "logfile reopened: ", m_output));
               }

               file::Output m_output;
               std::string m_alias = instance::alias();
            };

            namespace predicate
            {
               namespace detail
               {
                  auto regex( std::string_view value)
                  {
                     return std::regex{ std::begin( value), std::end( value)};
                  }

                  auto mandatory()
                  {
                     return std::regex{ "error"};
                  }
               } // detail

               auto expression( std::string_view expression)
               {
                  return [ filter = detail::regex( expression)]( auto& value) noexcept
                  {
                     return std::regex_match( value.category(), filter);
                  };
               }

               auto mandatory() noexcept
               {
                  return [ filter = detail::mandatory()]( auto& value) noexcept
                  {
                     return std::regex_match( value.category(), filter);
                  };
               }

               auto user() noexcept
               {
                  return expression( common::environment::variable::get( common::environment::variable::name::log::pattern, "error"));
               }
            } // predicate

            namespace stream
            {

               struct buffer : std::streambuf
               {
                  using base_type = std::streambuf;

                  buffer( std::string category)
                     : m_category( std::move( category))
                  {}

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

                  const auto& category() const noexcept { return m_category;}
                  
                  auto extract() { return std::move( m_category);}

               private:

                  void log()
                  {
                     File::instance().log( m_category, m_buffer);
                     m_buffer.clear();
                  }
                  std::string m_buffer;
                  std::string m_category;
               };


               struct holder
               {
                  holder( std::string category, log::Stream& stream)
                     : m_stream{ &stream}, m_category{ std::move( category)} {}

                  auto& category() const noexcept
                  {
                     if( m_buffer)
                        return m_buffer->category();

                     return m_category;
                  }

                  void activate() noexcept
                  {
                     if( m_buffer)
                        return;

                     m_buffer = std::make_unique< stream::buffer>( std::move( m_category));
                     m_stream->rdbuf( m_buffer.get());
                  }

                  void deactivate() noexcept
                  {
                     if( ! m_buffer)
                        return;

                     m_stream->rdbuf( nullptr);
                     m_category = m_buffer->extract();
                     m_buffer = {};
                  }

                  //! if predicate returns true, the holder is activated,
                  //! deactivated otherwise.
                  template< typename P>
                  void toggle( P&& predicate) noexcept
                  {
                     if( predicate( *this))
                        activate();
                     else
                        deactivate();
                  }

                  bool active() const noexcept { return common::predicate::boolean( m_buffer);}

                  log::Stream& stream() noexcept { return *m_stream;}

                  stream::buffer* buffer() noexcept
                  {
                     if( m_buffer)
                        return m_buffer.get();
                     return nullptr;
                  }

                  friend bool operator == ( const holder& lhs, std::string_view rhs) noexcept { return lhs.category() == rhs;}
                  friend bool operator == ( const holder& lhs, const log::Stream* rhs) noexcept { return lhs.m_stream == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE_NAME( category(), "category");
                     CASUAL_SERIALIZE_NAME( active(), "active");
                     CASUAL_SERIALIZE_NAME( m_stream, "address");
                  )

               private:

                  log::Stream* m_stream;
                  std::unique_ptr< stream::buffer> m_buffer;
                  std::string m_category;
               };

               auto activate() noexcept { return []( auto& stream) noexcept { stream.activate();};}
               auto deactivate() noexcept { return []( auto& stream) noexcept { stream.deactivate();};}
               
               template< typename P>
               auto toggle( P&& predicate) noexcept
               {
                  return [ predicate = std::move( predicate)]( auto& stream) noexcept { stream.toggle( predicate);};
               }


            } // stream

            std::vector< stream::holder>& streams() noexcept
            {
               static std::vector< stream::holder> streams;
               return streams;
            }

            stream::buffer* registration( log::Stream& stream, std::string category)
            {
               assert( ! category.empty());
               if( auto found = algorithm::find( local::streams(), category))
                  casual::terminate( "log category ", std::quoted( category), " already registered - stream: ", *found, " - all streams: ", local::streams());

               // make sure we register stream, and possibly activate the stream.
               auto& handle = local::streams().emplace_back( std::move( category), stream);
               handle.toggle( common::predicate::disjunction( predicate::mandatory(), predicate::user()));
               return handle.buffer();
            }


         } // unnamed
      } // local

      namespace stream
      {
         namespace thread
         {
            std::mutex Lock::m_mutex;
         } // thread

         Stream& get( std::string_view category)
         {
            if( auto found = algorithm::find( local::streams(), category))
               return found->stream();

            static Stream inactive;
            return inactive;
         }

         bool active( std::string_view category) noexcept
         {
            if( auto found = algorithm::find( local::streams(), category))
               return found->active();
            return false;
         }

         void activate( std::string_view expression) noexcept
         {
            algorithm::for_each_if( local::streams(), local::stream::activate(), local::predicate::expression( expression));
         }

         void deactivate( std::string_view expression) noexcept
         {
            algorithm::for_each_if( local::streams(), local::stream::deactivate(), 
               predicate::conjunction( local::predicate::expression( expression), predicate::negate( local::predicate::mandatory())));
         }

         void write( std::string_view category, const std::string& message)
         {
            thread::Lock lock;
            local::File::instance().log( category, message);
         }

         void reopen()
         {
            local::global::file::reopen.store( true);
         }

         const std::filesystem::path& path() noexcept
         {
            return local::File::instance().path();
         }

         void configure( const Configure& configure)
         {
            if( configure.expression.inclusive)
            {
               algorithm::for_each( local::streams(), local::stream::toggle( 
                  predicate::disjunction( 
                     local::predicate::expression( *configure.expression.inclusive),
                     local::predicate::mandatory())));
            }

            if( configure.path)
               local::File::instance().relocate( std::move( *configure.path));
         }

      } // stream


      Stream::Stream( std::string category) : std::ostream{ local::registration( *this, std::move( category))}
      {}

      Stream::~Stream()
      {
         if( auto found = algorithm::find( local::streams(), this))
            found->deactivate();
      }

      Stream::Stream() : std::ostream{ nullptr} {}

   } // common::log
} // casual


