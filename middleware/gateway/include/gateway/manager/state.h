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
            struct basic_group
            {
               common::process::Handle process;
               Configuration configuration;

               friend inline bool operator == ( const basic_group& lhs, common::strong::process::id rhs) { return lhs.process == rhs;}
               inline explicit operator bool () const { return static_cast< bool>( process);}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( configuration);
               )
            };

            namespace inbound
            {  
               using Group = basic_group< configuration::model::gateway::inbound::Group>;
            } // inbound

            namespace outbound
            {
               using base_group = basic_group< configuration::model::gateway::outbound::Group>;
               struct Group : base_group
               {
                  platform::size::type order{};
                  
                  CASUAL_LOG_SERIALIZE(
                     base_group::serialize( archive);
                     CASUAL_SERIALIZE( configuration);
                  )
               };
            } // outbound

            namespace executable
            {
               std::string path( const inbound::Group& value);
               std::string path( const outbound::Group& value);
            
            } // executable

            enum class Runlevel : short
            {
               startup,
               running,
               shutdown
            };
            std::string_view description( Runlevel value);
         } // state

         struct State
         {

            //! @return true if we're done, and ready to exit
            bool done() const;

            struct
            {
               std::vector< state::inbound::Group> groups;
               
               CASUAL_LOG_SERIALIZE( CASUAL_SERIALIZE( groups);)
            } inbound;

            struct
            {
               std::vector< state::outbound::Group> groups;
               
               CASUAL_LOG_SERIALIZE( CASUAL_SERIALIZE( groups);)
            } outbound;
            

            common::state::Machine< state::Runlevel, state::Runlevel::startup> runlevel;


            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( inbound);
               CASUAL_SERIALIZE( outbound);
               CASUAL_SERIALIZE( runlevel);
            )

         };

      } // manager
   } // gateway
} // casual


