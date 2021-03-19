//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/tcp.h"
#include "gateway/message.h"

#include "common/serialize/macro.h"
#include "common/communication/select.h"
#include "common/communication/tcp.h"
#include "common/communication/ipc.h"
#include "common/domain.h"
#include "common/message/service.h"
#include "common/state/machine.h"

#include "configuration/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace gateway::group::inbound
   {
      namespace state
      {
         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };
         std::ostream& operator << ( std::ostream& out, Runlevel value);

         namespace pending
         {
            using Limit = configuration::model::gateway::inbound::Limit;

            struct Requests
            {
               using complete_type = common::communication::ipc::message::Complete;

               inline pending::Limit limit() const { return m_limits;}
               inline void limit( pending::Limit limit) { m_limits = limit;}

               inline bool congested() const
               {
                  if( m_limits.size > 0 && m_size > m_limits.size)
                     return true;

                  return m_limits.messages > 0 && static_cast< platform::size::type>( m_services.size() + m_complete.size()) > m_limits.messages;
               }
               
               template< typename M>
               inline auto add( M&& message)
               {
                  m_complete.push_back( common::serialize::native::complete< complete_type>( std::forward< M>( message)));
                  m_size += m_complete.back().size();
               }

               inline void add( common::message::service::call::callee::Request&& message)
               {
                  m_size += Requests::size( message);
                  m_services.push_back( std::move( message));
               }

               //! consumes pending calls, and sets the 'pending-roundtrip-state'
               complete_type consume( const common::Uuid& correlation, platform::time::unit pending);
               complete_type consume( const common::Uuid& correlation);

               //! consumes all pending associated with the correlations, if any.
               struct Result
               {
                  std::vector< common::message::service::call::callee::Request> services;
                  std::vector< complete_type> complete;
               };

               Result consume( const std::vector< common::Uuid>& correlations);

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE_NAME( m_services, "services");
                  CASUAL_SERIALIZE_NAME( m_complete, "complete");
                  CASUAL_SERIALIZE_NAME( m_size, "size");
                  CASUAL_SERIALIZE_NAME( m_limits, "limits");
               )

            private:

               inline static platform::size::type size( const common::message::service::call::callee::Request& message)
               {
                  return sizeof( message) + message.buffer.memory.size() + message.buffer.type.size();
               }

               std::vector< common::message::service::call::callee::Request> m_services;
               std::vector< complete_type> m_complete;
               platform::size::type m_size = 0;
               pending::Limit m_limits;
               
            };

         } // pending


         namespace external
         {
            struct Connection : group::tcp::Connection 
            {
               using group::tcp::Connection::Connection;

               message::domain::protocol::Version protocol{};
               
               CASUAL_LOG_SERIALIZE( 
                  group::tcp::Connection::serialize( archive);
                  CASUAL_SERIALIZE( protocol);
               )
            };

            namespace connection
            {
               struct Information
               {
                  common::strong::file::descriptor::id descriptor;
                  common::domain::Identity domain;
                  configuration::model::gateway::inbound::Connection configuration;
                  platform::time::point::type created{};

                  inline friend bool operator == ( const Information& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;} 

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( descriptor);
                     CASUAL_SERIALIZE( domain);
                     CASUAL_SERIALIZE( configuration);
                     CASUAL_SERIALIZE( created);
                  )
               };
            } // connection
         } // external

         struct External
         {
            inline void add( 
               common::communication::select::Directive& directive,
               common::communication::Socket&& socket, 
               configuration::model::gateway::inbound::Connection configuration)
            {
               auto descriptor = socket.descriptor();

               connections.emplace_back( std::move( socket));
               descriptors.push_back( descriptor);
               informations.push_back( [&](){
                  state::external::connection::Information result;
                  result.descriptor = descriptor;
                  result.configuration = std::move( configuration);
                  result.created = platform::time::clock::type::now();
                  return result;
               }());

               directive.read.add( descriptor);
            }

            inline auto empty() const noexcept { return connections.empty();}

            inline std::optional< configuration::model::gateway::inbound::Connection> remove( 
               common::communication::select::Directive& directive, 
               common::strong::file::descriptor::id descriptor)
            {
               directive.read.remove( descriptor);
               common::algorithm::trim( connections, common::algorithm::remove( connections, descriptor));
               common::algorithm::trim( descriptors, common::algorithm::remove( descriptors, descriptor));
               if( auto found = common::algorithm::find( informations, descriptor))
                  return common::algorithm::extract( informations, std::begin( found)).configuration;
               
               return {};
            }

            inline void clear( common::communication::select::Directive& directive)
            {
               directive.read.remove( descriptors);
               connections.clear();
               descriptors.clear();
               informations.clear();
            }

            inline state::external::Connection* connection( common::strong::file::descriptor::id descriptor)
            {
               if( auto found = common::algorithm::find( connections, descriptor))
                  return found.data();
               return nullptr;
            }

            inline state::external::connection::Information* information( common::strong::file::descriptor::id descriptor)
            {
               if( auto found = common::algorithm::find( informations, descriptor))
                  return found.data();
               return nullptr;
            }

            std::vector< state::external::Connection> connections;
            std::vector< common::strong::file::descriptor::id> descriptors;
            std::vector< state::external::connection::Information> informations;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( descriptors);
               CASUAL_SERIALIZE( informations);
            )
         };
         

         struct Correlation
         {
            Correlation() = default;
            Correlation( common::Uuid correlation, common::strong::file::descriptor::id descriptor)
               : correlation{ std::move( correlation)}, descriptor{ descriptor} {}

            common::Uuid correlation;
            common::strong::file::descriptor::id descriptor;

            inline friend bool operator == ( const Correlation& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}
            inline friend bool operator == ( const Correlation& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;} 

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( descriptor);
            )
         };

         struct Timeout
         {
            platform::time::point::type deadline;
            common::strong::process::id pid;
            common::Uuid correlation;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( deadline);
               CASUAL_SERIALIZE( pid);
               CASUAL_SERIALIZE( correlation);
            )

         };

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel, state::Runlevel::running> runlevel;

         common::communication::select::Directive directive;
         state::External external;

         struct
         {
            state::pending::Requests requests;
            std::vector< common::strong::file::descriptor::id> disconnects;
            
            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( requests);
               CASUAL_SERIALIZE( disconnects);
            )
         } pending;
         

         std::vector< state::Correlation> correlations;


         std::string alias;
         std::string note;


         //! @return the correlated connection, and remove the correlation
         state::external::Connection* consume( const common::Uuid& correlation);

         //! @return true if the state is ready to 'terminate'
         bool done() const noexcept;

         //! @returns a reply message to state `request` that is filled with what's possible
         template< typename M>
         auto reply( M&& request)
         {
            auto reply = common::message::reverse::type( request, common::process::handle());

            reply.state.alias = alias;
            reply.state.note = note;
            reply.state.limit = pending.requests.limit();

            reply.state.connections = common::algorithm::transform( external.connections, [&]( auto& connection)
            {
               auto descriptor = connection.descriptor();
               message::inbound::state::Connection result;
               result.descriptor = descriptor;
               result.address.local = common::communication::tcp::socket::address::host( descriptor);
               result.address.peer = common::communication::tcp::socket::address::peer( descriptor);

               if( auto found = external.information( descriptor))
               {
                  result.domain = found->domain;
                  result.configuration = found->configuration;
                  result.created = found->created;
               }

               return result;
            });

            return reply;
         }

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( external);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( correlations);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( note);
         )
      };

   } // gateway::group::inbound
} // casual