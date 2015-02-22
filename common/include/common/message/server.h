//!
//! server.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMON_MESSAGE_SERVER_H_
#define COMMON_MESSAGE_SERVER_H_


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
         namespace server
         {
            namespace ping
            {

               struct Request : basic_id< cServerPingRequest>
               {

               };

               struct Reply : basic_id< cServerPingReply>
               {
                  using base_type = basic_id< cServerPingReply>;

                  Uuid uuid;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & uuid;
                  })

               };

            } // ping

            namespace connect
            {
               struct Request : public basic_connect< cServerConnectRequest>
               {
                  typedef basic_connect< cServerConnectRequest> base_type;

                  std::vector< Service> services;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                        base_type::marshal( archive);
                        archive & services;
                  })
               };


               //!
               //! Sent from the broker with "start-up-information" for a server
               //!
               struct Reply : basic_message< cServerConnectReply>
               {

                  Reply() = default;
                  Reply( Reply&&) = default;
                  Reply& operator = ( Reply&&) = default;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     // no information
                  })
               };


            } // connect
         } // server


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
            };


            struct Advertise : basic_message< cServiceAdvertise>
            {

               std::string serverPath;
               process::Handle process;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & serverPath;
                  archive & process;
                  archive & services;
               })
            };

            struct Unadvertise : basic_message< cServiceUnadvertise>
            {
               process::Handle process;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & process;
                  archive & services;
               })
            };

            namespace name
            {
               namespace lookup
               {
                  //!
                  //! Represent "service-name-lookup" request.
                  //!
                  struct Request : basic_message< cServiceNameLookupRequest>
                  {
                     Request() = default;
                     Request( Request&&) = default;
                     Request& operator = ( Request&&) = default;

                     std::string requested;
                     process::Handle process;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & requested;
                        archive & process;
                     })
                  };

                  //!
                  //! Represent "service-name-lookup" response.
                  //!
                  struct Reply : basic_message< cServiceNameLookupReply>
                  {

                     Service service;

                     process::Handle process;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & service;
                        archive & process;
                     })
                  };
               } // lookup
            } // name


            namespace call
            {

               struct base_call : basic_message< cServiceCall>
               {

                  base_call() = default;

                  base_call( base_call&&) = default;
                  base_call& operator = ( base_call&&) = default;

                  base_call( const base_call&) = delete;
                  base_call& operator = ( const base_call&) = delete;

                  platform::descriptor_type descriptor = 0;
                  Service service;
                  process::Handle reply;
                  common::Uuid execution;
                  std::string caller;
                  common::transaction::ID trid;
                  std::int64_t flags;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & descriptor;
                     archive & service;
                     archive & reply;
                     archive & execution;
                     archive & caller;
                     archive & trid;
                     archive & flags;
                  })
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

                     //
                     // Only for input
                     //
                     template< typename A>
                     void marshal( A& archive)
                     {
                        base_call::marshal( archive);
                        archive >> buffer;
                     }
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
               //! @todo: change to service::call::Reply
               //!
               struct Reply :  basic_message< cServiceReply>
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
                     archive & descriptor;
                     archive & error;
                     archive & code;
                     archive & buffer;
                     archive & transaction;
                  })

               };

               //!
               //! Represent the reply to the broker when a server is done handling
               //! a service-call and is ready for new calls
               //!
               struct ACK : basic_message< cServiceAcknowledge>
               {

                  std::string service;
                  process::Handle process;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & service;
                     archive & process;
                  })
               };

            } // call

         } // service
      } // message
   } //common
} // casual


#endif // SERVER_H_
