//!
//! action.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef ACTION_H_
#define ACTION_H_

#include "config/domain.h"
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
            std::vector< common::platform::pid_type> start( const config::domain::Server& server);

            struct Start
            {
               Start( State& state) : m_state( state) {}

               void operator() ( const config::domain::Server& server)
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

         struct Resource
         {
            std::string key;
            std::size_t instances;

            std::string openinfo;
            std::string closeinfo;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( openinfo);
               archive & CASUAL_MAKE_NVP( closeinfo);
            }

         };

         struct Group
                  {
                     std::string name;
                     std::string note;

                     Resource resource;
                     std::vector< std::string> dependecies;

                     template< typename A>
                     void serialize( A& archive)
                     {
                        archive & CASUAL_MAKE_NVP( name);
                        archive & CASUAL_MAKE_NVP( note);
                        archive & CASUAL_MAKE_NVP( resource);
                        archive & CASUAL_MAKE_NVP( dependecies);
                     }
                  };


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
