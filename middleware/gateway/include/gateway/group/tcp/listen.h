//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/tcp/logical/connect.h"

#include "common/communication/socket.h"
#include "common/log/category.h"

namespace casual
{
   //! Holds "all" stuff related to the _listen_ and _accept_ phase
   namespace gateway::group::tcp::listen
   {
      //! state for listener (connectee) groups
      namespace state
      {
         template< typename Configuration>
         struct Listen
         {
            struct Listener
            {
               Listener( common::communication::Socket socket, Configuration configuration)
                  : socket{ std::move( socket)}, configuration{ std::move( configuration)} {}

               auto descriptor() const noexcept { return socket.descriptor();}

               common::communication::Socket socket;
               Configuration configuration;
               platform::time::point::type created = platform::time::clock::type::now();

               inline friend bool operator == ( const Listener& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor() == rhs;}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( socket);
                  CASUAL_SERIALIZE( configuration);
                  CASUAL_SERIALIZE( created);
               )
            };

            friend auto descriptors( const std::vector< Listener>& listeners) noexcept
            {
               return common::algorithm::transform( listeners, []( auto& listener){ return listener.descriptor();});
            }
            
            //! clears the _actives_ and move to _offline_, also remove from directive.read
            auto clear( common::communication::select::Directive& directive)
            {
               auto listeners = std::exchange( actives, {});

               directive.read.remove( descriptors( listeners));

               for( auto& listener : listeners)
                  offline.push_back( std::move( listener.configuration));
            }

            common::strong::socket::id descriptor( const std::string& address) const
            {
               auto is_address = [ &address]( auto& listener){ return listener.configuration == address;};

               if( auto found = common::algorithm::find_if( actives, is_address))
                  return found->socket.descriptor();

               return {};
            }

            std::optional< Configuration> extract( common::communication::select::Directive& directive, common::strong::socket::id id)
            {
               if( auto found = common::algorithm::find( actives, id))
               {
                  auto active = common::algorithm::container::extract( actives, std::begin( found));
                  directive.read.remove( active.socket.descriptor());
                  return active.configuration;
               }
               return {};
            }

            std::optional< Listener> extract( common::communication::select::Directive& directive, const std::string& address)
            {
               auto is_address = [ &address]( auto& listener){ return listener.configuration.address == address;};

               if( auto found = common::algorithm::find_if( actives, is_address))
               {
                  auto active = common::algorithm::container::extract( actives, std::begin( found));
                  directive.read.remove( active.socket.descriptor());
                  return active;
               }
               return {};
            }

            void replace_configuration( Configuration configuration)
            {
               auto is_address = [ &configuration]( auto& listener){ return listener.configuration.address == configuration.address;};

               if( auto found = common::algorithm::find_if( actives, is_address))
                  found->configuration = std::move( configuration);
            }

            std::vector< Configuration> configuration() const
            {
               auto result = common::algorithm::transform( actives, []( auto& listener)
               { 
                  return listener.configuration;
               });

               // TODO offline/failed too?

               return result;
            };

            std::vector< Listener> actives;
            std::vector< Configuration> offline;
            std::vector< Configuration> failed;
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( actives);
               CASUAL_SERIALIZE( offline);
               CASUAL_SERIALIZE( failed);
            )
         };

         template< typename State, typename Message>
         auto request( State& state, Message& message)
         {
            Trace trace{ "gateway::group::tcp::listen::state::request"};
            using namespace common;

            log::line( verbose::log, "message: ", message);
            log::line( verbose::log, "state: ", state);

            auto reply = state.reply( message);
            
            reply.state.listeners = algorithm::transform( state.listen.actives, []( auto& listener)
            {
               message::state::Listener result;
               result.runlevel = decltype( result.runlevel)::listening;
               result.address = listener.configuration.address;
               result.descriptor = listener.socket.descriptor();
               result.created = listener.created;

               return result;
            });

            algorithm::transform( state.listen.failed, std::back_inserter( reply.state.listeners), []( auto& failed)
            {
               message::state::Listener result;
               result.runlevel = decltype( result.runlevel)::failed;
               result.address = failed.address;

               return result;
            });

            algorithm::transform( state.disabled_connections, std::back_inserter( reply.state.listeners), []( auto& disabled)
            {
               message::state::Listener result;
               result.runlevel = decltype( result.runlevel)::disabled;
               result.address = disabled.address;

               return result;
            });


            log::line( verbose::log, "reply: ", reply);

            return reply;
         };

      } // state


      template< typename State, typename Configuration>
      void attempt( State& state, std::vector< Configuration> configuration)
      {
         using namespace common;
         auto try_listen = [ &state]( auto& configuration)
         {
            try
            {
               auto& listener = state.listen.actives.emplace_back( 
                  communication::tcp::socket::listen( configuration.address),
                  std::move( configuration));

               state.directive.read.add( listener.socket.descriptor());

               // we need the socket to not block in 'accept'
               listener.socket.set( communication::socket::option::File::no_block);

               log::information( "started listening on: ", listener.configuration.address);
            }
            catch( ...)
            {
               log::error( exception::capture(), "failed to listen on ", configuration.address);
               state.listen.failed.push_back( std::move( configuration));
            } 
         };

         algorithm::for_each( configuration, try_listen);
      }


      namespace dispatch
      {
         template< typename State>
         auto create( State& state, logical::connect::Bound bound)
         {
            return [&state, bound]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::read)
            {
               Trace trace{ "gateway::group::tcp::listener::dispatch"};

               if( auto found = common::algorithm::find( state.listen.actives, descriptor))
               {
                  common::log::line( verbose::log, "found: ", *found);

                  auto accept = []( auto& socket)
                  {
                     try
                     {
                        return common::communication::tcp::socket::accept( socket);
                     }
                     catch( ...)
                     {
                        common::exception::sink();
                        return common::communication::Socket{};
                     }
                  };

                  if( auto socket = accept( found->socket))
                  {
                     // the socket needs to be 'no block'
                     socket.set( common::communication::socket::option::File::no_block);
                     common::log::line( verbose::log, "socket: ", socket);

                     auto connector = logical::connect::spawn( bound, socket);
                     common::log::line( verbose::log, "connector: ", connector);

                     state.connections.pending().add(
                        std::move( connector),
                        std::move( socket),
                        found->configuration);
                  }

                  return true;
               }
               return false;
            };
         }

      } // dispatch
   } // gateway::group::tcp::listen
} // casual