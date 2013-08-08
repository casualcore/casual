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
#include "common/string.h"

namespace casual
{
   namespace broker
   {
      namespace action
      {


         void addGroups( State& state, const config::domain::Domain& domain);

         std::vector< std::vector< std::shared_ptr< broker::Group> > > bootOrder( State& state);


         namespace instances
         {

            /*
            struct Active
            {
               bool operator () ( const type& value) const
               {

               }
            };
            */

            inline void modify( const std::shared_ptr< broker::Server>& executable, std::size_t instances)
            {



            }
         } // instances

         namespace server
         {
            std::vector< common::platform::pid_type> start( const config::domain::Server& server);

            template< typename Q>
            struct basic_start
            {
               basic_start( State& state) : m_state( state) {}

               void operator() ( const config::domain::Server& server)
               {
                  auto started = std::make_shared< broker::Server>();
                  started->path = server.path;

                  for( auto& groups: server.memberships)
                  {
                     started->memberships.push_back( m_state.groups.at( groups));
                  }

                  started->pid = common::process::spawn( server.path, common::string::split( server.arguments));

                  m_state.processes.push_back( started->pid);

                  m_state.servers.emplace( started->pid, std::move( started));
               }

               void operator() ( const std::vector< config::domain::Server>& servers)
               {

                  auto toBeStarted = servers;

                  auto batches = bootOrder( m_state);


                  for( auto& batch : batches)
                  {
                     for( auto& server : toBeStarted)
                     {
                        //group->
                     }
                  }
               }



            private:
               State& m_state;
            };
         }

         template< typename Q>
         void startServers( State& state, const config::domain::Domain& domain, Q& queue)
         {
            if( state.groups.empty())
            {
               addGroups( state, domain);
            }

            for( auto& order : bootOrder( state))
            {


            }
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
