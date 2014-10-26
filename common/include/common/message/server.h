//!
//! server.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMON_MESSAGE_SERVER_H_
#define COMMON_MESSAGE_SERVER_H_


#include "common/message/type.h"

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

                  typedef platform::queue_id_type queue_id_type;
               };


            } // connect


            typedef basic_disconnect< cServerDisconnect> Disconnect;

         } // server


         namespace service
         {

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

                     process::Handle supplier;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & service;
                        archive & supplier;
                     })
                  };
               } // lookup
            } // name

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
               std::string callee;
               common::transaction::ID trid;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & descriptor;
                  archive & service;
                  archive & reply;
                  archive & execution;
                  archive & callee;
                  archive & trid;
               })
            };

            namespace callee
            {

               //!
               //! Represents a service call. via tp(a)call, from the callee's perspective
               //!
               struct Call: public base_call
               {

                  Call() = default;

                  Call( Call&&) = default;
                  Call& operator = ( Call&&) = default;

                  Call( const Call&) = delete;
                  Call& operator = ( const Call&) = delete;

                  buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_call::marshal( archive);
                     archive & buffer;
                  })
               };

            } // callee

            namespace caller
            {
               //!
               //! Represents a service call. via tp(a)call, from the callers perspective
               //!
               struct Call: public base_call
               {

                  Call( buffer::Payload& buffer)
                        : buffer( buffer)
                  {
                  }

                  Call( Call&&) = default;
                  Call& operator = ( Call&&) = default;

                  Call( const Call&) = delete;
                  Call& operator = ( const Call&) = delete;

                  buffer::Payload& buffer;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_call::marshal( archive);
                     archive & buffer;
                  })
               };

            }

            //!
            //! Represent service reply.
            //!
            struct Reply :  basic_message< cServiceReply>
            {

               Reply() = default;
               Reply( Reply&&) noexcept = default;
               Reply& operator = ( Reply&&) noexcept = default;


               Reply( const Reply&) = delete;
               Reply& operator = ( const Reply&) = delete;

               int callDescriptor = 0;
               int returnValue = 0;
               long userReturnCode = 0;
               buffer::Payload buffer;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & callDescriptor;
                  archive & returnValue;
                  archive & userReturnCode;
                  archive & buffer;
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
         } // service
      } // message
   } //common
} // casual


#endif // SERVER_H_
