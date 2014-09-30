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
               struct Reply : basic_messsage< cServerConnectReply>
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

            struct Advertise : basic_messsage< cServiceAdvertise>
            {

               std::string serverPath;
               server::Id server;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & serverPath;
                  archive & server;
                  archive & services;
               })
            };

            struct Unadvertise : basic_messsage< cServiceUnadvertise>
            {
               server::Id server;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & server;
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
                  struct Request : basic_messsage< cServiceNameLookupRequest>
                  {
                     Request() = default;
                     Request( Request&&) = default;
                     Request& operator = ( Request&&) = default;

                     std::string requested;
                     server::Id server;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & requested;
                        archive & server;
                     })
                  };

                  //!
                  //! Represent "service-name-lookup" response.
                  //!
                  struct Reply : basic_messsage< cServiceNameLookupReply>
                  {

                     Service service;

                     std::vector< server::Id> server;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & service;
                        archive & server;
                     })
                  };
               } // lookup
            } // name

            struct base_call : basic_messsage< cServiceCall>
            {

               base_call() = default;

               base_call( base_call&&) = default;
               base_call& operator = ( base_call&&) = default;

               base_call( const base_call&) = delete;
               base_call& operator = ( const base_call&) = delete;

               int callDescriptor = 0;
               Service service;
               server::Id reply;
               common::Uuid callId;
               std::string callee;
               Transaction transaction;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & callDescriptor;
                  archive & service;
                  archive & reply;
                  archive & callId;
                  archive & callee;
                  archive & transaction;
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
            struct Reply :  basic_messsage< cServiceReply>
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
            struct ACK : basic_messsage< cServiceAcknowledge>
            {

               std::string service;
               server::Id server;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & service;
                  archive & server;
               })
            };
         } // service
      } // message
   } //common
} // casual


#endif // SERVER_H_
