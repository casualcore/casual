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

            template< common::message::Type type> 
            using basic_event = common::message::basic_request< type>;

            namespace subscription
            {

               using base_begin = basic_event< common::message::Type::event_subscription_begin>;
               struct Begin : base_begin
               {
                  std::vector< common::message::Type> types;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_begin::serialize( archive);
                     CASUAL_SERIALIZE( types);
                  )
               };

               using base_end = basic_event< common::message::Type::event_subscription_end>;
               struct End : base_end
               {
               };
               

            } // subscription

            namespace domain
            {
               namespace server
               {
                  using base_connect = basic_event< common::message::Type::event_domain_server_connect>;
                  struct Connect : base_connect
                  {
                     common::Uuid identification;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_connect::serialize( archive);
                        CASUAL_SERIALIZE( identification);
                     )

                  };                  

               } // server

               using base_error = basic_event< common::message::Type::event_domain_error>;
               struct Error : base_error
               {
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
                        default: return out << "unknown";
                     }
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

               using base_group = basic_event< common::message::Type::event_domain_group>;
               struct Group : base_group
               {

                  enum class Context : int
                  {
                     boot_start,
                     boot_end,
                     shutdown_start,
                     shutdown_end,
                  };

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

               template< common::message::Type type>
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
                  struct Begin : basic_procedure< common::message::Type::event_domain_boot_begin>{};
                  struct End : basic_procedure< common::message::Type::event_domain_boot_end>{};
               } // boot

               namespace shutdown
               {
                  struct Begin : basic_procedure< common::message::Type::event_domain_shutdown_begin>{};
                  struct End : basic_procedure< common::message::Type::event_domain_shutdown_end>{};
               } // shutdown

            } // domain


            namespace process
            {
               using base_spawn = basic_event< common::message::Type::event_process_spawn>;
               struct Spawn : base_spawn
               {
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

               using base_exit = basic_event< common::message::Type::event_process_exit>;
               struct Exit : base_exit
               {
                  Exit() = default;
                  Exit( common::process::lifetime::Exit state) : state( std::move( state)) {}

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
                  common::Uuid execution;
                  common::transaction::ID trid;

                  common::platform::time::point::type start;
                  common::platform::time::point::type end;

                  common::code::xatmi code;

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

            } // inline v1
         } // event

         namespace is
         {
            namespace event
            {
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


