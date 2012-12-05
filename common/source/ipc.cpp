//!
//! casual_ipc.cpp
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#include "common/ipc.h"

#include "utility/environment.h"
#include "utility/error.h"
#include "utility/exception.h"
#include "utility/signal.h"
#include "utility/uuid.h"



#include <fstream>

#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


// temp
#include <iostream>




namespace casual
{
   namespace common
   {
      namespace ipc
      {
         namespace internal
         {

            base_queue::queue_key_type base_queue::getKey() const
            {
               return m_key;
            }


         }

         namespace send
         {
            Queue::Queue(queue_key_type key)
            {
               m_key = key;
               m_id = msgget( m_key, 0220);

               if( m_id  == -1)
               {
                  throw utility::exception::QueueFailed( utility::error::stringFromErrno());
               }
            }

            bool Queue::send( message::Transport& message, const long flags) const
            {

               ssize_t result = msgsnd( m_id, message.raw(), message.size(), flags);

               if( result == -1)
               {
                  switch( errno)
                  {
                     case EINTR:
                     {
                        utility::signal::handle();
                        return false;
                     }
                     case ENOMSG:
                     {
                        return false;
                     }
                     default:
                     {
                        throw utility::exception::QueueSend( utility::error::stringFromErrno());
                     }
                  }
               }

               return true;
            }



         }

         namespace receive
         {

            Queue::Queue()
               : m_scopedPath( utility::environment::getTemporaryPath() + "/ipc_queue_" + utility::Uuid().getString())
            {
               //
               // Create queue
               //
               std::ofstream ipcQueueFile( m_scopedPath.path().c_str());

               m_key = ftok( m_scopedPath.path().c_str(), 'X');
               m_id = msgget( m_key, 0660 | IPC_CREAT);

               ipcQueueFile << "key: " << m_key << "\nid:  " << m_id << std::endl;

               if( m_id  == -1)
               {
                  throw utility::exception::QueueFailed( utility::error::stringFromErrno());
               }

            }


            Queue::~Queue()
            {
               //
               // Destroy queue
               //
               if( m_id != 0)
               {
                  msgctl( m_id, IPC_RMID, 0);
               }
            }



            bool Queue::receive( message::Transport& message, const long flags) const
            {
               ssize_t result = msgrcv( m_id, message.raw(), message.size(), 0, flags);

               if( result == -1)
               {
                  switch( errno)
                  {
                     case EINTR:
                     {
                        utility::signal::handle();
                        return false;
                     }
                     case ENOMSG:
                     {
                        return false;
                     }
                     default:
                     {
                        throw utility::exception::QueueReceive( utility::error::stringFromErrno());
                     }
                  }
               }

               message.size( result);

               return result > 0;
            }
         } // receive

         namespace local
         {
            namespace
            {
               send::Queue initializeBrokerQueue()
               {
                  static const std::string brokerFile = utility::environment::getBrokerQueueFileName();

                  std::ifstream file( brokerFile.c_str());

                  if( file.fail())
                  {
                     throw utility::exception::xatmi::SystemError( "Failed to open domain configuration file: " + brokerFile);
                  }

                  send::Queue::queue_key_type key;
                  file >> key;

                  return send::Queue( key);

               }
            }
         }


         send::Queue& getBrokerQueue()
         {
            static send::Queue brokerQueue = local::initializeBrokerQueue();
            return brokerQueue;
         }

         receive::Queue& getReceiveQueue()
         {
            static receive::Queue singleton;
            return singleton;
         }

      } // ipc
	} // common
} // casual






