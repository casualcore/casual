//!
//! transform.h
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include "broker/brokervo.h"
#include "broker/broker.h"


namespace casual
{

   namespace broker
   {
      namespace admin
      {
         namespace transform
         {

            struct Server
            {
               admin::InstanceVO operator () ( const state::Server::Instance& value) const
               {
                  admin::InstanceVO result;

                  result.pid = value.pid;
                  result.queueId = value.queue_id;
                  result.state = static_cast< long>( value.state);
                  result.invoked = value.invoked;
                  result.last = value.last;

                  return result;
               }

               admin::ServerVO operator () ( const state::Server& value) const
               {
                  admin::ServerVO result;

                  result.alias = value.alias;
                  result.path = value.path;

                  //common::range::transform( value.instances, result.instances, transform::Server{});

                  return result;
               }
            };

            struct Pid
            {
               state::Server::pid_type operator () ( const state::Server::Instance& value) const
               {
                  return value.pid;
               }
            };

            struct Service
            {
               admin::ServiceVO operator () ( const state::Service& value) const
               {
                  admin::ServiceVO result;

                  result.name = value.information.name;
                  result.timeout = value.information.timeout;
                  result.lookedup = value.lookedup;

                  common::range::transform( value.instances, result.instances,
                        common::chain::Nested::link( Pid{}, common::extract::Get{}));

                  return result;
               }
            };
         } // transform
      } // admin
   } // broker
} // casual



#endif /* TRANSFORM_H_ */
