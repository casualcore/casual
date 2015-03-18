//!
//! broker.h
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_BROKER_BROKER_H_
#define QUEUE_BROKER_BROKER_H_


#include "common/message/queue.h"
#include "common/transaction/id.h"

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

               using id_type = common::process::Handle;

               Group() = default;
               Group( std::string name, id_type process) : name( std::move( name)), process( std::move( process)) {}

               std::string name;
               id_type process;

               std::string queuebase;

               bool connected = false;

               friend bool operator == ( const Group& lhs, id_type process) { return lhs.process == process;}

            };

            std::vector< Group> groups;

            std::unordered_map< std::string, common::message::queue::lookup::Reply> queues;

            std::map< common::transaction::ID, std::vector< Group::id_type>> involved;

            std::vector< common::platform::pid_type> processes() const;

            void removeProcess( common::platform::pid_type);


            struct Correlation
            {
               using id_type = common::process::Handle;

               Correlation( id_type caller, std::vector< Group::id_type> groups)
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
                  Request( Group::id_type group) : group( std::move( group)) {}

                  Group::id_type group;
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
                  auto found = common::range::find_if( requests, [&]( const Request& r){ return r.group.pid == id.pid;});

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


         struct Queues
         {
            common::platform::pid_type groupId() const { return group.process.pid;}

            broker::State::Group group;
            std::vector< common::message::queue::Queue> queues;
         };

      } // broker

      struct Broker
      {
         Broker( broker::Settings settings);
         ~Broker();

         void start();

         const broker::State& state() const;

         std::vector< common::message::queue::information::queues::Reply> queues();

         common::message::queue::information::messages::Reply messages( const std::string& queue);

      private:


         casual::common::file::scoped::Path m_queueFilePath;
         broker::State m_state;

      };

   } // queue
} // casual

#endif // BROKER_H_
