//!
//! ipc.cpp
//!
//! Created on: May 30, 2014
//!     Author: Lazan
//!

#include "common/mockup/ipc.h"


#include "common/queue.h"
#include "common/environment.h"
#include "common/file.h"
#include "common/signal.h"

#include "common/internal/log.h"
#include "common/internal/trace.h"


#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <fstream>

namespace casual
{

   namespace common
   {

      namespace ipc
      {
         namespace message
         {
            //!
            //! Overload for ipc::message::Complete
            //!
            template< typename M>
            void casual_marshal_value( Complete& value, M& marshler)
            {
              marshler << value.complete;
              marshler << value.correlation;
              marshler << value.payload;
              marshler << value.type;

            }

            template< typename M>
            void casual_unmarshal_value( Complete& value, M& unmarshler)
            {
              unmarshler >> value.complete;
              unmarshler >> value.correlation;
              unmarshler >> value.payload;
              unmarshler >> value.type;
            }
         } // message
      } // ipc


      namespace mockup
      {
         namespace ipc
         {
            namespace local
            {
               namespace
               {

                  namespace message
                  {
                     enum Type
                     {

                        cMockupConnectReply  = 10000, // avoid conflict with real messages
                        cMockupDisconnect,
                        cMockupStart,
                        cMockupMessageToSend,
                        cMockupFetchRequest,
                        cMockupFetchReply,

                     };

                     template< message::Type type>
                     struct basic_messsage
                     {
                        enum
                        {
                           message_type = type
                        };
                     };

                     template< message::Type type>
                     struct basic_queue_id : basic_messsage< type>
                     {
                        typedef platform::queue_id_type queue_id_type;

                        queue_id_type queue_id = 0;

                        template< typename A>
                        void marshal( A& archive)
                        {
                           archive & queue_id;
                        }
                     };

                     using ConnectReply = basic_queue_id< cMockupConnectReply>;

                     using Disconnect =  basic_messsage< cMockupDisconnect>;


                     using Start =  basic_messsage< cMockupStart>;

                     struct ToSend : basic_messsage< cMockupMessageToSend>
                     {
                        typedef platform::queue_id_type queue_id_type;

                        queue_id_type destination;
                        common::ipc::message::Complete message;

                        template< typename A>
                        void marshal( A& archive)
                        {
                           archive & destination;
                           archive & message;
                        }

                     };


                     namespace fetch
                     {
                        struct Request : basic_messsage< cMockupFetchRequest>
                        {
                           typedef platform::queue_id_type queue_id_type;
                           typedef common::ipc::receive::Queue::type_type type_type;

                           queue_id_type destination = 0;
                           std::vector< type_type> types;

                           template< typename A>
                           void marshal( A& archive)
                           {
                              archive & destination;
                              archive & types;
                           }
                        };


                     } // fetch



                  } // message


                  template< typename P>
                  struct basic_thread_queue
                  {
                     void operator () ( common::ipc::receive::Queue::id_type id)
                     {
                        common::Trace trace( common::log::internal::ipc, "basic_thread_queue::operator ()");

                        log::internal::ipc << "started with queue id " << id << " from main thread" << std::endl;

                        common::ipc::receive::Queue receiveQueue;


                        log::internal::ipc << "created ipc queue " << receiveQueue.id() << std::endl;

                        //
                        // Write this threads queue id to main-thread, so we
                        // establish communications
                        //
                        {
                           message::ConnectReply reply;
                           reply.queue_id = receiveQueue.id();

                           common::queue::blocking::Writer write( id);
                           write( reply);

                           log::internal::ipc << "written queue id " << receiveQueue.id() << " to main thread" << std::endl;
                        }

                        //
                        // Let the implementation do it's thing...
                        //
                        P()( receiveQueue);
                     }
                  };


                  template< typename P>
                  struct basic_implementation
                  {
                     basic_implementation() : m_thread( m_functor, m_receiveQueue.id())
                     {
                        try
                        {
                           common::queue::blocking::Reader reader( m_receiveQueue);

                           //
                           // Wait for thread to write it's queue id
                           //
                           message::ConnectReply reply;
                           reader( reply);

                           m_threadQueueId = reply.queue_id;
                           log::internal::ipc << "received queue id " << m_threadQueueId << " from thread" << std::endl;

                        }
                        catch( std::exception& exception)
                        {
                           log::debug << "exception: " << exception.what() << std::endl;
                           throw;
                        }

                     }

                     ~basic_implementation()
                     {
                        common::queue::non_blocking::Writer write( m_threadQueueId);
                        message::Disconnect disconnect;
                        write( disconnect);

                        m_thread.join();
                     }


                     template< typename M>
                     void send( M&& message) const
                     {
                        common::queue::blocking::Writer write( m_threadQueueId);
                        write( message);
                     }

                     template< typename M>
                     bool try_send( M&& message) const
                     {
                        common::queue::non_blocking::Writer write( m_threadQueueId);
                        return write( message);
                     }


                     common::ipc::receive::Queue& ipc() { return m_receiveQueue;}

                     typedef platform::queue_id_type queue_id_type;

                     common::ipc::receive::Queue m_receiveQueue;
                     basic_thread_queue< P> m_functor;
                     std::thread m_thread;
                     queue_id_type m_threadQueueId;

                  };


                  struct Sender
                  {
                     struct Send
                     {
                        struct Queue
                        {
                           Queue()
                           {
                              work = true;
                           }
                           void push_back( message::ToSend&& message) const
                           {
                              std::lock_guard< std::mutex> lock( m_mutext);

                              m_messages.push_back( std::move( message));

                              log::internal::ipc << "Q push_back size: " << m_messages.size() << std::endl;
                           }

                           std::vector< message::ToSend> pop_front() const
                           {
                              std::lock_guard< std::mutex> lock( m_mutext);
                              std::vector< message::ToSend> result;

                              if( ! m_messages.empty())
                              {
                                 result.push_back( std::move( m_messages.front()));
                                 m_messages.pop_front();
                              }
                              log::internal::ipc << "Q pop_front size: " << m_messages.size() << std::endl;

                              return result;
                           }

                           std::size_t size() const
                           {
                              std::lock_guard< std::mutex> lock( m_mutext);
                              return m_messages.size();
                           }

                           mutable std::atomic< bool> work;

                        private:
                           mutable std::deque< message::ToSend> m_messages;
                           mutable std::mutex m_mutext;

                        };

                        void operator () ( const Queue& queue) const
                        {
                           log::internal::ipc << "Send::operator () called" << std::endl;

                           std::size_t numberOfMessages = 0;

                           while( true)
                           {
                              auto message = queue.pop_front();

                              if( message.empty())
                              {
                                 // we're done

                                 if( ! queue.work)
                                 {
                                    log::internal::ipc << "Send is done" << std::endl;
                                    return;
                                 }
                                 process::sleep( std::chrono::milliseconds( 1));

                              }
                              else
                              {
                                 common::queue::blocking::Writer write( message.front().destination);
                                 write.send( message.front().message);


                                 log::internal::ipc << "total sent messages: " << ++numberOfMessages << " - queue size: " << queue.size() << std::endl;
                              }
                           }
                        }
                     };


                     void operator () ( common::ipc::receive::Queue& receiveQueue)
                     {

                        //
                        // Start the message pump
                        //
                        common::queue::blocking::Reader reader( receiveQueue);


                        Send send;
                        Send::Queue queue;
                        std::thread sender = std::thread{ send, std::ref( queue)};

                        while( true)
                        {
                           try
                           {
                              auto next = reader.next();

                              switch( next.type())
                              {
                                 case message::cMockupDisconnect:
                                 {
                                    log::internal::ipc << "disconnect received - stop" << std::endl;
                                    queue.work = false;
                                    sender.join();
                                    return;
                                 }
                                 case message::cMockupMessageToSend:
                                 {
                                    Trace trace( log::internal::ipc, "case message::cMockupMessageToSend:");
                                    message::ToSend message;
                                    next >> message;

                                    queue.push_back( std::move( message));
                                    break;
                                 }
                                 default:
                                 {
                                    log::error << "unknown message " << next.type() << "- discard" << std::endl;
                                    break;
                                 }
                              }
                           }
                           catch( const exception::signal::User&)
                           {
                              log::internal::ipc << "signal received - read from input queue" << std::endl;
                           }
                        }
                     }
                  };


                  struct Receiver
                  {
                     void operator () ( common::ipc::receive::Queue& receiveQueue)
                     {

                        //
                        // Start the message pump
                        //
                        common::queue::blocking::Reader reader( receiveQueue);

                        std::vector< common::ipc::message::Complete> messages;

                        while( true)
                        {
                           auto next = reader.next();

                           switch( next.type())
                           {
                              case message::cMockupDisconnect:
                              {
                                 log::internal::ipc << "disconnect received - stop" << std::endl;
                                 return;
                              }
                              case message::fetch::Request::message_type:
                              {
                                 message::fetch::Request message;
                                 next >> message;

                                 auto found = range::find_if(
                                       messages, [&](const common::ipc::message::Complete& m)
                                       {
                                          if( message.types.empty()) { return true;}

                                          return ! range::find( message.types, m.type).empty();
                                       }
                                       );

                                 if( found)
                                 {
                                    common::queue::non_blocking::Writer write( message.destination);
                                    write.send( *found);
                                    messages.erase( found.first);
                                 }
                                 else
                                 {
                                    log::error << "failed to retreive message - types: " <<  range::make( message.types) << std::endl;
                                 }
                                 break;
                              }
                              default:
                              {
                                 messages.push_back( next.release());
                                 break;
                              }
                           }
                        }
                     }
                  };



               } // <unnamed>
            } // local



            struct Sender::Implementation : public local::basic_implementation< local::Sender>
            {
               using base_type = local::basic_implementation< local::Sender>;

               void add( queue_id_type destination, const common::ipc::message::Complete& message)
               {
                  //signal::clear();

                  local::message::ToSend toSend;
                  toSend.destination = destination;

                  toSend.message.complete = message.complete;
                  toSend.message.correlation = message.correlation;
                  toSend.message.payload = message.payload;
                  toSend.message.type = message.type;

                  send( toSend);
                  //start();
               }

               void start()
               {
                  send( local::message::Start{});
               }
            };


            Sender::Sender()
            {

            }

            Sender::~Sender()
            {

            }

            void Sender::add( queue_id_type destination, const common::ipc::message::Complete& message) const
            {
               m_implementation->add( destination, message);
            }




            struct Receiver::Implementation : public local::basic_implementation< local::Receiver>
            {
               void fetch( std::vector< type_type> types)
               {
                  local::message::fetch::Request request;
                  request.destination = ipc().id();
                  request.types = std::move( types);
                  send( request);
               }

               void clear()
               {
                  m_receiveQueue.clear();
               }
            };


            Receiver::Receiver() {}

            Receiver::~Receiver() {}

            Receiver::Receiver( Receiver&&) = default;
            Receiver& Receiver::operator = ( Receiver&&) = default;

            Receiver::id_type Receiver::id() const
            {
               return m_implementation->m_threadQueueId;
            }

            void Receiver::clear()
            {
               m_implementation->clear();
            }

            std::vector< common::ipc::message::Complete> Receiver::operator ()( const long flags)
            {
               m_implementation->fetch( {});
               return m_implementation->ipc()( flags);
            }

            std::vector< common::ipc::message::Complete> Receiver::operator ()( type_type type, const long flags)
            {
               m_implementation->fetch( { type});
               return m_implementation->ipc()( type, flags);
            }



            std::vector< common::ipc::message::Complete> Receiver::operator ()( const std::vector< type_type>& types, const long flags)
            {
               m_implementation->fetch( types);
               return m_implementation->ipc()( types, flags);
            }





            namespace broker
            {

               Receiver initializeMockupBrokerQueue()
               {
                  static file::ScopedPath path{ common::environment::file::brokerQueue()};

                  Receiver queue;

                  log::debug << "writing mockup broker queue file: " << path.path() << std::endl;

                  std::ofstream brokerQueueFile( path);

                  if( brokerQueueFile)
                  {
                     brokerQueueFile << queue.id() << std::endl;
                     brokerQueueFile.close();
                  }
                  else
                  {
                     throw exception::NotReallySureWhatToNameThisException( "failed to write broker queue file: " + path.path());
                  }

                  return queue;
               }


               Receiver& queue()
               {
                  static Receiver singelton = initializeMockupBrokerQueue();
                  return singelton;
               }

               id_type id()
               {
                  return queue().id();
               }

            } // broker


            namespace receive
            {
               bool Sender::operator () ( const common::ipc::message::Complete& message) const
               {
                  m_sender.add( common::ipc::receive::id(), message);
                  return true;
               }


               bool Sender::operator () ( const common::ipc::message::Complete& message, const long flags) const
               {
                  m_sender.add( common::ipc::receive::id(), message);
                  return true;
               }

               Sender& queue()
               {
                  static Sender singelton;
                  return singelton;
               }

               id_type id()
               {
                  return common::ipc::receive::id();
               }
            }
         } // ipc
      } // mockup
   } // common
} // casual
