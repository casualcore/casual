//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_MESSAGE_SERVICE_H_
#define CASUAL_COMMON_MESSAGE_SERVICE_H_

#include "common/message/type.h"
#include "common/message/buffer.h"

#include "common/transaction/id.h"
#include "common/service/type.h"
#include "common/buffer/type.h"
#include "common/uuid.h"
#include "common/flag/xatmi.h"
#include "common/code/xatmi.h"

#include "common/service/header.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace service
         {
            struct Base
            {
               Base() = default;

               explicit Base( std::string name, std::string category, common::service::transaction::Type transaction)
                  : name( std::move( name)), category( std::move( category)), transaction( transaction)
               {}

               Base( std::string name)
                  : name( std::move( name))
               {}

               std::string name;
               std::string category;
               common::service::transaction::Type transaction = common::service::transaction::Type::automatic;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & name;
                  archive & category;
                  archive & transaction;
               })
            };
            static_assert( traits::is_movable< Base>::value, "not movable");

         } // service

         struct Service : service::Base
         {
            using service::Base::Base;

            common::platform::time::unit timeout = common::platform::time::unit::zero();

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               service::Base::marshal( archive);
               archive & timeout;
            })

            friend std::ostream& operator << ( std::ostream& out, const Service& value);
         };
         static_assert( traits::is_movable< Service>::value, "not movable");


         namespace service
         {
            namespace call
            {
               //!
               //! Represent service information in a 'call context'
               //!
               struct Service : message::Service
               {
                  using message::Service::Service;

                  std::vector< strong::ipc::id> event_subscribers;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     message::Service::marshal( archive);
                     archive & event_subscribers;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const call::Service& value);
               };
               static_assert( traits::is_movable< Service>::value, "not movable");
            } // call

            struct Transaction
            {
               enum class State : char
               {
                  absent,
                  active = absent,
                  rollback,
                  timeout,
                  error,
               };

               common::transaction::ID trid;
               State state = State::active;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & trid;
                  archive & state;
               })

               friend std::ostream& operator << ( std::ostream& out, State value);
               friend std::ostream& operator << ( std::ostream& out, const Transaction& message);
            };
            static_assert( traits::is_movable< Transaction>::value, "not movable");

            namespace advertise
            {
               //!
               //! Represent service information in a 'advertise context'
               //!
               using Service = message::service::Base;

               static_assert( traits::is_movable< Service>::value, "not movable");

            } // advertise


            struct Advertise : basic_message< Type::service_advertise>
            {
               enum class Directive : char
               {
                  add,
                  remove,
                  replace
               };

               Directive directive = Directive::add;

               common::process::Handle process;
               std::vector< advertise::Service> services;


               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & directive;
                  archive & process;
                  archive & services;
               })

               friend std::ostream& operator << ( std::ostream& out, Directive value);
               friend std::ostream& operator << ( std::ostream& out, const Advertise& message);
            };
            static_assert( traits::is_movable< Advertise>::value, "not movable");


            namespace lookup
            {
               //!
               //! Represent "service-name-lookup" request.
               //!
               struct Request : basic_message< Type::service_name_lookup_request>
               {
                  enum class Context : char
                  {
                     regular,
                     no_reply,
                     forward,
                     gateway,
                  };

                  std::string requested;
                  common::process::Handle process;
                  Context context = Context::regular;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & requested;
                     archive & process;
                     archive & context;
                  })


                  friend std::ostream& operator << ( std::ostream& out, const Context& value);
                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };
               static_assert( traits::is_movable< Request>::value, "not movable");



               //!
               //! Represent "service-name-lookup" response.
               //!
               struct Reply : basic_message< Type::service_name_lookup_reply>
               {
                  call::Service service;
                  common::process::Handle process;

                  enum class State : char
                  {
                     absent,
                     busy,
                     idle

                  };

                  State state = State::idle;

                  inline bool busy() const { return state == State::busy;}

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & service;
                     archive & process;
                     archive & state;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");
            } // lookup


            namespace call
            {
               namespace request
               {
                  enum class Flag : long
                  {
                     no_transaction = cast::underlying( flag::xatmi::Flag::no_transaction),
                     no_reply = cast::underlying( flag::xatmi::Flag::no_reply),
                     no_time = cast::underlying( flag::xatmi::Flag::no_time),
                  };
                  using Flags = common::Flags< Flag>;

               } // request
               struct common_request
               {
                  common::process::Handle process;

                  Service service;
                  std::string parent;

                  common::transaction::ID trid;
                  request::Flags flags;

                  common::service::header::Fields header;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & process;
                     archive & service;
                     archive & parent;
                     archive & trid;
                     archive & flags;
                     archive & header;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const common_request& value);
               };

               using base_request = type_wrapper< common_request, Type::service_call>;
               struct basic_request : base_request
               {
                  request::Flags flags;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_request::marshal( archive);
                     archive & flags;
                  )
               };


               namespace caller
               {
                  //!
                  //! Represents a service call. via tp(a)call, from the callers perspective
                  //!
                  using Request = message::buffer::caller::basic_request< call::basic_request>;

                  static_assert( traits::is_movable< Request>::value, "not movable");
               } // caller

               namespace callee
               {
                  //!
                  //! Represents a service call. via tp(a)call, from the callee's perspective
                  //!
                  using Request = message::buffer::callee::basic_request< call::basic_request>;

                  static_assert( traits::is_movable< Request>::value, "not movable");

               } // callee


               //!
               //! Represent service reply.
               //!
               struct Reply :  basic_message< Type::service_reply>
               {
                  code::xatmi status = code::xatmi::ok;
                  long code = 0;
                  Transaction transaction;
                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & status;
                     archive & code;
                     archive & transaction;
                     archive & buffer;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Reply& message);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

               //!
               //! Represent the reply to the broker when a server is done handling
               //! a service-call and is ready for new calls
               //!
               struct ACK : basic_message< Type::service_acknowledge>
               {
                  common::process::Handle process;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const ACK& message);
               };
               static_assert( traits::is_movable< ACK>::value, "not movable");

            } // call

            namespace remote
            {
               struct Metric : basic_message< Type::service_remote_metrics>
               {
                  struct Service
                  {
                     Service() = default;
                     Service( std::string name, common::platform::time::unit duration)
                      : name( std::move( name)), duration( std::move( duration)) {}

                     std::string name;
                     common::platform::time::unit duration;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & name;
                        archive & duration;
                     })
                  };

                  common::process::Handle process;
                  std::vector< Service> services;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                     archive & services;
                  })

               };
               static_assert( traits::is_movable< Metric>::value, "not movable");
            } // remote

         } // service

         namespace reverse
         {

            template<>
            struct type_traits< service::lookup::Request> : detail::type< service::lookup::Reply> {};

            template<>
            struct type_traits< service::call::caller::Request> : detail::type<  service::call::Reply> {};

            template<>
            struct type_traits< service::call::callee::Request> : detail::type<  service::call::Reply> {};

         } // reverse

      } // message

   } // common


} // casual

#endif // SERVICE_H_
