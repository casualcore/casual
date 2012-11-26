//!
//! casual_ipc_messages.h
//!
//! Created on: Apr 25, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_MESSAGES_H_
#define CASUAL_MESSAGES_H_

#include "common/ipc.h"
#include "common/buffer_context.h"

#include "utility/platform.h"
#include "utility/exception.h"


#include <vector>



namespace casual
{
   namespace common
   {

      namespace message
      {
         struct Service
         {
            typedef int Seconds;

            Service() = default;

            explicit Service( const std::string& name_) : name( name_), timeout( 0) {}

            std::string name;
            Seconds timeout = 0;

            template< typename A>
            void marshal( A& archive)
            {
               archive & name;
               archive & timeout;
            }
         };

         namespace server
         {

            //!
            //! Represents id for a server.
            //!
            struct Id
            {
               typedef utility::platform::pid_type pid_type;
               typedef ipc::message::Transport::queue_key_type queue_key_type;

               Id() : pid( utility::platform::getProcessId()) {}

               queue_key_type queue_key;
               pid_type pid;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & queue_key;
                  archive & pid;
               }
            };


            struct Disconnect
            {
               enum
               {
                  message_type = 3
               };

               Id serverId;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & serverId;
               }
            };
         }


         namespace service
         {

            struct Advertise
            {
               enum
               {
                  message_type = 4
               };

               std::string serverPath;
               server::Id serverId;
               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & serverPath;
                  archive & serverId;
                  archive & services;
               }
            };

            struct Unadvertise
            {
               enum
               {
                  message_type = 5
               };

               server::Id serverId;
               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & serverId;
                  archive & services;
               }
            };


            namespace name
            {
               namespace lookup
               {
                  //!
                  //! Represent "service-name-lookup" request.
                  //!
                  struct Request
                  {
                     enum
                     {
                        message_type = 10
                     };

                     std::string requested;
                     std::string current;
                     server::Id server;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        archive & requested;
                        archive & current;
                        archive & server;
                     }
                  };


                  //!
                  //! Represent "service-name-lookup" response.
                  //!
                  struct Reply
                  {

                     enum
                     {
                        message_type = 11
                     };

                     Service service;

                     std::vector< server::Id> server;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        archive & service;
                        archive & server;
                     }
                  };
               }
            }

            //!
            //! Represents a service call. via tp(a)call
            //!
            struct Call
            {
               enum
               {
                  message_type = 20
               };

               Call( buffer::Buffer& buffer) : m_buffer( buffer) {}

               int callDescriptor = 0;
               Service service;
               server::Id reply;

               buffer::Buffer& buffer()
               {
                  return m_buffer;
               }

               const buffer::Buffer& buffer() const
               {
                  return m_buffer;
               }

               template< typename A>
               void marshal( A& archive)
               {
                  archive & callDescriptor;
                  archive & service;
                  archive & reply;
                  archive & m_buffer;
               }

            private:
               buffer::Buffer& m_buffer;
            };

            //!
            //! Represent service reply.
            //!
            struct Reply
            {
               enum
               {
                  message_type = 21
               };

               Reply() = default;

               Reply( buffer::Buffer& buffer) : m_buffer( &buffer) {}

               void setBuffer( buffer::Buffer& buffer)
               {
                  m_buffer = &buffer;
               }

               buffer::Buffer& getBuffer()
               {
                  return *m_buffer;
               }


               int callDescriptor = 0;
               int returnValue = 0;
               long userReturnCode = 0;

               template< typename A>
               void marshal( A& archive)
               {
                  if( m_buffer == nullptr)
                  {
                     throw utility::exception::xatmi::SystemError( "Not a valid buffer for ServiceReply");
                  }
                  archive & callDescriptor;
                  archive & returnValue;
                  archive & userReturnCode;
                  archive & *m_buffer;
               }

            private:
               buffer::Buffer* m_buffer = nullptr;
            };

            //!
            //! Represent the reply to the broker when a server is done handling
            //! a service-call and is ready for new calls
            //!
            struct ACK
            {
               enum
               {
                  message_type = 22
               };


               std::string service;
               server::Id server;


               template< typename A>
               void marshal( A& archive)
               {
                  archive & service;
                  archive & server;
               }
            };
         }


         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         ipc::message::Transport::message_type_type type( const M& message)
         {
            return M::message_type;
         }

      } // message
	} //common
} // casual



#endif /* CASUAL_IPC_MESSAGES_H_ */
