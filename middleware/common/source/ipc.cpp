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
#include "common/message/type.h"


#include <fstream>

#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


// temp
//#include <iostream>




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
            inline bool send_error( int code, id_type id, const Transport& transport)
            {
               switch( code)
               {
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
                     throw exception::invalid::Argument( "invalid queue arguments - id: " + std::to_string( id) + " - " + common::error::string());
                  }
               }
            }

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
                  auto code = errno;

                  switch( code)
                  {
                     case EINTR:
                     {
                        common::signal::handle();

                        //
                        // we got a signal we don't have a handle for
                        // We continue
                        //
                        return send( id, transport, flags);
                     }
                     default:
                     {
                        return send_error( code, id, transport);
                     }
                  }
               }

               return true;
            }

            inline bool receive_error( int code, id_type id, const Transport& transport, long flags)
            {
               switch( code)
               {
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
                     msg << "ipc < [" << id << "] receive failed - transport: " << transport << " - flags: " << flags << " - " << common::error::string();
                     log::internal::ipc << msg.str() << std::endl;
                     throw exception::invalid::Argument( msg.str(), __FILE__, __LINE__);
                  }
               }
            }

            bool receive( id_type id, Transport& transport, long flags)
            {
               //
               // We have to check and process (throw) pending signals before we might block
               //
               common::signal::handle();

               auto result = msgrcv( id, &transport.message, message::Transport::message_max_size, 0, flags);

               if( result == -1)
               {
                  auto code = errno;

                  switch( code)
                  {
                     case EINTR:
                     {
                        common::signal::handle();

                        //
                        // we got a signal we don't have a handle for
                        // We continue
                        //
                        return receive( id, transport, flags);
                     }
                     default:
                     {
                        return receive_error( code, id, transport, flags);
                     }
                  }
               }
               transport.m_size = result - Transport::header_size;

               return result > 0;
            }

            namespace ignore
            {
               namespace signal
               {
                  bool send( id_type id, const Transport& transport, long flags)
                  {
                     //
                     // We don't check signals..., and we block any coming signals.
                     //

                     common::signal::thread::scope::Block signal_block;


                     auto size = message::Transport::header_size + transport.size();

                     auto result = msgsnd( id, &const_cast< Transport&>( transport).message, size, flags);

                     if( result == -1)
                     {
                        //
                        // Will throw on EINTR
                        //
                        return send_error( errno, id, transport);
                     }
                     return true;
                  }

                  bool receive( id_type id, Transport& transport, long flags)
                  {
                     //
                     // We don't check signals..., and we block any coming signals.
                     //
                     common::signal::thread::scope::Block signal_block;

                     auto result = msgrcv( id, &transport.message, message::Transport::message_max_size, 0, flags);

                     if( result == -1)
                     {
                        //
                        // Will throw on EINTR
                        //
                        return receive_error( errno, id, transport, flags);
                     }

                     transport.m_size = result - Transport::header_size;

                     return result > 0;
                  }

               } // signal
            } // ignore

            Transport::Transport()
            {
               memset( &message, 0, sizeof( message_t));
            }

            //void* Transport::raw() { return &payload;}

            std::size_t Transport::size() const
            {
               return m_size;
            }


            bool Transport::last() const
            {
               return message.header.complete.index * Transport::payload_max_size >= message.header.complete.size;
            }

            std::ostream& operator << ( std::ostream& out, const Transport& value)
            {
               return out << "{type: " << value.message.type << " header: {correlation: " << common::uuid::string( value.message.header.correlation)
                     << ", total: { size: " << value.message.header.complete.size << ", index: "
                     << value.message.header.complete.index << " last: " << std::boolalpha << value.last() << "}}, size: { header:" << sizeof( Transport::header_t)
                     << ", payload: " << value.m_size << ", total: " << sizeof( Transport::header_t) + value.m_size << ", max:" << Transport::message_max_size << "}}";
            }


            Complete::Complete( message_type_type type, const Uuid& correlation)
              : type( type), correlation( correlation), offset( 0)
            {
               payload.reserve( 128);
            }

            Complete::Complete( Transport& transport)
               : type( transport.message.type), correlation( transport.message.header.correlation),
                 payload( transport.message.header.complete.size), offset( transport.size())
            {
               std::copy(
                  std::begin( transport.message.payload),
                  std::begin( transport.message.payload) + offset,
                  std::begin( payload));
            }


            Complete::Complete( Complete&&) noexcept = default;


            Complete& Complete::operator = ( Complete&& rhs) noexcept
            {
               //
               // vector::operator = (vector&&) only support noexecpt if
               // allocator support it. It seems that it's not supported on our
               // target platform currently...
               //
               type = std::move( rhs.type);
               correlation = std::move( rhs.correlation);
               payload = std::move( rhs.payload);
               offset = std::move( rhs.offset);
               return *this;
            }



            bool Complete::complete() const
            {
               return offset == payload.size();
            }

            std::ostream& operator << ( std::ostream& out, const Complete& value)
            {
               return out << "{ type: " << value.type << ", correlation: " << value.correlation << ", size: "
                     << value.payload.size() << std::boolalpha << ", complete: " << value.complete() << '}';
            }


            void Complete::add( Transport& transport)
            {
               assert( payload.size() == transport.message.header.complete.size);

               if( transport.message.header.complete.index * Transport::payload_max_size < offset)
               {
                  log::internal::ipc << "transport " << transport << " is already consumed - action: discard" << std::endl;
                  return;
               }

               std::copy(
                  std::begin( transport.message.payload),
                  std::begin( transport.message.payload) + transport.size(),
                  std::begin( payload) + offset);

               offset += transport.size();
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
               transport.message.header.complete.size = message.payload.size();


               //
               // Calculate number of parts
               //
               auto calculate = [&]() -> std::uint64_t {
                  if( message.payload.empty())
                  {
                     return 1;
                  }
                  else
                  {
                     auto chunks = std::div( message.payload.size(), message::Transport::payload_max_size);
                     return chunks.quot + ( chunks.rem == 0 ? 0 : 1);
                  }
               };

               const auto parts = calculate();

               auto partBegin = std::begin( message.payload);

               while( transport.message.header.complete.index < parts)
               {

                  auto partEnd = ( partBegin + message::Transport::payload_max_size) > std::end( message.payload) ?
                        std::end( message.payload) : partBegin + message::Transport::payload_max_size;

                  transport.assign( partBegin, partEnd);

                  //
                  // send the physical message
                  //
                  if( ! message::send( m_id, transport, flags))
                  {
                     return uuid::empty();
                  }

                  partBegin = partEnd;

                  ++transport.message.header.complete.index;
               }

               log::internal::ipc << "ipc > [" << id() << "] sent message - " << message << std::endl;


               return message.correlation;
            }


         } // send

         namespace receive
         {
            Queue::Queue()
               : internal::base_queue( msgget( IPC_PRIVATE, IPC_CREAT | 0660)),
                    m_path( file::name::unique( environment::directory::temporary() + "/ipc_queue_"))
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
                           return message.complete();
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

            void Queue::flush()
            {
               //
               // We try to find a non existing message, until the ipc-queue is consumed
               // and in the cache
               //
               find(
                     &message::ignore::signal::receive,
                     chain::And::link(
                           local::find::Type( common::message::Type::cFlushIPC),
                           local::find::Complete()), cNoBlocking);
            }

            void Queue::discard( const Uuid& correlation)
            {

               auto found = find( local::find::Correlation( correlation), cNoBlocking);

               if( found)
               {
                  m_cache.erase( found.first);
               }
               else if( ! range::find( m_discarded, correlation))
               {
                  m_discarded.push_back( correlation);
               }
            }

            void Queue::clear()
            {
               while( ! operator() ( ipc::receive::Queue::cNoBlocking).empty())
                  ;

               m_cache.clear();
            }


            template< typename IPC, typename P>
            Queue::range_type Queue::find( IPC ipc, P predicate, const long flags)
            {
               auto found = range::find_if( m_cache, predicate);

               message::Transport transport;

               while( ! found && ipc( m_id, transport, flags))
               {
                  //
                  // Check if the message should be discarded
                  //
                  if( ! discard( transport))
                  {
                     found = cache( transport);
                     found = range::find_if( found, predicate);
                  }
               }

               return found;
            }

            template< typename P>
            Queue::range_type Queue::find( P predicate, const long flags)
            {
               return find( &message::receive, predicate, flags);
            }


            bool Queue::discard( message::Transport& transport)
            {
               auto found = range::find( m_discarded, transport.message.header.correlation);

               if( found)
               {
                  //
                  // If transport is the last part in the message, we don't need to
                  // discard any more transports...
                  //
                  if( transport.last())
                  {
                     m_discarded.erase( found.first);
                  }
                  return true;
               }
               return false;
            }

            Queue::range_type Queue::cache( message::Transport& transport)
            {
               auto found = range::find_if( m_cache,
                     local::find::Correlation( transport.message.header.correlation));

               if( found)
               {
                  found->add( transport);
               }
               else
               {
                  // m_cache.push_back( message::Complete( transport));
                  //found.first = found.last - 1;

                  found.first = m_cache.emplace( std::end( m_cache), transport);
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
                     throw common::exception::xatmi::System( "Failed to open broker queue configuration file: " + brokerFile);
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



         bool remove( platform::queue_id_type id)
         {
            if( msgctl( id, IPC_RMID, nullptr) != 0)
            {
               return false;
            }
            return true;
         }

         bool remove( const process::Handle& owner)
         {
            struct msqid_ds info;

            if( msgctl( owner.queue, IPC_STAT, &info) != 0)
            {
               return false;
            }
            if( info.msg_lrpid == owner.pid)
            {
               return remove( owner.queue);
            }
            return false;
         }

      } // ipc
	} // common
} // casual

