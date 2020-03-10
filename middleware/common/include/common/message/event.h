//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"

#include "common/domain.h"
#include "common/code/xatmi.h"

namespace casual
{
   namespace common
   {
      namespace message
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

            namespace general
            {
               template< Type type>
               struct basic_task : basic_event< type>
               {
                  using basic_event< type>::basic_event;

                  enum class State : short
                  {
                     started,
                     success,
                     warning,
                     error,
                  };

                  State state = State::success;
                  std::string description;

                  bool done() const { return state != State::started;}

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     basic_event< type>::serialize( archive);
                     CASUAL_SERIALIZE( state);
                     CASUAL_SERIALIZE( description);
                  })

                  inline friend std::ostream& operator << ( std::ostream& out, State state)
                  {
                     switch( state)
                     {
                        case State::started: return out << "started";
                        case State::success: return out << "success";
                        case State::warning: return out << "warning";
                        case State::error: return out << "error";
                     }
                     return out << "<unknown>";
                  }
               };

               using Task = basic_task< Type::event_general_task>;

               namespace sub
               {
                  using Task = general::basic_task< Type::event_general_sub_task>;                  
               } // sub

            } // general

            namespace domain
            {
               namespace task
               {
                  enum class State : short
                  {
                     ok,
                     aborted,
                     error
                  };
                  inline std::ostream& operator << ( std::ostream& out, State state)
                  {
                     switch( state)
                     {
                        case State::ok: return out << "ok";
                        case State::aborted: return out << "aborted";
                        case State::error: return out << "error";
                     }
                     return out << "<unknown>";
                  }


                  template< Type type> 
                  struct basic_task : basic_event< type>
                  {
                     strong::task::id id;
                     std::string description;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        basic_event< type>::serialize( archive);
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( description);
                     })
                  };

                  using Begin = basic_task< Type::event_domain_task_begin>;
                  
                  using base_end = basic_task< Type::event_domain_task_end>;
                  struct End : base_end
                  {
                     State state = State::ok;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        base_end::serialize( archive);
                        CASUAL_SERIALIZE( state);
                     })
                  };
               } // task

               namespace server
               {
                  using base_connect = basic_event< Type::event_domain_server_connect>;
                  struct Connect : base_connect
                  {
                     Uuid identification;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_connect::serialize( archive);
                        CASUAL_SERIALIZE( identification);
                     )

                  };                  

               } // server

               using base_error = basic_event< Type::event_domain_error>;
               struct Error : base_error
               {
                  using base_error::base_error;
                  
                  enum class Severity : short
                  {
                     fatal, // shutting down
                     error, // keep going
                     warning
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, Severity value)
                  {
                     switch( value)
                     {
                        case Severity::fatal: return out << "fatal";
                        case Severity::error: return out << "error";
                        case Severity::warning: return out << "warning";
                     }
                     return out << "unknown";
                  }

                  std::string message;
                  std::string executable;
                  strong::process::id pid;
                  std::vector< std::string> details;

                  Severity severity = Severity::error;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_error::serialize( archive);
                     CASUAL_SERIALIZE( message);
                     CASUAL_SERIALIZE( executable);
                     CASUAL_SERIALIZE( pid);
                     CASUAL_SERIALIZE( details);
                     CASUAL_SERIALIZE( severity);
                  )
               };

               using base_group = basic_event< Type::event_domain_group>;
               struct Group : base_group
               {
                  enum class Context : int
                  {
                     boot_start,
                     boot_end,
                     shutdown_start,
                     shutdown_end,
                  };
                  inline friend std::ostream& operator << ( std::ostream& out, Context value)
                  {
                     switch( value)
                     {
                        case Context::boot_start: return out << "boot.start";
                        case Context::boot_end: return out << "boot.end";
                        case Context::shutdown_start: return out << "shutdown.start";
                        case Context::shutdown_end: return out << "shutdown.end";
                     }
                     assert( ! "not valid context");
                  }

                  platform::size::type id = 0;
                  std::string name;
                  Context context;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_group::serialize( archive);
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( context);
                  )

               };

               template< Type type>
               struct basic_procedure : basic_event< type>
               {
                  common::domain::Identity domain;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     basic_event< type>::serialize( archive);
                     CASUAL_SERIALIZE( domain);
                  )
               };


               namespace boot
               {
                  struct Begin : basic_procedure< Type::event_domain_boot_begin>{};
                  struct End : basic_procedure< Type::event_domain_boot_end>{};
               } // boot

               namespace shutdown
               {
                  struct Begin : basic_procedure< Type::event_domain_shutdown_begin>{};
                  struct End : basic_procedure< Type::event_domain_shutdown_end>{};
               } // shutdown

            } // domain


            namespace process
            {
               using base_spawn = basic_event< Type::event_domain_process_spawn>;
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

               using base_exit = basic_event< Type::event_domain_process_exit>;
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

            } // process

            namespace service
            {
               struct Metric 
               {
                  std::string service;
                  std::string parent;
                  common::process::Handle process;
                  Uuid execution;
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
                     CASUAL_SERIALIZE( process);
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

               struct Calls : basic_event< Type::event_service_calls>
               {
                  std::vector< Metric> metrics;

                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     basic_event< Type::event_service_calls>::serialize( archive);
                     CASUAL_SERIALIZE( metrics);
                  )
               };
        
            } // service

            namespace terminal
            {
               std::ostream& print( std::ostream& out, const general::Task& event);
               std::ostream& print( std::ostream& out, const general::sub::Task& event);
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
      } // message
   } // common
} // casual


