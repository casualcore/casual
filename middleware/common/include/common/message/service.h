//!
//! service.h
//!
//! Created on: Jul 2, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MESSAGE_SERVICE_H_
#define CASUAL_COMMON_MESSAGE_SERVICE_H_

#include "common/message/type.h"

#include "common/transaction/id.h"
#include "common/buffer/type.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {
      namespace message
      {

         namespace service
         {
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


            struct Advertise : basic_message< Type::service_advertise>
            {

               std::string serverPath;
               process::Handle process;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & serverPath;
                  archive & process;
                  archive & services;
               })
            };

            struct Unadvertise : basic_message< Type::service_unadvertise>
            {
               process::Handle process;
               std::vector< Service> services;

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
                  process::Handle process;
                  Context context = Context::regular;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & requested;
                     archive & process;
                     archive & context;
                  })
               };

               //!
               //! Represent "service-name-lookup" response.
               //!
               struct Reply : basic_message< Type::service_name_lookup_reply>
               {
                  Service service;
                  process::Handle process;

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
                  process::Handle process;

                  Service service;
                  std::string parent;

                  common::transaction::ID trid;
                  std::int64_t flags = 0;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & descriptor;
                     archive & process;
                     archive & service;
                     archive & parent;
                     archive & execution;
                     archive & trid;
                     archive & flags;
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
                  buffer::Payload buffer;
                  Transaction transaction;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & descriptor;
                     archive & error;
                     archive & code;
                     archive & buffer;
                     archive & transaction;
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
                  process::Handle process;

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
