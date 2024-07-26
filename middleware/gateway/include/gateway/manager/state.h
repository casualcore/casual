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

#include "casual/task.h"


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

               friend inline bool operator == ( const basic_group& lhs, const std::string& rhs) { return lhs.configuration.alias == rhs;}
               friend inline bool operator == ( const basic_group& lhs, common::strong::process::id rhs) { return lhs.process == rhs;}
               inline explicit operator bool () const { return static_cast< bool>( process);}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( configuration);
               )
            };

            namespace inbound
            {  
               using Group = basic_group< casual::configuration::model::gateway::inbound::Group>;
            } // inbound

            namespace outbound
            {
               using Group = basic_group< casual::configuration::model::gateway::outbound::Group>;
            } // outbound

            namespace executable
            {
               std::filesystem::path path( const inbound::Group& value);
               std::filesystem::path path( const outbound::Group& value);
            
            } // executable

            enum class Runlevel : short
            {
               configuring,
               running,
               shutdown
            };
            std::string_view description( Runlevel value);
         } // state

         struct State
         {
            common::state::Machine< state::Runlevel, state::Runlevel::configuring> runlevel;

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

            //! coordinated tasks, only(?) for shutdown 
            //! inbound before outbound
            casual::task::Coordinator tasks;

            void remove( common::strong::process::id pid);

            
            //! @return true if we're done, and ready to exit
            bool done() const noexcept;
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( inbound);
               CASUAL_SERIALIZE( outbound);
               CASUAL_SERIALIZE( tasks);
            )

         };

      } // manager
   } // gateway
} // casual


