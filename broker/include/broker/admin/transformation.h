//!
//! transform.h
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include "broker/admin/brokervo.h"
#include "broker/broker.h"


namespace casual
{

   namespace broker
   {
      namespace admin
      {
         namespace transform
         {

            struct Instance
            {
               admin::InstanceVO operator () ( const state::Server::Instance& value) const
               {
                  admin::InstanceVO result;

                  result.pid = value.process.pid;
                  result.queue = value.process.queue;
                  result.state = static_cast< long>( value.state);
                  result.invoked = value.invoked;
                  result.last = value.last;
                  result.server = value.server;

                  return result;
               }

            };

            struct Server
            {
               admin::ServerVO operator () ( const state::Server& value) const
               {
                  admin::ServerVO result;

                  result.id = value.id;
                  result.alias = value.alias;
                  result.path = value.path;
                  result.instances = value.instances;

                  return result;
               }
            };

            struct Pid
            {
               state::Server::pid_type operator () ( const state::Server::Instance& value) const
               {
                  return value.process.pid;
               }
            };

            struct Service
            {
               admin::ServiceVO operator () ( const state::Service& value) const
               {
                  admin::ServiceVO result;

                  result.name = value.information.name;
                  result.timeout = value.information.timeout.count();
                  result.lookedup = value.lookedup;

                  common::range::transform( value.instances, result.instances, Pid{});

                  return result;
               }
            };
         } // transform
      } // admin
   } // broker
} // casual



#endif /* TRANSFORM_H_ */
