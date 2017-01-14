//!
//! casual 
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

            //
            // Service
            //
            {

               common::range::transform( domain.services, result.service.services, []( const service::Service& s){

                  common::message::domain::configuration::service::Service result;

                  result.name = s.name;
                  if( s.routes)
                     result.routes = s.routes.value();

                  if( s.timeout)
                     result.timeout = common::chronology::from::string( s.timeout.value());

                  return result;
               });
            }

            //
            // Transaction
            //
            {
               result.transaction.log = domain.transaction.log;

               common::range::transform( domain.transaction.resources, result.transaction.resources, []( const transaction::Resource& r){

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

            //
            // Gateway
            //
            {
               common::range::transform( domain.gateway.listeners, result.gateway.listeners, []( const gateway::Listener& l){
                  common::message::domain::configuration::gateway::Listener result;

                  result.address = l.address;

                  return result;
               });

               common::range::transform( domain.gateway.connections, result.gateway.connections, []( const gateway::Connection& c){
                  common::message::domain::configuration::gateway::Connection result;

                  result.note = c.note;
                  result.restart = c.restart.value_or( true);
                  if( c.type.has_value())
                     { result.type = c.type.value() == "ipc" ?
                           common::message::domain::configuration::gateway::Connection::Type::ipc :
                           common::message::domain::configuration::gateway::Connection::Type::tcp;}
                  result.address = c.address.value_or( "");
                  result.services = c.services;
                  result.queues = c.queues;

                  return result;
               });

            }

            //
            // Queue
            //
            {
               common::range::transform( domain.queue.groups, result.queue.groups, []( const queue::Group& g){
                  common::message::domain::configuration::queue::Group result;

                  result.name = g.name;
                  result.note = g.note;
                  result.queuebase = g.queuebase.value_or( "");

                  common::range::transform( g.queues, result.queues, []( const queue::Queue& q){
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
