//!
//! action.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef ACTION_H_
#define ACTION_H_

#include "broker/configuration.h"
#include "broker/broker.h"
#include "common/platform.h"

namespace casual
{
   namespace broker
   {
      namespace action
      {
         namespace server
         {
            std::vector< common::platform::pid_type> start( const configuration::Server& server);

            struct Start
            {
               Start( State& state) : m_state( state) {}

               void operator() ( const configuration::Server& server)
               {
                  for( auto pid : start( server))
                  {
                     m_state.processes.push_back( pid);
                  }
               }
            private:
               State m_state;
            };
         }


         namespace server
         {
            struct Instances
            {
               std::string pathRegexp;
               std::size_t instances;

               template< typename A>
               void serialize( A& archive)
               {
                  archive & CASUAL_MAKE_NVP( pathRegexp);
                  archive & CASUAL_MAKE_NVP( instances);
               }

            };
         }



      } // action
   } // broker
} // casual



#endif /* ACTION_H_ */
