//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/tcp/logical/connect.h"

#include "common/algorithm/container.h"

namespace casual
{
   //! Holds "all" stuff related to _connectors_ connect phase
   namespace gateway::group::tcp::connect
   {

      namespace state
      {
         template< typename Configuration>
         struct Connect
         {
            struct Prospect
            {
               Prospect( Configuration configuration)
                  : configuration{ std::move( configuration)} {}

               Configuration configuration;

               struct
               {
                  platform::size::type attempts{};
                  
                  CASUAL_LOG_SERIALIZE( CASUAL_SERIALIZE( attempts);)
               } metric;

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( configuration);
                  CASUAL_SERIALIZE( metric);
               )
            };

            struct Pending
            {
               Pending( common::communication::Socket socket, Prospect prospect)
                  : socket{ std::move( socket)}, prospect{ std::move( prospect)} {}

               common::communication::Socket socket;
               Prospect prospect;

               friend bool operator == ( const Pending& lhs, common::strong::file::descriptor::id rhs) { return lhs.socket == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( socket);
                  CASUAL_SERIALIZE( prospect);
               )
            };


            std::vector< Prospect> prospects;
            std::vector< Pending> pending;
            std::vector< Configuration> failed;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( prospects);
               CASUAL_SERIALIZE( pending);
               CASUAL_SERIALIZE( failed);
            )

         };

         template< typename State, typename Message>
         auto request( State& state, Message& message)
         {
            using namespace common;
            log::line( verbose::log, "message: ", message);
            log::line( verbose::log, "state: ", state);

            auto reply = state.reply( message);

            using Connection = std::decay_t< decltype( range::front( reply.state.connections))>;

            // add prospects connections
            algorithm::transform( state.connect.prospects, std::back_inserter( reply.state.connections), []( auto& prospect)
            {
               Connection result;
               result.runlevel = decltype( result.runlevel)::connecting;
               result.configuration = prospect.configuration;
               result.address.peer = prospect.configuration.address;
               return result;
            });

            algorithm::transform( state.connect.pending, std::back_inserter( reply.state.connections), []( auto& pending)
            {
               Connection result;
               result.runlevel = decltype( result.runlevel)::pending;
               result.configuration = pending.prospect.configuration;
               result.address.peer = pending.prospect.configuration.address;
               result.descriptor = pending.socket.descriptor();
               return result;
            });

            algorithm::transform( state.connect.failed, std::back_inserter( reply.state.connections), []( auto& failed)
            {
               Connection result;
               result.runlevel = decltype( result.runlevel)::failed;
               result.configuration = failed;
               result.address.peer = failed.address;
               return result;
            });

            algorithm::transform( state.disabled_connections, std::back_inserter( reply.state.connections), []( auto& disabled)
            {
               Connection result;
               result.runlevel = decltype( result.runlevel)::disabled;
               result.configuration = disabled;
               result.address.peer = disabled.address;
               return result;
            });

            return reply;
         }
      } // state

      namespace retry
      {
         template< typename Configuration>
         void alarm( const state::Connect< Configuration>& connect)
         {
            if( ! connect.prospects.empty())
            {
               static const auto duration = []() -> platform::time::unit
               {
                  // check if we're in unittest context or not.
                  if( common::environment::variable::exists( common::environment::variable::name::internal::unittest::context))
                     return std::chrono::milliseconds{ 10};
                  return platform::tcp::connect::attempts::delay;
               }();

               common::signal::timer::set( duration);
            }
         }
      } // retry


      //! Tries to connect all state.connect.prospects.
      //! Depending on the connect outcome:
      //!   * Socket -> success and we start the logical connection phase (unlikely)
      //!   * _pending connect_ -> the connection phase has started, we multiplex on when it's done
      //!   * recoverable error -> we keep the connection in prospects and try later
      //!   * non-recoverable error -> we move it to failed and don't try again.
      //!
      //! used by outbound and reverse inbound 
      template< logical::connect::Bound bound, typename State>
      void attempt( State& state)
      {
         Trace trace{ "gateway::group::tcp::connect::attempt"};
         using namespace common;

         // we don't want to be interrupted during the connect phase.
         common::signal::thread::scope::Block block;

         auto try_connect = [ &state]( auto& prospect)
         {
            ++prospect.metric.attempts;

            auto handlers = overload::compose(
               [ &state, &prospect]( communication::Socket socket) noexcept
               {
                  // check if recoverable error occurred.
                  if( ! socket)
                     return false;

                  auto connector = logical::connect::spawn( bound, socket);

                  state.connections.pending().add( 
                     std::move( connector),
                     std::move( socket),
                     prospect.configuration);

                  return true; 
               },
               [ &state, &prospect]( std::system_error error) noexcept
               {
                  log::line( log::category::error, "fatal connection error: ", error, " for connection: ", prospect); 

                  state.connect.failed.push_back( prospect.configuration);
                  return true;
               },
               [ &state, &prospect]( communication::tcp::non::blocking::Pending pending) noexcept
               {
                  Trace trace( "communication::tcp::connect pending");

                  auto socket = std::move( pending).socket();

                  state.directive.write.add( socket.descriptor());
                  state.connect.pending.emplace_back( std::move( socket), std::move( prospect));
                  
                  return true;
               });

            return std::visit( std::move( handlers), communication::tcp::non::blocking::connect( prospect.configuration.address));
         };

         algorithm::container::erase_if( state.connect.prospects, try_connect);

         retry::alarm( state.connect);

         log::line( verbose::log, "state.connect: ", state.connect);
      }

      namespace dispatch
      {
         template< typename State>
         auto create( State& state, logical::connect::Bound bound) 
         { 
            using namespace common;

            return [ &state, bound]( strong::file::descriptor::id descriptor, communication::select::tag::write)
            {
               Trace trace{ "gateway::group::tcp::connect::dispatch"};
               
               if( auto found = algorithm::find( state.connect.pending, descriptor))
               {
                  auto pending = algorithm::container::extract( state.connect.pending, std::begin( found));
                  log::line( verbose::log, "pending: ", pending);

                  // we don't multiplex any more
                  state.directive.write.remove( pending.socket.descriptor());

                  if( auto error = pending.socket.error())
                  {
                     if( communication::tcp::non::blocking::error::recoverable( error.value()))
                     {
                        log::line( verbose::log, "multiplex connect recoverable error for: ", pending.prospect, " - error: ", error, " - action: retry later");
                        state.connect.prospects.push_back( std::move( pending.prospect));
                        retry::alarm( state.connect);
                     }
                     else
                     {
                        log::line( log::category::error, "multiplex connect fatal error for: ", pending.prospect, " - error: ", error, " - action: move to failed");
                        state.connect.failed.push_back( std::move( pending.prospect.configuration));
                     }
                     return true;
                  }

                  auto connector = logical::connect::spawn( bound, pending.socket);

                  state.connections.pending().add( 
                     std::move( connector),
                     std::move( pending.socket),
                     std::move( pending.prospect.configuration));
                  
                  return true;
               }
               return false;
            };
         }
      } // dispatch

   } // gateway::group::tcp::connect
} // casual