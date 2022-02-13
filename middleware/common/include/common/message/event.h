//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "common/service/type.h"
#include "common/transaction/id.h"
#include "common/domain.h"
#include "common/code/xatmi.h"

namespace casual
{
   namespace common::message
   {
      namespace event
      {
         inline namespace v1 {

         // used 'internally' to poke on running tasks to enable them
         // to decide if they are done.
         using Idle = message::basic_message< Type::event_idle>;

         template< Type type> 
         using basic_event = message::basic_request< type>;

         namespace subscription
         {

            using base_begin = basic_event< Type::event_subscription_begin>;
            struct Begin : base_begin
            {
               using base_begin::base_begin;

               std::vector< Type> types;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_begin::serialize( archive);
                  CASUAL_SERIALIZE( types);
               )
            };

            using base_end = basic_event< Type::event_subscription_end>;
            struct End : base_end
            {
               using base_end::base_end;
            };
            

         } // subscription

         using base_error = basic_event< Type::event_error>;
         struct Error : base_error
         {
            using base_error::base_error;
            
            enum class Severity : short
            {
               fatal, // shutting down
               error, // keep going
            };

            friend std::ostream& operator << ( std::ostream& out, Severity value);

            std::error_code code;
            std::string message;
            Severity severity = Severity::error;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_error::serialize( archive);
               CASUAL_SERIALIZE( code);
               CASUAL_SERIALIZE( message);
               CASUAL_SERIALIZE( severity);
            )
         };

         using base_notification = basic_event< Type::event_notification>;
         struct Notification : base_notification
         {
            using base_notification::base_notification;
            
            std::string message;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               base_notification::serialize( archive);
               CASUAL_SERIALIZE( message);
            )
         };

         namespace task
         {
            enum class State : short
            {
               started,
               done,
               aborted,
               warning,
               error,
            };

            std::ostream& operator << ( std::ostream& out, State state);
         } // task

         template< Type type>
         struct basic_task : basic_event< type>
         {
            using basic_event< type>::basic_event;

            task::State state = task::State::done;
            std::string description;

            bool done() const { return state != task::State::started;}

            CASUAL_CONST_CORRECT_SERIALIZE({
               basic_event< type>::serialize( archive);
               CASUAL_SERIALIZE( state);
               CASUAL_SERIALIZE( description);
            })
         };

         using Task = basic_task< Type::event_task>;

         namespace sub
         {
            using Task = event::basic_task< Type::event_sub_task>;                  
         } // sub


         namespace process
         {
            using base_spawn = basic_event< Type::event_process_spawn>;
            struct Spawn : base_spawn
            {
               using base_spawn::base_spawn;

               std::string alias;
               std::string path;
               std::vector< strong::process::id> pids;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_spawn::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( path);
                  CASUAL_SERIALIZE( pids);
               )
            };

            using base_exit = basic_event< Type::event_process_exit>;
            struct Exit : base_exit
            {
               Exit() = default;
               Exit( common::process::lifetime::Exit state) : base_exit{ common::process::handle()}, state( std::move( state)) {}

               common::process::lifetime::Exit state;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_exit::serialize( archive);
                  CASUAL_SERIALIZE( state);
               )
            };

            using base_assassination_contract = basic_event< Type::event_process_assassination_contract>;
            struct Assassination : base_assassination_contract
            {
               using base_assassination_contract::base_assassination_contract;
               using Contract = common::service::execution::timeout::contract::Type;

               common::strong::process::id target{};
               Contract contract = Contract::linger;
                  
               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_assassination_contract::serialize( archive);
                  CASUAL_SERIALIZE( target);
                  CASUAL_SERIALIZE( contract);
               )
            };

            using base_configured = basic_event< Type::event_process_configured>;
            struct Configured : base_configured
            {
               using base_configured::base_configured;

               std::string alias;
               std::string path;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_configured::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( path);
               )
            };

         } // process

         namespace service
         {
            namespace metric
            {
               enum class Type : long
               {
                  sequential = 1,
                  concurrent = 2,
               };

               inline std::ostream& operator << ( std::ostream& out, Type value)
               {
                  switch( value)
                  {
                     case Type::sequential: return out << "sequential";
                     case Type::concurrent: return out << "concurrent";
                  }
                  return out << "<unknown>";
               }
            } // metric

            struct Metric 
            {
               std::string service;
               std::string parent;
               metric::Type type = metric::Type::sequential;
               
               common::process::Handle process;
               strong::correlation::id correlation;
               execution::type execution;
               common::transaction::ID trid;

               platform::time::point::type start{};
               platform::time::point::type end{};

               platform::time::unit pending{};

               code::xatmi code = code::xatmi::ok;

               auto duration() const noexcept { return end - start;}

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( parent);
                  CASUAL_SERIALIZE( type);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( execution);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( start);
                  CASUAL_SERIALIZE( end);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( code);
               )
            };

            struct Call : basic_event< Type::event_service_call>
            {
               Metric metric;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  basic_event< Type::event_service_call>::serialize( archive);
                  CASUAL_SERIALIZE( metric);
               )
            };

            using base_calls = basic_event< Type::event_service_calls>;
            struct Calls : base_calls
            {
               using base_calls::base_calls;

               std::vector< Metric> metrics;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  base_calls::serialize( archive);
                  CASUAL_SERIALIZE( metrics);
               )
            };
      
         } // service

         namespace terminal
         {
            std::ostream& print( std::ostream& out, const Error& event);
            std::ostream& print( std::ostream& out, const Notification& event);
            std::ostream& print( std::ostream& out, const process::Spawn& event);
            std::ostream& print( std::ostream& out, const process::Exit& event);

            std::ostream& print( std::ostream& out, const Task& event);
            std::ostream& print( std::ostream& out, const sub::Task& event);
         } // terminal
         
         } // inline v1
      } // event

      namespace is
      {
         namespace event
         {
            template< typename Message>
            constexpr bool message()
            {
               return Message::type() > Type::EVENT_BASE && Message::type() < Type::EVENT_BASE_END; 
            }

            template< typename M>
            constexpr bool message( M&& message)
            {
               return type( message) > Type::EVENT_BASE && type( message) < Type::EVENT_BASE_END; 
            }
            struct Message 
            {
               template< typename M>
               constexpr bool operator () ( M&& message) { return is::event::message( message);}
            };
         } // event
      } // is

   } // common::message
} // casual


