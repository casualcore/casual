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
            struct Base
            {
               Base( const broker::State& state) : m_state( state) {}

            protected:
               const broker::State& m_state;
            };

            struct Server : Base
            {
               using Base::Base;

               admin::InstanceVO operator () ( state::Server::pid_type pid) const
               {
                  admin::InstanceVO result;

                  auto& value = m_state.getInstance( pid);

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

                  common::range::transform( value.instances, result.instances, *this);

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

                  common::range::transform( value.instances, result.instances, Pid{});

                  return result;
               }
            };
         } // transform
      } // admin
   } // broker
} // casual



#endif /* TRANSFORM_H_ */
