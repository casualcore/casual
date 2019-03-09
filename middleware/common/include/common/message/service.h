//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "common/message/buffer.h"
#include "common/message/event.h"

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

            struct Code 
            {
               code::xatmi result = code::xatmi::ok;
               long user = 0;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & result;
                  archive & user;
               })
               friend std::ostream& operator << ( std::ostream& out, const Code& value);
            };

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
               //! Represent service information in a 'call context'
               struct Service : message::Service
               {
                  using message::Service::Service;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     message::Service::marshal( archive);
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
               //! Represent service information in a 'advertise context'
               using Service = message::service::Base;

               static_assert( traits::is_movable< Service>::value, "not movable");

            } // advertise


            struct Advertise : basic_message< Type::service_advertise>
            {
               enum class Directive : short
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
                  CASUAL_MARSHAL( directive);
                  CASUAL_MARSHAL( process);
                  CASUAL_MARSHAL( services);
               })

               friend std::ostream& operator << ( std::ostream& out, Directive value);
               friend std::ostream& operator << ( std::ostream& out, const Advertise& message);
            };
            static_assert( traits::is_movable< Advertise>::value, "not movable");

            namespace concurrent
            {
               namespace advertise
               {
                  //! Represent service information in a 'advertise context'
                  struct Service : message::Service
                  {
                     Service() = default;
                     inline Service( std::string name, 
                        std::string category, 
                        common::service::transaction::Type transaction,
                        platform::size::type hops = 0)
                        : message::Service( std::move( name), std::move( category), transaction), hops( hops) {}

                     inline Service( std::function<void(Service&)> foreign) { foreign( *this);}

                     platform::size::type hops = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        message::Service::marshal( archive);
                        CASUAL_MARSHAL( hops);
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Service& value);
                  };

                  static_assert( traits::is_movable< Service>::value, "not movable");

               } // advertise


               struct Advertise : basic_message< Type::service_concurrent_advertise>
               {
                  enum class Directive : short
                  {
                     add,
                     remove
                  };

                  Directive directive = Directive::add;

                  common::process::Handle process;
                  platform::size::type order = 0;
                  std::vector< advertise::Service> services;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     CASUAL_MARSHAL( directive);
                     CASUAL_MARSHAL( process);
                     CASUAL_MARSHAL( order);
                     CASUAL_MARSHAL( services);
                  })

                  friend std::ostream& operator << ( std::ostream& out, Directive value);
                  friend std::ostream& operator << ( std::ostream& out, const Advertise& message);
               };
               static_assert( traits::is_movable< Advertise>::value, "not movable");
              
            } // concurrent   


            namespace lookup
            {
               //! Represent "service-name-lookup" request.
               struct Request : basic_message< Type::service_name_lookup_request>
               {
                  enum class Context : short
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


               //! Represent "service-name-lookup" response.
               struct Reply : basic_message< Type::service_name_lookup_reply>
               {
                  call::Service service;
                  common::process::Handle process;

                  enum class State : short
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

               namespace discard
               {
                  struct Request : basic_message< Type::service_name_lookup_discard_request>
                  {
                     std::string requested;
                     common::process::Handle process;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & requested;
                        archive & process;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };

                  struct Reply : basic_message< Type::service_name_lookup_discard_reply>
                  {
                     enum class State : short
                     {
                        absent,
                        discarded,
                        replied
                     };
                     State state = State::absent;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & state;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
               } // discard
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
                  //! Represents a service call. via tp(a)call, from the callers perspective
                  using Request = message::buffer::caller::basic_request< call::basic_request>;

                  static_assert( traits::is_movable< Request>::value, "not movable");
               } // caller

               namespace callee
               {
                  //! Represents a service call. via tp(a)call, from the callee's perspective
                  using Request = message::buffer::callee::basic_request< call::basic_request>;

                  static_assert( traits::is_movable< Request>::value, "not movable");

               } // callee


               //! Represent service reply.
               struct Reply :  basic_message< Type::service_reply>
               {
                  service::Code code;
                  Transaction transaction;
                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & code;
                     archive & transaction;
                     archive & buffer;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Reply& message);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

               //! Represent the reply to the service-manager when a server is done handling
               //! a service-call and is ready for new calls
               struct ACK : basic_message< Type::service_acknowledge>
               {
                  event::service::Metric metric;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & metric;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const ACK& message);
               };
               static_assert( traits::is_movable< ACK>::value, "not movable");

               
            } // call



         } // service

         namespace reverse
         {

            template<>
            struct type_traits< service::lookup::Request> : detail::type< service::lookup::Reply> {};

            template<>
            struct type_traits< service::lookup::discard::Request> : detail::type< service::lookup::discard::Reply> {};

            template<>
            struct type_traits< service::call::caller::Request> : detail::type<  service::call::Reply> {};

            template<>
            struct type_traits< service::call::callee::Request> : detail::type<  service::call::Reply> {};

         } // reverse

      } // message

   } // common


} // casual


