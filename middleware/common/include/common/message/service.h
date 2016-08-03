//!
//! casual
//!

#ifndef CASUAL_COMMON_MESSAGE_SERVICE_H_
#define CASUAL_COMMON_MESSAGE_SERVICE_H_

#include "common/message/type.h"

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
         struct Service
         {

            Service() = default;
            Service& operator = (const Service& rhs) = default;



            explicit Service( std::string name, std::uint64_t type, common::service::transaction::Type transaction)
               : name( std::move( name)), type( type), transaction( transaction)
            {}

            Service( std::string name)
               : Service( std::move( name), 0, common::service::transaction::Type::automatic)
            {}

            std::string name;
            std::uint64_t type = 0;
            std::chrono::microseconds timeout = std::chrono::microseconds::zero();
            common::service::transaction::Type transaction = common::service::transaction::Type::automatic;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & name;
               archive & type;
               archive & timeout;
               archive & transaction;
            })

            friend std::ostream& operator << ( std::ostream& out, const Service& value);
         };

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

            namespace advertise
            {
               struct Service : message::Service
               {
                  Service() = default;
                  Service( std::string name,
                        std::uint64_t type = 0,
                        common::service::transaction::Type transaction = common::service::transaction::Type::automatic,
                        std::size_t hops = 0)
                   : message::Service{ std::move( name), type, transaction}, hops{ hops} {}

                  std::size_t hops = 0;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     message::Service::marshal( archive);
                     archive & hops;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Service& message);
               };

            } // advertise


            struct Advertise : basic_message< Type::service_advertise>
            {
               common::process::Handle process;
               std::vector< advertise::Service> services;


               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & process;
                  archive & services;
               })

               friend std::ostream& operator << ( std::ostream& out, const Advertise& message);
            };

            struct Unadvertise : basic_message< Type::service_unadvertise>
            {
               common::process::Handle process;
               std::vector< std::string> services;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & process;
                  archive & services;
               })
            };



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
                  Request() = default;
                  Request( Request&&) = default;
                  Request& operator = ( Request&&) = default;

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
            } // lookup


            namespace call
            {

               struct base_call : basic_message< Type::service_call>
               {

                  base_call() = default;

                  base_call( base_call&&) = default;
                  base_call& operator = ( base_call&&) = default;

                  base_call( const base_call&) = delete;
                  base_call& operator = ( const base_call&) = delete;

                  platform::descriptor_type descriptor = 0;
                  common::process::Handle process;

                  Service service;
                  std::string parent;

                  common::transaction::ID trid;
                  std::int64_t flags = 0;

                  std::vector< common::service::header::Field> header;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & descriptor;
                     archive & process;
                     archive & service;
                     archive & parent;
                     archive & trid;
                     archive & flags;
                     archive & header;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const base_call& value);
               };

               namespace callee
               {

                  //!
                  //! Represents a service call. via tp(a)call, from the callee's perspective
                  //!
                  struct Request : public base_call
                  {

                     Request() = default;

                     Request( Request&&) = default;
                     Request& operator = ( Request&&) = default;

                     Request( const Request&) = delete;
                     Request& operator = ( const Request&) = delete;

                     buffer::Payload buffer;


                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_call::marshal( archive);
                        archive & buffer;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };

               } // callee

               namespace caller
               {
                  //!
                  //! Represents a service call. via tp(a)call, from the callers perspective
                  //!
                  struct Request : public base_call
                  {

                     Request( buffer::payload::Send&& buffer)
                           : buffer( std::move( buffer))
                     {
                     }

                     Request( Request&&) = default;
                     Request& operator = ( Request&&) = default;

                     Request( const Request&) = delete;
                     Request& operator = ( const Request&) = delete;

                     buffer::payload::Send buffer;

                     //
                     // Only for output
                     //
                     template< typename A>
                     void marshal( A& archive) const
                     {
                        base_call::marshal( archive);
                        archive << buffer;
                     }
                  };

               }

               //!
               //! Represent service reply.
               //!
               struct Reply :  basic_message< Type::service_reply>
               {

                  Reply() = default;
                  Reply( Reply&&) noexcept = default;
                  Reply& operator = ( Reply&&) noexcept = default;
                  Reply( const Reply&) = default;
                  Reply& operator = ( const Reply&) = default;

                  int descriptor = 0;
                  int error = 0;
                  long code = 0;
                  Transaction transaction;
                  buffer::Payload buffer;

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
