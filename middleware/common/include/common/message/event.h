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

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_begin::marshal( archive);
                     archive & types;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Begin& value);
               };

               using base_end = basic_event< common::message::Type::event_subscription_end>;
               struct End : base_end
               {

                  friend std::ostream& operator << ( std::ostream& out, const End& value);
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

                     CASUAL_CONST_CORRECT_MARSHAL(
                        base_connect::marshal( archive);
                        archive & process;
                        archive & identification;
                     )

                  };                  

               } // server

               using base_error = basic_event< common::message::Type::event_domain_error>;
               struct Error : base_error
               {
                  std::string message;
                  std::string executable;
                  strong::process::id pid;
                  std::vector< std::string> details;

                  enum class Severity : char
                  {
                     fatal, // shutting down
                     error, // keep going
                     warning
                  } severity = Severity::error;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_error::marshal( archive);
                     archive & message;
                     archive & executable;
                     archive & pid;
                     archive & details;
                     archive & severity;
                  )

                  friend std::ostream& operator << ( std::ostream& out, Severity value);
                  friend std::ostream& operator << ( std::ostream& out, const Error& value);
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

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_group::marshal( archive);
                     archive & id;
                     archive & name;
                     archive & context;
                  )
               };

               template< common::message::Type type>
               struct basic_procedure : basic_event< type>
               {
                  common::domain::Identity domain;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     basic_event< type>::marshal( archive);
                     archive & domain;
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

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_spawn::marshal( archive);
                     archive & alias;
                     archive & path;
                     archive & pids;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Spawn& value);
               };

               using base_exit = basic_event< common::message::Type::event_process_exit>;
               struct Exit : base_exit
               {
                  Exit() = default;
                  Exit( common::process::lifetime::Exit state) : state( std::move( state)) {}

                  common::process::lifetime::Exit state;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_exit::marshal( archive);
                     archive & state;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Exit& value);
               };
               static_assert( traits::is_movable< Exit>::value, "not movable");


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

                  CASUAL_CONST_CORRECT_MARSHAL
                  (
                     archive & service;
                     archive & parent;
                     archive & process;
                     archive & execution;
                     archive & trid;
                     archive & start;
                     archive & end;
                     archive & code;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Metric& value);
               };


               struct Call : basic_event< Type::event_service_call>
               {
                  Metric metric;

                  CASUAL_CONST_CORRECT_MARSHAL
                  (
                     basic_event< Type::event_service_call>::marshal( archive);
                     archive & metric;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Call& value);
               };
               static_assert( traits::is_movable< Call>::value, "not movable");

               struct Calls : basic_event< Type::event_service_calls>
               {
                  std::vector< Metric> metrics;

                  CASUAL_CONST_CORRECT_MARSHAL
                  (
                     basic_event< Type::event_service_calls>::marshal( archive);
                     archive & metrics;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Calls& value);
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


