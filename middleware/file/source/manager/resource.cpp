//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/manager/resource.h"

#include "common/communication/instance.h"

#include "common/string/compose.h"

#include <expected>

namespace casual::file::resource
{
   namespace local
   {
      namespace
      {
         auto temporary( const common::transaction::ID& transaction, const std::filesystem::path& path)
         {
            std::filesystem::path result{ path};
            result += common::string::compose( ".casual.", common::transaction::id::range::global( transaction));
            return result;
         }

         auto pull( auto& content, const auto& value, auto&& projection)
         {
            std::remove_reference_t<decltype( content)> result;
            auto [first, last] = std::ranges::remove( content, value, projection);
            std::ranges::move( first, last, std::back_inserter( result));
            content.erase( first, last);
            return result;
         };

         auto drop( auto& content, const auto& value, auto&& projection)
         {
            auto [first, last] = std::ranges::remove( content, value, projection);
            content.erase( first, last);
         };

         auto tidy( auto& content, auto&& projection)
         {
            std::ranges::sort( content, {}, projection);
            auto [first, last] = std::ranges::unique( content, {}, projection);
            content.erase( first, last);
         }

         auto copy( const auto& content, const auto& value, auto&& projection)
         {
            std::remove_cvref_t<decltype( content)> result;
            std::ranges::copy_if( content, std::back_inserter( result), [&] ( const auto& element) { return value == std::invoke( projection, element);});
            return result;
         }

      } //
   } // local

   namespace local
   {
      namespace 
      {
         namespace detail
         {
            auto reserve( const Request& request)
            {
               auto result = local::temporary( request.trid, request.path);

               //
               // potentially copy existing file
               if( std::filesystem::exists( request.path))
                  std::filesystem::copy_file( request.path, result);

               return result;
            }

            void commit( const Request& request)
            {
               const auto rushes = local::temporary( request.trid, request.path);

               if( std::filesystem::exists( rushes))
                  std::filesystem::rename( rushes, request.path);
               else
                  std::filesystem::remove_all( request.path);
            }

            void rollback( const Request& request)
            {
               const auto rushes = local::temporary( request.trid, request.path);

               if( std::filesystem::exists( rushes))
                  std::filesystem::remove_all( rushes);
            }

            void mitigate( const Request& request)
            {
               rollback( request);
            }

            auto reserve( State& state, const Request& request) -> std::expected<std::filesystem::path, code>
            {
               if( state.runlevel == State::Runlevel::shutdown)
               {
                  common::log::error( common::code::casual::shutdown, "failed to reserve due to shutdown");
                  return std::unexpected( code::error);
               }
   
               if( const auto work = std::ranges::find( state.working, request.path, &Request::path); work != state.working.end())
               {
                  //
                  // requested path is involved
   
                  if( work->trid == request.trid)
                     return temporary( request.trid, request.path);
                  else
                     return std::unexpected( code::busy);
               }
   
               try
               {
                  auto result = reserve( request);
   
                  if( std::ranges::find( state.working, request.trid, &Request::trid) == state.working.end())
                  {
                     //
                     // no request in this transaction seems to be involved earlier
                     common::communication::device::blocking::send( 
                        common::communication::instance::outbound::transaction::manager::device(),
                        common::message::transaction::resource::external::involved::create( request));
                  }
   
                  return result;
               }
               catch( ...)
               {
                  common::log::error( common::exception::capture(), "failed to aqcuire ", request.path);
                  return std::unexpected( code::error);
               }
            }
   
         } // detail

         void reserve( State& state, Request request)
         {
            auto reply = common::message::reverse::type( static_cast<const Reserve&>(request));

            if(auto result = detail::reserve( state, request))
               reply.path = std::move( result.value());
            else
               reply.code = std::move( result.error());
      
            switch( reply.code)
            {
            break; case code::ok:
               // push to work load
               state.working.push_back( std::move( request));
               // send the reply
               state.multiplex.send( request.process.ipc, std::move( reply));
            break; case code::busy:
               if( request.wait)
                  // push to wait list
                  state.pending.push_back( std::move( request));
               else
                  // send the reply
                  state.multiplex.send( request.process.ipc, std::move( reply));
            break; default:
               // just send the reply
               state.multiplex.send( request.process.ipc, std::move( reply));
            }
         }

         auto release( auto& state, const auto& value, auto&& projection, auto&& operation)
         {
            //
            // get hold of the work load for this scenario
            auto working = pull( state.working, value, projection);

            //
            // make sure to handle paths just one time
            tidy( working, &Request::path);

            auto result = common::code::xa::ok;

            for( const auto& work : working)
            {
               try
               {
                  operation( work);
               }
               catch( ...)
               {
                  common::log::error( common::exception::capture(), "failed to finalize ", work.path);
                  result = common::code::xa::resource_error;
               }

               //
               // free related (potentially) pending requests for this path (for real)
               for(auto&& wait : pull( state.pending, work.path, &Request::path))
               {
                  //
                  // start all over
                  reserve( state, std::move( wait));
               }
            }

            return result;
         }
      } // 
   } // local

   void reserve( State& state, const Reserve& request)
   {
      Request work{request};
      work.path = work.path.lexically_normal();
      work.time = std::chrono::system_clock::now();

      local::reserve( state, std::move( work));
   }

   void prepare( State& state, const Prepare& request)
   {
      state.multiplex.send( request.process.ipc, common::message::reverse::type( request));
   }

   void commit( State& state, const Commit& request)
   {
      auto reply = common::message::reverse::type( request);
      reply.resource = request.resource;
      reply.trid = request.trid;

      //
      // pick the requests involved and finalize them
      reply.state = local::release( state, request.trid, &Request::trid, &local::detail::commit);

      //
      // send the reply to the transaction manager
      state.multiplex.send( request.process.ipc, std::move( reply));
   }

   void rollback( State& state, const Rollback& request)
   {
      auto reply = common::message::reverse::type( request);
      reply.resource = request.resource;
      reply.trid = request.trid;

      //
      // pick the requests involved and finalize them
      reply.state = local::release( state, request.trid, &Request::trid, &local::detail::rollback);

      //
      // send the reply to the transaction manager
      state.multiplex.send( request.process.ipc, std::move( reply));
   }

   void mitigate( State& state, const Exit& request)
   {
      //
      // drop potential pending requests
      local::drop( state.pending, request.process, &Request::process);

      //
      // pick the requests involved and finalize them
      local::release( state, request.process, &Request::process, &local::detail::mitigate);
   }

   void shutdown( State& state, const Shutdown& request)
   {
      state.runlevel = State::Runlevel::shutdown;

      for( auto&& request : state.pending)
      {
         //
         // using existing funtionality
         reserve( state, std::move( request));
      }

      //
      // drop all pending requests
      state.pending.clear();
   }

   namespace recovery
   {
      namespace detail
      {
         namespace
         {
            auto recover( State& state, auto gtrids, auto&& operation)
            {
               std::erase_if( gtrids, [&]( const auto& gtrid) 
                  {
                     const auto requests = local::copy( state.working, gtrid, &Request::trid);

                     if( requests.empty())
                     {
                        // not affected
                        return true;
                     }
      
                     // make sure to try all
                     return std::ranges::count_if( 
                        requests, [&] ( const auto& request) 
                        { 
                           return local::release( state, request.trid, &Request::trid, operation) == common::code::xa::ok;
                        }) != std::ssize( requests);
                  });
      
               return gtrids;
            }
         } //
      } // detail

      std::vector< common::transaction::global::ID> commit( State& state, std::vector< common::transaction::global::ID> gtrids)
      {
         return detail::recover( state, std::move( gtrids), &local::detail::commit);
      }

      std::vector< common::transaction::global::ID> rollback( State& state, std::vector< common::transaction::global::ID> gtrids)
      {
         return detail::recover( state, std::move( gtrids), &local::detail::rollback);
      }
   } // recovery

} // casual::file::resource
