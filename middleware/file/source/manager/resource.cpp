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

      } //
   } // local

   namespace local
   {
      namespace 
      {
         namespace detail
         {
            auto reserve( const Reserve& request)
            {
               auto result = local::temporary( request.trid, request.path);

               if( std::filesystem::exists( request.path))
               {
                  //
                  // copy any existing file
                  std::filesystem::copy_file( request.path, result);
               }

               return result;
            }

            void commit( const Reserve& request)
            {
               const auto rushes = local::temporary( request.trid, request.path);

               if( std::filesystem::exists( rushes))
               {
                  std::filesystem::rename( rushes, request.path);
               }
               else
               {
                  std::filesystem::remove_all( request.path);
               }
            }

            void rollback( const Reserve& request)
            {
               const auto rushes = local::temporary( request.trid, request.path);

               if( std::filesystem::exists( rushes))
               {
                  std::filesystem::remove_all( rushes);
               }
            }

            void mitigate( const Reserve& request)
            {
               rollback( request);
            }
         } // detail


         auto reserve( State& state, const Reserve& request) -> std::expected<std::filesystem::path, code>
         {
            if( state.runlevel == State::Runlevel::shutdown)
            {
               common::log::error( common::exception::capture(), "failed to reserve due to shutdown");
               return std::unexpected( code::error);
            }

            if( const auto work = std::ranges::find( state.working, request.path, &Reserve::path); work != state.working.end())
            {
               //
               // requested path is involved

               if( work->trid == request.trid)
               {
                  return local::temporary( request.trid, request.path);
               }
               else
               {
                  return std::unexpected( code::busy);
               }
            }

            try
            {
               auto result = detail::reserve( request);

               if( std::ranges::find( state.working, request.trid, &Reserve::trid) == state.working.end())
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

      } //
   } // local


   namespace local
   {
      namespace
      {
         auto release( auto& state, const auto& value, auto&& projection, auto&& operation)
         {
            //
            // get hold of the work load for this scenario
            auto working = pull( state.working, value, projection);

            //
            // make sure to handle paths just one time
            tidy( working, &Reserve::path);

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
               for(auto&& wait : pull( state.pending, work.path, &Reserve::path))
               {
                  //
                  // start all over
                  resource::reserve( state, std::move( wait));
               }
            }

            return result;
         }

      } // 
   } // local

   void reserve( State& state, Reserve request)
   {
      request.path = request.path.lexically_normal();

      auto reply = common::message::reverse::type( request);

      if(auto result = local::reserve( state, request))
      {
         reply.path = std::move( result.value());
      }
      else
      {
         reply.code = std::move( result.error());
      }

      switch( reply.code)
      {
      case code::ok:
      {
         // push to work load
         state.working.push_back( std::move( request));
         // send the reply
         state.multiplex.send( request.process.ipc, std::move( reply));
         break;
      }
      case code::busy:
      {
         if( request.wait)
         {
            // push to wait list
            state.pending.push_back( std::move( request));
         }
         else
         {
            // send the reply
            state.multiplex.send( request.process.ipc, std::move( reply));
         }
         break;
      }
      default:
      {
         // just send the reply
         state.multiplex.send( request.process.ipc, std::move( reply));
         break;
      }
      }
   }

   void commit( State& state, const Commit& request)
   {
      auto reply = common::message::reverse::type( request);
      reply.resource = request.resource;
      reply.trid = request.trid;

      //
      // pick the requests involved and finalize them
      reply.state = local::release( state, request.trid, &Reserve::trid, &local::detail::commit);

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
      reply.state = local::release( state, request.trid, &Reserve::trid, &local::detail::rollback);

      //
      // send the reply to the transaction manager
      state.multiplex.send( request.process.ipc, std::move( reply));
   }

   void mitigate( State& state, const Exit& request)
   {
      //
      // drop potential pending requests
      local::drop( state.pending, request.process, &Reserve::process);

      //
      // pick the requests involved and finalize them
      local::release( state, request.process, &Reserve::process, &local::detail::mitigate);
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

} // casual::file::resource
