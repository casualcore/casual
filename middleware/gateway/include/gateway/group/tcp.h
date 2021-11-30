//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/common.h"
#include "gateway/message.h"

#include "common/communication/tcp.h"
#include "common/communication/select.h"
#include "common/algorithm.h"
#include "common/signal/timer.h"
#include "common/environment.h"

#include <chrono>

namespace casual
{
   namespace gateway::group::tcp
   {
      namespace connector
      {
         enum class Bound
         {
            in,
            out
         };

         std::ostream& operator << ( std::ostream& out, Bound value);
         
         common::Process spawn( Bound bound, const common::communication::Socket& socket);

         template< typename Configuration>
         struct Pending
         {
            struct Connector
            {
               Connector( common::Process process, common::communication::Socket socket, Configuration configuration)
                  : process{ std::move( process)}, socket{ std::move( socket)}, configuration{ std::move( configuration)}
               {}

               common::Process process;
               common::communication::Socket socket;
               Configuration configuration;

               friend bool operator == ( const Connector& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( socket);
                  CASUAL_SERIALIZE( configuration);
               )
            };

            void add( common::Process&& connector,
               common::communication::Socket&& socket,
               Configuration configuration)
            {
               m_connectors.emplace_back( std::move( connector), std::move( socket), std::move( configuration));
            }

            auto extract( common::strong::process::id pid)
            {
               if( auto found = common::algorithm::find( m_connectors, pid))
                  return common::algorithm::container::extract( m_connectors, std::begin( found));

               common::code::raise::error( common::code::casual::invalid_semantics, "failed to correlate pending connector for pid: ", pid);
            }

            //! removes the connection iff we've got it, and the exit is NOT
            //! a clean exit (status 0)
            void exit( const common::process::lifetime::Exit& exit)
            {
               if( exit.reason == decltype( exit.reason)::exited && exit.status == 0)
                  return;

               if( auto found = common::algorithm::find( m_connectors, exit.pid))
               {
                  auto connector = common::algorithm::container::extract( m_connectors, std::begin( found));
                  // we 'clear' the connector to not send unnecessary signals.
                  connector.process.clear();
               }
            }

            auto& connections() const noexcept { return m_connectors;}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE_NAME( m_connectors, "connectors");
            )

         private:
            std::vector< Connector> m_connectors;

         };

      } // connector

      namespace listener::dispatch
      {
         template< typename State>
         auto create( State& state, connector::Bound bound)
         {
            return [&state, bound]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::read)
            {
               Trace trace{ "gateway::group::tcp::listener::dispatch"};

               if( auto found = common::algorithm::find( state.listeners, descriptor))
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

                     auto connector = connector::spawn( bound, socket);
                     common::log::line( verbose::log, "connector: ", connector);

                     state.external.pending().add(
                        std::move( connector),
                        std::move( socket),
                        found->configuration);
                  }

                  return true;
               }
               return false;
            };

         }

      } // listener::dispatch

      struct Connection
      {
         using complete_type = common::communication::tcp::message::Complete;

         inline explicit Connection( common::communication::tcp::Duplex&& device, message::domain::protocol::Version protocol)
            : m_device{ std::move( device)}, m_protocol{ protocol} {}

      
         inline auto descriptor() const noexcept { return m_device.connector().descriptor();}

         //! tries to send as much of unsent as possible, if any.
         void unsent( common::communication::select::Directive& directive);
         
         template< typename M>
         auto send( common::communication::select::Directive& directive, M&& message)
         {
            return send( directive, common::serialize::native::complete< complete_type>( std::forward< M>( message)));
         }

         inline auto next()
         {
            return common::communication::device::non::blocking::next( m_device);
         }

         inline auto protocol() const noexcept { return m_protocol;}
         inline void protocol( message::domain::protocol::Version protocol) noexcept { m_protocol = protocol;}

         inline friend bool operator == ( const Connection& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor() == rhs;}
         
         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE_NAME( m_device, "device");
            CASUAL_SERIALIZE_NAME( m_unsent, "unsent");
            CASUAL_SERIALIZE_NAME( m_protocol, "protocol");
         )

      private:
         
         common::strong::correlation::id send( common::communication::select::Directive& directive, complete_type&& complete);

         std::vector< complete_type> m_unsent;
         common::communication::tcp::Duplex m_device;
         message::domain::protocol::Version m_protocol;
      };

      template< typename Configuration>
      struct External
      {
         struct Information
         {
            Information( common::strong::file::descriptor::id descriptor,
               common::domain::Identity domain,
               Configuration configuration)
               : descriptor{ descriptor}, domain{ std::move( domain)}, configuration{ std::move( configuration)}
            {}

            common::strong::file::descriptor::id descriptor;
            common::domain::Identity domain;
            Configuration configuration;
            platform::time::point::type created = platform::time::clock::type::now();

            inline friend bool operator == ( const Information& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;} 

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( domain);
               CASUAL_SERIALIZE( configuration);
               CASUAL_SERIALIZE( created);
            )
         };

         auto connected( 
            common::communication::select::Directive& directive,
            const gateway::message::domain::Connected& message)
         {
            auto connector = m_pending.extract( message.connector);
            // we 'know' the process has/going to exit by it self. We make sure we don't try to send signals
            connector.process.clear();
            
            auto descriptor = connector.socket.descriptor();

            common::log::line( common::log::category::information, "connection established to domain: '", message.domain.name, 
               "' - host: ", common::communication::tcp::socket::address::host( connector.socket),
               ", peer: ", common::communication::tcp::socket::address::peer( connector.socket));

            m_connections.emplace_back( std::move( connector.socket), message.version);
            m_information.emplace_back( descriptor, message.domain, std::move( connector.configuration));
            directive.read.add( descriptor);

            return descriptor;
         }

         auto descriptors() const noexcept 
         {
            return common::algorithm::transform( m_connections, []( auto& connection){ return connection.descriptor();});
         }

         group::tcp::Connection* connection( common::strong::file::descriptor::id descriptor)
         {
            if( auto found = common::algorithm::find( m_connections, descriptor))
               return found.data();
            return nullptr;
         }

         const Information* information( common::strong::file::descriptor::id descriptor) const noexcept
         {
            if( auto found = common::algorithm::find( m_information, descriptor))
               return found.data();
            return nullptr;
         }

         std::optional< Configuration> remove( 
            common::communication::select::Directive& directive, 
            common::strong::file::descriptor::id descriptor)
         {
            directive.read.remove( descriptor);
            common::algorithm::trim( m_connections, common::algorithm::remove( m_connections, descriptor));
            if( auto found = common::algorithm::find( m_information, descriptor))
               return common::algorithm::container::extract( m_information, std::begin( found)).configuration;
            
            return {};
         }

         void clear( common::communication::select::Directive& directive)
         {
            directive.read.remove( descriptors());
            m_connections.clear();
            m_information.clear();
         }

         auto& information() const noexcept { return m_information;}
         auto& connections() const noexcept { return m_connections;}
         auto connections() noexcept { return common::range::make( m_connections);}

         auto& pending() const noexcept { return m_pending;}
         auto& pending()  noexcept { return m_pending;}

         void last( common::strong::file::descriptor::id value) { m_last = value;}
         auto last() const noexcept { return m_last;}

         auto empty() const noexcept { return m_connections.empty();}

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE_NAME( m_connections, "connections");
            CASUAL_SERIALIZE_NAME( m_information, "information");
            CASUAL_SERIALIZE_NAME( m_pending, "pending");
            CASUAL_SERIALIZE_NAME( m_last, "last");
         )
      private:

         std::vector< Connection> m_connections;
         std::vector< Information> m_information;
         connector::Pending< Configuration> m_pending;

         //! holds the last external connection that was used
         common::strong::file::descriptor::id m_last;
      };
     
      //! Tries to connect the provided connections, if connect success, add
      //! to the state via `state.external.pending().add( ...)`
      //! used by outbound and reverse inbound 
      template< connector::Bound bound, typename State, typename C>
      void connect( State& state, C& connections)
      {
         Trace trace{ "gateway::group::tcp::connect"};

         // we don't want to be interupted during the connect phase.
         common::signal::thread::scope::Block block;

         auto connected = [&state]( auto& connection)
         {
            try
            {
               ++connection.metric.attempts;
               if( auto socket = common::communication::tcp::connect( connection.configuration.address))
               {
                  socket.set( common::communication::socket::option::File::no_block);
                  
                  auto connector = connector::spawn( bound, socket);

                  state.external.pending().add( 
                     std::move( connector),
                     std::move( socket),
                     connection.configuration);

                  return true;
               }
            }
            catch( ...)
            {
               // make sure we don't try directly
               connection.metric.attempts = std::max( connection.metric.attempts, platform::tcp::connect::attempts::threshhold);

               auto error = common::exception::capture();
               common::log::line( common::log::category::warning, error, " connect severely failed for address: '", connection.configuration.address, "' - action: try later");
            }

            return false;
         };

         common::algorithm::trim( connections, common::algorithm::remove_if( connections, connected));

         // check if we need to set a timeout to keep trying to connect

         auto min_attempts = []( auto& l, auto& r){ return l.metric.attempts < r.metric.attempts;};

         if( auto min = common::algorithm::min( connections, min_attempts))
         {
            // check if we're in unittest context or not.
            if( common::environment::variable::exists( common::environment::variable::name::unittest::context))
               common::signal::timer::set( std::chrono::milliseconds{ 10});
            else
               common::signal::timer::set( std::chrono::seconds{ 3});
         }

         common::log::line( verbose::log, "connections: ", connections);
      }

      template< typename State, typename M, typename L>
      common::strong::correlation::id send( State& state, common::strong::file::descriptor::id descriptor, M&& message, L&& lost)
      {
         try
         {
            if( auto connection = state.external.connection( descriptor))
            {
               return connection->send( state.directive, std::forward< M>( message));
            }
            else
            {
               common::log::line( common::log::category::error, common::code::casual::internal_correlation, " failed to correlate descriptor: ", descriptor);
               common::log::line( common::log::category::verbose::error, "state: ", state);
            }
         }
         catch( ...)
         {
            if( common::exception::capture().code() != common::code::casual::communication_unavailable)
               throw;
            
            lost( state, descriptor);
                  
         }
         return {};
      }

      namespace pending::send
      {
         template< typename State>
         auto dispatch( State& state)
         {
            return [&state]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::write)
            {
               Trace trace{ "gateway::group::tcp::pending::send::dispatch"};
               common::log::line( verbose::log, "descriptor: ", descriptor);

               if( auto connection = state.external.connection( descriptor))
               {
                  connection->unsent( state.directive);
                  return true;
               }

               return false;

            };
         }
      } // pending::send
      
   } // gateway::tcp
} // casual