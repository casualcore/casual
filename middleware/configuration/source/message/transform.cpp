//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/message/transform.h"
#include "configuration/common.h"

#include "common/chronology.h"

namespace casual
{
   namespace configuration
   {
      namespace transform
      {
         common::message::domain::configuration::Domain configuration( const configuration::domain::Manager& domain)
         {
            Trace trace{ "configuration::transform domain"};

            common::message::domain::configuration::Domain result;

            result.name = domain.name;

            // Service
            {
               result.service.default_timeout = common::chronology::from::string( domain.manager_default.service.timeout);

               common::algorithm::transform( domain.services, result.service.services, []( const auto& s)
               {
                  common::message::domain::configuration::service::Service result;

                  result.name = s.name;
                  if( s.routes)
                     result.routes = s.routes.value();

                  if( s.timeout)
                     result.timeout = common::chronology::from::string( s.timeout.value());

                  return result;
               });
            }

            // Transaction
            {
               result.transaction.log = domain.transaction.log;

               common::algorithm::transform( domain.transaction.resources, result.transaction.resources, []( const auto& r)
               {
                  common::message::domain::configuration::transaction::Resource result;

                  result.name = r.name;
                  result.key = r.key.value_or( "");
                  result.instances = r.instances.value_or( 0);
                  result.note = r.note;
                  result.openinfo = r.openinfo.value_or( "");
                  result.closeinfo = r.closeinfo.value_or( "");

                  return result;
               });
            }

            // Gateway
            {
               common::algorithm::transform( domain.gateway.listeners, result.gateway.listeners, []( const auto& l)
               {
                  common::message::domain::configuration::gateway::Listener result;

                  result.address = l.address;

                  if( l.limit.has_value())
                  {
                     result.limit.messages = l.limit.value().messages.value_or( 0);
                     result.limit.size = l.limit.value().size.value_or( 0);
                  }

                  return result;
               });

               common::algorithm::transform( domain.gateway.connections, result.gateway.connections, []( const auto& c)
               {
                  common::message::domain::configuration::gateway::Connection result;

                  result.note = c.note;
                  result.restart = c.restart.value_or( true);
                  result.address = c.address.value_or( "");
                  result.services = c.services;
                  result.queues = c.queues;

                  return result;
               });
            }

            // Queue
            {
               common::algorithm::transform( domain.queue.groups, result.queue.groups, []( const auto& g)
               {
                  common::message::domain::configuration::queue::Group result;

                  result.name = g.name;
                  result.note = g.note;
                  result.queuebase = g.queuebase.value_or( "");

                  common::algorithm::transform( g.queues, result.queues, []( const auto& q)
                  {
                     common::message::domain::configuration::queue::Queue result;

                     result.name = q.name;
                     result.note = q.note;
                     result.retries = q.retries.value_or( 0);

                     return result;
                  });

                  return result;
               });
            }

            return result;
         }

      } // transform
   } // configuration
} // casual
