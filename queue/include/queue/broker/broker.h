//!
//! broker.h
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_BROKER_BROKER_H_
#define QUEUE_BROKER_BROKER_H_


#include "common/message/queue.h"
#include "common/transaction_id.h"

#include "config/queue.h"



#include <string>
#include <unordered_map>

namespace casual
{
   namespace queue
   {
      namespace broker
      {
         struct Settings
         {
            std::string configuration;
         };

         struct State
         {
            struct Group
            {
               using id_type = common::message::server::Id;

               Group() = default;
               Group( id_type id) : id( std::move( id)) {}

               id_type id;

            };

            std::vector< Group> groups;

            std::unordered_map< std::string, common::message::queue::lookup::Reply> queues;

            std::map< common::transaction::ID, std::vector< Group>> involved;

            std::vector< common::platform::pid_type> processes() const;

            void removeProcess( common::platform::pid_type);


            struct Correlation
            {
               using id_type = common::message::server::Id;

               Correlation( id_type caller, std::vector< Group> groups)
                  : caller( std::move( caller))
               {
                  std::move( std::begin( groups), std::end( groups), std::back_inserter( requests));
               }

               enum class State
               {
                  empty,
                  pending,
                  replied,
                  error,
               };

               struct Request
               {
                  Request() = default;
                  Request( Group group) : group( std::move( group)) {}

                  Group group;
                  State state = State::pending;
               };

               bool replied() const
               {
                  return common::range::all_of( requests, []( const Request& r){ return r.state >= State::replied;});
               }

               State state() const
               {
                  auto max = common::range::max( requests, []( const Request& lhs, const Request& rhs)
                        {
                           return lhs.state < rhs.state;
                        });

                  if( max)
                  {
                     return max->state;
                  }
                  return State::empty;
               }


               void state( const id_type& id, State state)
               {
                  auto found = common::range::find_if( requests, [&]( const Request& r){ return r.group.id.pid == id.pid;});

                  if( found)
                  {
                     found->state = state;
                  }
               }


               id_type caller;

               std::vector< Request> requests;

            };

            std::map< common::transaction::ID, Correlation> correlation;
         };



      } // broker

      struct Broker
      {
         Broker( broker::Settings settings);
         ~Broker();

         void start();

      private:

         casual::common::file::scoped::Path m_queueFilePath;
         broker::State m_state;

      };

   } // queue
} // casual

#endif // BROKER_H_
