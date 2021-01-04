//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "gateway/message.h"

#include "common/process.h"
#include "common/domain.h"

#include "common/communication/tcp.h"
#include "common/communication/select.h"
#include "common/message/coordinate.h"
#include "common/state/machine.h"

#include "configuration/model.h"


namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace state
         {      
            template< typename Configuration>
            struct basic_bound
            {
               common::process::Handle process;
               Configuration configuration;

               friend inline bool operator == ( const basic_bound& lhs, common::strong::process::id rhs) { return lhs.process == rhs;}
               inline explicit operator bool () const { return static_cast< bool>( process);}


               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( configuration);
               )
            };

            using Outbound = basic_bound< configuration::model::gateway::Outbound>;
            using Inbound = basic_bound< configuration::model::gateway::Inbound>;

            namespace executable
            {
               std::string path( const Outbound& value);
               std::string path( const Inbound& value);
            
            } // executable

            enum class Runlevel : short
            {
               startup,
               running,
               shutdown
            };
            std::ostream& operator << ( std::ostream& out, Runlevel value);
         } // state

         struct State
         {

            //! @return true if we're done, and ready to exit
            bool done() const;

            std::vector< state::Inbound> inbounds;
            std::vector< state::Outbound> outbounds;

            common::state::Machine< state::Runlevel, state::Runlevel::startup> runlevel;

            struct
            {
               common::message::coordinate::fan::Out< common::message::gateway::domain::discover::Reply, common::strong::process::id> discovery;
               common::message::coordinate::fan::Out< message::outbound::rediscover::Reply, common::strong::process::id> rediscovery;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( discovery);
                  CASUAL_SERIALIZE( rediscovery);
               )
            } coordinate;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( inbounds);
               CASUAL_SERIALIZE( outbounds);
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( coordinate);
            )

         };

      } // manager
   } // gateway
} // casual


