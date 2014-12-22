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
#include "common/internal/log.h"
#include "common/algorithm.h"


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

            bool send( id_type id, const Transport& transport, long flags)
            {
               //
               // We have to check and process (throw) pending signals before we might block
               //
               common::signal::handle();

               auto size = message::Transport::header_size + transport.size();

               auto result = msgsnd( id, &const_cast< Transport&>( transport).message, size, flags);

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
                     case EIDRM:
                     {
                        throw exception::queue::Unavailable{ "queue unavailable - id: " + std::to_string( id) + " - " + common::error::string()};
                     }
                     case ENOMEM:
                     {
                        throw exception::limit::Memory{ "id: " + std::to_string( id) + " - " + common::error::string()};
                     }
                     case EINVAL:
                     {
                        if( /* message.size() < MSGMAX  && */ transport.message.type > 0)
                        {
                           //
                           // The problem is with queue-id. We guess that it has been removed.
                           //
                           throw exception::queue::Unavailable{ "queue unavailable - id: " + std::to_string( id) + " - " + common::error::string()};
                        }
                        // we let it fall through to default
                     }
                     case EFAULT:
                     default:
                     {
                        throw common::exception::invalid::Argument( "invalid queue arguments - id: " + std::to_string( id) + " - " + common::error::string());
                     }
                  }
               }
               return true;

            }

            bool receive( id_type id, Transport& message, long flags)
            {
               //
               // We have to check and process (throw) pending signals before we might block
               //
               common::signal::handle();

               auto result = msgrcv( id, &message.message, message::Transport::message_max_size, 0, flags);

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
                     case EAGAIN:
                     {
                        return false;
                     }
                     case EIDRM:
                     {
                        throw exception::queue::Unavailable{ "queue removed - id: " + std::to_string( id) + " - " + common::error::string()};
                     }
                     default:
                     {
                        std::ostringstream msg;
                        msg << "ipc < [" << id << "] receive failed - message: " << message << " - flags: " << flags << " - " << common::error::string();
                        log::internal::ipc << msg.str() << std::endl;
                        throw exception::invalid::Argument( msg.str(), __FILE__, __LINE__);
                     }
                  }
               }
               message.m_size = result - Transport::header_size;

               return result > 0;
            }

            Transport::Transport()
            {
               memset( &message, 0, sizeof( Message));
            }

            //void* Transport::raw() { return &payload;}

            std::size_t Transport::size() const
            {
               return m_size;
            }

            std::ostream& operator << ( std::ostream& out, const Transport& value)
            {
               return out << "{type: " << value.message.type << " header: {correlation: " << common::uuid::string( value.message.header.correlation)
                     << ", count:" << value.message.header.count << "}, size: { header:" << sizeof( Transport::Header)
                     << ", payload: " << value.m_size << ", total: " << sizeof( Transport::Header) + value.m_size << ", max:" << Transport::message_max_size << "}}";
            }


            Complete::Complete() = default;

            Complete::Complete( Transport& transport)
               : type( transport.message.type), correlation( transport.message.header.correlation)
            {
               add( transport);
            }


            Complete::Complete( Complete&&) noexcept = default;
            Complete& Complete::operator = ( Complete&&) noexcept = default;

            void Complete::add( Transport& transport)
            {
               payload.insert(
                  std::end( payload),
                  std::begin( transport.message.payload),
                  std::begin( transport.message.payload) + transport.size());

               complete = transport.message.header.count == 0;

            }

            std::ostream& operator << ( std::ostream& out, const Complete& value)
            {
               return out << "{ type: " << value.type << " correlation: " << value.correlation << " size: "
                     << value.payload.size() << " complete: " << value.complete << '}';
            }

         }// message


         namespace send
         {
            Queue::Queue( id_type id) : internal::base_queue( id)
            {

            }


            Uuid Queue::send( const message::Complete& message, const long flags) const
            {
               //
               // partition the payload and send the resulting physical messages
               //

               ipc::message::Transport transport;
               transport.message.type = message.type;
               message.correlation.copy( transport.message.header.correlation);

               //
               // TODO: Redo this crap.
               //

               auto size = message.payload.size();
               if( size % message::Transport::payload_max_size == 0 && size > 0)
               {
                  transport.message.header.count = size / message::Transport::payload_max_size - 1;
               }
               else
               {
                  transport.message.header.count = size / message::Transport::payload_max_size;
               }


               auto partBegin = std::begin( message.payload);

               while( transport.message.header.count >= 0)
               {

                  auto partEnd = partBegin + message::Transport::payload_max_size > std::end( message.payload) ?
                        std::end( message.payload) : partBegin + message::Transport::payload_max_size;

                  transport.assign( partBegin, partEnd);

                  //
                  // send the physical message
                  //
                  if( ! message::send( m_id, transport, flags))
                  {
                     if( transport.message.header.count > 0)
                     {
                        // TODO: Partially sent message, what to do now?
                        log::internal::ipc << "TODO: Partially sent message, what to do now?\n";
                     }
                     return uuid::empty();
                  }

                  --transport.message.header.count;


                  partBegin = partEnd;
               }

               log::internal::ipc << "ipc > [" << id() << "] sent message - " << message << std::endl;


               return message.correlation;
            }


         } // send

         namespace receive
         {
            Queue::Queue()
               : internal::base_queue( msgget( IPC_PRIVATE, IPC_CREAT | 0660)),
                    m_path( file::unique( environment::directory::temporary() + "/ipc_queue_"))
            {
               if( m_id  == -1)
               {

                  throw exception::invalid::Argument( "ipc queue create failed - " + common::error::string(), __FILE__, __LINE__);
               }

               //
               // Write queue information
               //
               std::ofstream ipcQueueFile( m_path.path());

               ipcQueueFile << "id: " << m_id << std::endl
                     << "pid: " << process::id() <<  std::endl
                     << "path: " << process::path() << std::endl;


               log::internal::ipc << "created queue - id: " << m_id << " pid: " <<  process::id() << std::endl;

            }


            Queue::~Queue()
            {
               try
               {

                  if( ! m_cache.empty())
                  {
                     log::error << "queue: " << m_id << " has unconsumed messages in cache";
                  }

                  if( m_id != -1)
                  {
                     //
                     // Destroy queue
                     //
                     ipc::remove( m_id);
                     log::internal::ipc << "queue id: " << m_id << " removed - cache capacity: " << m_cache.capacity() << std::endl;
                  }

               }
               catch( ...)
               {
                  error::handler();
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

                     struct Correlation
                     {
                        Correlation( const Uuid& correlation) : m_correlation( correlation.get()) {}
                        Correlation( const Uuid::uuid_type& correlation) : m_correlation( correlation) {}

                        bool operator ()( const message::Complete& message) const
                        {
                           return message.correlation == m_correlation;
                        }
                     private:
                        const Uuid::uuid_type& m_correlation;
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

                     struct Types
                     {
                        typedef message::Complete::message_type_type message_type;
                        using message_types = std::vector< message_type>;

                        Types( message_types types) : m_types( std::move( types)) {}

                        bool operator () ( const message::Complete& message) const
                        {
                           return ! range::find( m_types, message.type).empty();
                        }


                     private:
                        message_types m_types;

                     };

                  } // find
               } // <unnamed>
            } // local


            std::vector< message::Complete> Queue::operator () ( const long flags)
            {
               std::vector< message::Complete> result;

               auto found = find( local::find::Complete(), flags);

               if( found)
               {
                  result.push_back( std::move( *found));
                  m_cache.erase( found.first);

                  log::internal::ipc << "ipc < [" << id() << "] received message - " << result.back() << std::endl;
               }

               return result;
            }

            std::vector< message::Complete> Queue::operator () ( message::Complete::message_type_type type, const long flags)
            {
               std::vector< message::Complete> result;

               auto found = find(
                     chain::And::link(
                           local::find::Type( type),
                           local::find::Complete()), flags);

               if( found)
               {
                  result.push_back( std::move( *found));
                  m_cache.erase( found.first);

                  log::internal::ipc << "ipc < [" << id() << "] received message - " << result.back() << std::endl;
               }

               return result;
            }

            std::vector< message::Complete> Queue::operator () ( const std::vector< type_type>& types, const long flags)
            {
               std::vector< message::Complete> result;

               auto found = find(
                     chain::And::link(
                           local::find::Types( types),
                           local::find::Complete()), flags);

               if( found)
               {
                  result.push_back( std::move( *found));
                  m_cache.erase( found.first);

                  log::internal::ipc << "ipc < [" << id() << "] received message - " << result.back() << std::endl;
               }

               return result;
            }


            std::vector< message::Complete> Queue::operator () ( const Uuid& correlation, const long flags)
            {
               std::vector< message::Complete> result;

               auto found = find(
                     chain::And::link(
                           local::find::Correlation( correlation),
                           local::find::Complete()), flags);

               if( found)
               {
                  result.push_back( std::move( *found));
                  m_cache.erase( found.first);

                  log::internal::ipc << "ipc < [" << id() << "] received message - " << result.back() << std::endl;
               }

               return result;
            }


            void Queue::clear()
            {
               while( ! operator() ( ipc::receive::Queue::cNoBlocking).empty())
                  ;

               m_cache.clear();
            }


            template< typename P>
            Queue::range_type Queue::find( P predicate, const long flags)
            {
               auto found = range::find_if( m_cache, predicate);

               message::Transport transport;

               while( ! found && message::receive( m_id, transport, flags))
               {
                  found = cache( transport);
                  found = range::find_if( found, predicate);
               }

               return found;
            }



            Queue::range_type Queue::cache( message::Transport& message)
            {
               auto found = range::find_if( m_cache,
                     local::find::Correlation( message.message.header.correlation));

               if( found)
               {
                  found->add( message);
               }
               else
               {
                  found.first = m_cache.emplace( std::end( m_cache), message);
                  found.last = std::end( m_cache);
               }
               return found;
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

                  if( ! file)
                  {
                     log::internal::ipc << "Failed to open broker queue configuration file" << std::endl;
                     throw common::exception::xatmi::SystemError( "Failed to open broker queue configuration file: " + brokerFile);
                  }

                  send::Queue::id_type id{ 0};
                  file >> id;

                  return send::Queue( id);

               }
            }
         }

         namespace broker
         {


            send::Queue& queue()
            {
               static send::Queue brokerQueue = local::initializeBrokerQueue();
               return brokerQueue;
            }

            send::Queue::id_type id()
            {
               return queue().id();
            }

         } // broker


         namespace receive
         {
            receive::Queue::id_type id()
            {
               return queue().id();
            }

            receive::Queue create()
            {
               receive::Queue queue;
               return queue;
            }


            receive::Queue& queue()
            {
               static receive::Queue singleton = create();
               return singleton;
            }

         } // receive

         void remove( platform::queue_id_type id)
         {

            if( msgctl( id, IPC_RMID, nullptr) != 0)
            {
               throw exception::invalid::Argument( "failed to rmove ipc-queue id: " + std::to_string( id ) + " - " + error::string(), __FILE__, __LINE__);
            }
         }

      } // ipc
	} // common
} // casual

