//!
//! transform.h
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include "broker/servervo.h"
#include "broker/servicevo.h"
#include "broker/broker.h"

#include "sf/functional.h"

namespace casual
{

   namespace broker
   {
      namespace admin
      {
         namespace transform
         {
            typedef sf::functional::Chain< sf::functional::link::Nested> Chain;

            struct Server
            {
               admin::ServerVO operator () ( const std::shared_ptr< broker::Server>& value) const
               {
                  admin::ServerVO result;

                  result.setPath( value->path);
                  result.setPid( value->pid);
                  result.setQueue( value->queue_id);
                  result.setIdle( value->state != broker::Server::State::busy);

                  return result;
               }
            };

            struct Pid
            {
               broker::Server::pid_type operator () ( const std::shared_ptr< broker::Server>& value) const
               {
                  return value->pid;
               }
            };

            struct Service
            {
               admin::ServiceVO operator () ( const broker::Service& value) const
               {
                  admin::ServiceVO result;

                  result.setNameF( value.information.name);
                  result.setTimeoutF( value.information.timeout);

                  std::vector< long> pids;

                  std::transform( std::begin( value.servers), std::end( value.servers), std::back_inserter( pids), Pid());

                  result.setPids( std::move( pids));

                  return result;
               }
            };
         } // transform
      } // admin
   } // broker
} // casual



#endif /* TRANSFORM_H_ */
