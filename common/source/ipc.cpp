//!
//! casual_ipc.cpp
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#include "common/ipc.h"

#include "common/environment.h"
#include "common/process.h"
#include "common/error.h"
#include "common/exception.h"
#include "common/signal.h"
#include "common/uuid.h"
#include "common/logger.h"


// TODO: header dependency to sf... not so good...
#include "sf/functional.h"


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

            base_queue::id_type base_queue::id() const
            {
               return m_id;
            }


         } // internal


         namespace message
         {

            Complete::Complete( Transport& transport)
               : type( transport.m_payload.m_type), correlation( transport.m_payload.m_header.m_correlation)
            {
               add( transport);
            }

            void Complete::add( Transport& transport)
            {
               payload.insert(
                  std::end( payload),
                  std::begin( transport.m_payload.m_payload),
                  std::begin( transport.m_payload.m_payload) + transport.paylodSize());

               complete = transport.m_payload.m_header.m_count == 0;

            }
         }// message


         namespace send
         {
            Queue::Queue( id_type id) : internal::base_queue( id)
            {

            }

            bool Queue::send( message::Complete& message, const long flags) const
            {
               //
               // partition the payload and send the resulting physical messages
               //

               ipc::message::Transport transport;
               transport.m_payload.m_type = message.type;
               message.correlation.copy( transport.m_payload.m_header.m_correlation);

               //
               // This crap got to do a lot better...
               //
               auto size = message.payload.size();
               if( size % message::Transport::payload_max_size == 0 && size > 0)
               {
                  transport.m_payload.m_header.m_count = size / message::Transport::payload_max_size - 1;
               }
               else
               {
                  transport.m_payload.m_header.m_count = size / message::Transport::payload_max_size;
               }


               auto partBegin = std::begin( message.payload);

               while( partBegin != std::end( message.payload))
               {

                  auto partEnd = partBegin + message::Transport::payload_max_size > std::end( message.payload) ?
                        std::end( message.payload) : partBegin + message::Transport::payload_max_size;

                  std::copy( partBegin, partEnd, std::begin( transport.m_payload.m_payload));

                  transport.paylodSize( partEnd - partBegin);

                  //
                  // send the physical message
                  //
                  if( ! send( transport, flags))
                  {
                     if( transport.m_payload.m_header.m_count > 0)
                     {
                        // TODO: Partially sent message, what to do now?
                     }
                     return false;
                  }

                  --transport.m_payload.m_header.m_count;


                  partBegin = partEnd;
               }

               return true;
            }

            bool Queue::send( message::Transport& message, const long flags) const
            {
               //
               // We have to check and process (throw) pending signals before we might block
               //
               common::signal::handle();
               auto result = msgsnd( m_id, message.raw(), message.size(), flags);

               if( result == -1)
               {
                  switch( errno)
                  {
                     case EINTR:
                     {
                        common::signal::handle();
                        return false;
                     }
                     case EAGAIN:
                     {
                        return false;
                     }
                     default:
                     {
                        throw common::exception::QueueSend( "id: " + std::to_string( m_id) + " - " + common::error::stringFromErrno());
                     }
                  }
               }
               return true;
            }

         } // send

         namespace receive
         {

            Queue::Queue()
               : internal::base_queue( msgget( IPC_PRIVATE, IPC_CREAT | 0660)),
                    m_scopedPath( environment::directory::temporary() + "/ipc_queue_" + Uuid::make().string())
            {
               //
               // Write queue information
               //
               std::ofstream ipcQueueFile( m_scopedPath.path());

               ipcQueueFile << "id: " << m_id << std::endl
                     << "pid: " << process::id() <<  std::endl
                     << "path: " << environment::file::executable() << std::endl;


               if( m_id  == -1)
               {
                  throw common::exception::QueueFailed( common::error::stringFromErrno());
               }

            }


            Queue::~Queue()
            {
               if( ! m_cache.empty())
               {
                  logger::error << "queue: " << m_id << " has unconsumed messages in cache";
               }

               //
               // Destroy queue
               //
               if( m_id != 0)
               {
                  if( msgctl( m_id, IPC_RMID, 0) == -1)
                  {
                     logger::error << "failed to remove queue: " << m_id << " - " << common::error::stringFromErrno();
                  }
               }
               else
               {
                  logger::debug << "queue id: " << m_id << " removed";
               }
            }


            namespace local
            {
               namespace
               {
                  namespace find
                  {
                     struct Complete
                     {
                        bool operator ()( const message::Complete& message) const
                        {
                           return message.complete;
                        }
                     };


                     struct Type
                     {
                        typedef message::Complete::message_type_type message_type;
                        Type( message_type type) : m_type( type) {}

                        bool operator () ( const message::Complete& message) const
                        {
                           return message.type == m_type;
                        }


                     private:
                        message_type m_type;

                     };



                     struct Correlation
                     {
                        Correlation( message::Transport::correalation_type& correlation) : m_correlation( correlation) {}

                        bool operator ()( const message::Complete& complete) const
                        {
                           return complete.correlation == m_correlation;
                        }
                     private:
                        message::Transport::correalation_type& m_correlation;
                     };


                     typedef sf::functional::Chain< sf::functional::link::And> And;

                  } // find
               } // <unnamed>
            } // local




            std::vector< message::Complete> Queue::operator () ( const long flags)
            {
               std::vector< message::Complete> result;

               auto findIter = find( local::find::Complete(), flags);

               if( findIter != std::end( m_cache))
               {
                  result.push_back( std::move( *findIter));
                  m_cache.erase( findIter);
               }

               return result;
            }

            std::vector< message::Complete> Queue::operator () ( message::Complete::message_type_type type, const long flags)
            {
               std::vector< message::Complete> result;

               auto findIter = find(
                     local::find::And::link(
                           local::find::Type( type),
                           local::find::Complete()), flags);

               if( findIter != std::end( m_cache))
               {
                  result.push_back( std::move( *findIter));
                  m_cache.erase( findIter);
               }

               return result;
            }

            template< typename P>
            Queue::cache_type::iterator Queue::find( P predicate, const long flags)
            {
               auto findIter = std::find_if( std::begin( m_cache), std::end( m_cache), predicate);

               message::Transport transport;

               while( findIter == std::end( m_cache) && receive( transport, flags))
               {
                  findIter = cache( transport);
                  findIter = std::find_if( findIter, std::end( m_cache), predicate);
               }

               return findIter;
            }


            bool Queue::receive( message::Transport& message, const long flags)
            {
               //
               // We have to check and process (throw) pending signals before we might block
               //
               common::signal::handle();
               auto result = msgrcv( m_id, message.raw(), message.size(), 0, flags);

               if( result == -1)
               {
                  switch( errno)
                  {
                     case EINTR:
                     {
                        common::signal::handle();
                        return false;
                     }
                     case ENOMSG:
                     {
                        return false;
                     }
                     default:
                     {
                        throw common::exception::QueueReceive( common::error::stringFromErrno() + " - id: " + std::to_string( m_id) + " - flags: " + std::to_string( flags) + " - size: " + std::to_string( message.size()));
                     }
                  }
               }

               message.size( result);

               return result > 0;
            }

            Queue::cache_type::iterator Queue::cache( message::Transport& message)
            {
               auto findIter = std::find_if( std::begin( m_cache), std::end( m_cache),
                     local::find::Correlation( message.m_payload.m_header.m_correlation));

               if( findIter != std::end( m_cache))
               {
                  findIter->add( message);
               }
               else
               {
                  return m_cache.emplace( std::end( m_cache), message);
               }
               return findIter;
            }

         } // receive

         namespace local
         {
            namespace
            {
               send::Queue initializeBrokerQueue()
               {
                  static const std::string brokerFile = common::environment::file::brokerQueue();

                  std::ifstream file( brokerFile.c_str());

                  if( file.fail())
                  {
                     throw common::exception::xatmi::SystemError( "Failed to open domain configuration file: " + brokerFile);
                  }

                  send::Queue::id_type id;
                  file >> id;

                  return send::Queue( id);

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

