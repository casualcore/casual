//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/reverse/inbound/state/coordinate.h"

#include "common/serialize/macro.h"
#include "common/communication/select.h"
#include "common/communication/tcp.h"
#include "common/domain.h"
#include "common/message/service.h"

#include "configuration/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace gateway::reverse::inbound
   {
      namespace state
      {
         namespace pending
         {
            using Limit = configuration::model::gateway::Limit;
         } // pending

         struct Messages 
         {
            using complete_type = common::communication::message::Complete;

            inline pending::Limit limit() const { return m_limits;}
            inline void limit( pending::Limit limit) { m_limits = limit;}

            inline bool congested() const
            {
               if( m_limits.size > 0 && m_size > m_limits.size)
                  return true;

               return m_limits.messages > 0 && static_cast< platform::size::type>( m_calls.size() + m_complete.size()) > m_limits.messages;
            }
            
            template< typename M>
            inline auto add( M&& message)
            {
               m_complete.push_back( common::serialize::native::complete( std::forward< M>( message)));
               m_size += m_complete.back().size();
            }

            inline void add( common::message::service::call::callee::Request&& message)
            {
               m_size += Messages::size( message);
               m_calls.push_back( std::move( message));
            }

            //! consumes pending calls, and sets the 'pending-roundtrip-state'
            complete_type consume( const common::Uuid& correlation, platform::time::unit pending);
            complete_type consume( const common::Uuid& correlation);

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE_NAME( m_calls, "calls");
               CASUAL_SERIALIZE_NAME( m_complete, "complete");
               CASUAL_SERIALIZE_NAME( m_size, "size");
               CASUAL_SERIALIZE_NAME( m_limits, "limits");
            )

         private:

            inline static platform::size::type size( const common::message::service::call::callee::Request& message)
            {
               return sizeof( message) + message.buffer.memory.size() + message.buffer.type.size();
            }

            std::vector< common::message::service::call::callee::Request> m_calls;
            std::vector< complete_type> m_complete;
            platform::size::type m_size = 0;
            pending::Limit m_limits;
            
         };

         namespace external
         {
            struct Connection
            {
               inline explicit Connection( common::communication::Socket&& socket)
                  : device{ std::move( socket)} {}

               common::communication::tcp::Duplex device;

               inline friend bool operator == ( const Connection& lhs, common::strong::file::descriptor::id rhs) { return lhs.device.connector().descriptor() == rhs;}
               
               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( device);
               )
            };

            namespace connection
            {
               struct Information
               {
                  common::strong::file::descriptor::id id;
                  common::domain::Identity domain;
                  std::string note;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( domain);
                     CASUAL_SERIALIZE( note);
                  )
               };
            } // connection
         } // external

         struct External
         {
            inline void add( 
               common::communication::select::Directive& directive,
               common::communication::Socket&& socket, 
               std::string note)
            {
               auto descriptor = socket.descriptor();

               connections.emplace_back( std::move( socket));
               descriptors.push_back( descriptor);
               information.push_back( [&](){
                  state::external::connection::Information result;
                  result.id = descriptor;
                  result.note = std::move( note);
                  return result;
               }());

               directive.read.add( descriptor);
            }

            std::vector< state::external::Connection> connections;
            std::vector< common::strong::file::descriptor::id> descriptors;
            std::vector< state::external::connection::Information> information;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( descriptors);
               CASUAL_SERIALIZE( information);
            )
         };
         

         struct Correlation
         {
            Correlation( common::Uuid correlation, common::strong::file::descriptor::id descriptor)
               : correlation{ std::move( correlation)}, descriptor{ descriptor} {}

            common::Uuid correlation;
            common::strong::file::descriptor::id descriptor;

            inline friend bool operator == ( const Correlation& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( descriptor);
            )
         };

      } // state

      struct State
      {
         common::communication::select::Directive directive;
         state::External external;

         state::Messages messages;

         std::vector< state::Correlation> correlations;

         //! @return the correlated connection, and remove the correlation
         state::external::Connection* consume( const common::Uuid& correlation);

         struct
         {
            state::coordinate::Discovery discovery;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( discovery);
            )
         } coordinate;

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( external);
            CASUAL_SERIALIZE( messages);
            CASUAL_SERIALIZE( correlations);
            CASUAL_SERIALIZE( coordinate);
         )
      };

      } // gateway::reverse::inbound

} // casual
