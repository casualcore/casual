//!
//! casual
//!

#ifndef CASUAL_COMMON_MESSAGE_SERVICE_H_
#define CASUAL_COMMON_MESSAGE_SERVICE_H_

#include "common/message/type.h"
#include "common/message/buffer.h"

#include "common/transaction/id.h"
#include "common/service/type.h"
#include "common/buffer/type.h"
#include "common/uuid.h"

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

            std::chrono::microseconds timeout = std::chrono::microseconds::zero();

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

                  std::vector< platform::ipc::id::type> traffic_monitors;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     message::Service::marshal( archive);
                     archive & traffic_monitors;
                  })
               };
               static_assert( traits::is_movable< Service>::value, "not movable");
            } // call

            struct Transaction
            {
               common::transaction::ID trid;
               std::int64_t state = 0;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & trid;
                  archive & state;
               })

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
               struct base_request
               {

                  platform::descriptor::type descriptor = 0;
                  common::process::Handle process;

                  Service service;
                  std::string parent;

                  common::transaction::ID trid;
                  std::int64_t flags = 0;

                  std::vector< common::service::header::Field> header;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & descriptor;
                     archive & process;
                     archive & service;
                     archive & parent;
                     archive & trid;
                     archive & flags;
                     archive & header;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const base_request& value);
               };

               template< Type type>
               using basic_request = type_wrapper< base_request, type>;


               namespace caller
               {
                  template< Type type>
                  using basic_request = message::buffer::caller::basic_request< call::basic_request< type>>;

                  //!
                  //! Represents a service call. via tp(a)call, from the callers perspective
                  //!
                  using Request = basic_request< Type::service_call>;

                  static_assert( traits::is_movable< Request>::value, "not movable");
               } // caller

               namespace callee
               {
                  template< Type type>
                  using basic_request = message::buffer::callee::basic_request< call::basic_request< type>>;

                  //!
                  //! Represents a service call. via tp(a)call, from the callee's perspective
                  //!
                  using Request = basic_request< Type::service_call>;

                  static_assert( traits::is_movable< Request>::value, "not movable");

               } // callee


               //!
               //! Represent service reply.
               //!
               struct Reply :  basic_message< Type::service_reply>
               {

                  platform::descriptor::type descriptor = 0;
                  int error = 0;
                  long code = 0;
                  Transaction transaction;
                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & descriptor;
                     archive & error;
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

                  std::string service;
                  common::process::Handle process;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & service;
                     archive & process;
                  })
               };
               static_assert( traits::is_movable< ACK>::value, "not movable");

            } // call

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
