//!
//! transform.cpp
//!
//! Created on: Oct 4, 2014
//!     Author: Lazan
//!

#include "queue/transform.h"
#include "queue/broker/broker.h"

#include "common/algorithm.h"


namespace casual
{
   namespace queue
   {

      namespace transform
      {

         namespace local
         {
            namespace
            {
               template< typename G>
               void group( const broker::State::Group& group, G& result)
               {
                  result.pid = group.process.pid;
                  result.queue_id = group.process.queue;

                  result.name = group.name;

               }

               struct Group
               {
                  broker::admin::GroupVO operator () ( const broker::State::Group& group) const
                  {
                     broker::admin::GroupVO result;

                     local::group( group, result);

                     return result;
                  }

                  broker::admin::verbose::GroupVO operator () ( const broker::Queues& queues) const
                  {
                     broker::admin::verbose::GroupVO result;

                     local::group( queues.group, result);

                     return result;
                  }

               };

            } // <unnamed>
         } // local




         std::vector< broker::admin::GroupVO> groups( const broker::State& state)
         {
            std::vector< broker::admin::GroupVO> result;

            common::range::transform( state.groups, result, local::Group{});

            std::map< common::platform::pid_type, std::vector< std::string>> groupQeues;

            for( auto&& touple : state.queues)
            {
               groupQeues[ touple.second.process.pid].push_back( touple.first);
            }

            for( auto& group : result)
            {
               group.queues = std::move( groupQeues[ group.pid]);
            }
            return result;
         }

         broker::admin::QueueVO Queue::operator () ( const common::message::queue::Queue& queue) const
         {
            broker::admin::QueueVO result;

            result.id = queue.id;
            result.name = queue.name;
            result.retries = queue.retries;
            result.type = queue.type;
            result.error = queue.error;


            return result;
         }


         broker::admin::verbose::GroupVO Group::operator () ( const broker::Queues& queues) const
         {
            broker::admin::verbose::GroupVO result = local::Group()( queues);

            common::range::transform( queues.queues, result.queues, Queue());


            return result;
         }


         queue::Message Message::operator () ( common::message::queue::dequeue::Reply::Message& value) const
         {
            queue::Message result;

            result.id = value.id;
            result.attribues.available = value.avalible;
            result.attribues.properties = value.correlation;
            result.attribues.reply = value.reply;
            result.payload.type = value.type;
            std::swap( result.payload.data, value.payload);

            return result;
         }




      } // transform
   } // qeue


} // casual
