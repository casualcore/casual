//!
//! ipc.cpp
//!
//! Created on: May 30, 2014
//!     Author: Lazan
//!

#include "common/mockup/ipc.h"


#include "common/queue.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/file.h"
#include "common/signal.h"
#include "common/exception.h"

#include "common/internal/log.h"
#include "common/internal/trace.h"


#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <fstream>

// TODO:
#include <iostream>


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
              marshler << value.correlation;
              marshler << value.payload;
              marshler << value.type;
              marshler << value.offset;

            }

            template< typename M>
            void casual_unmarshal_value( Complete& value, M& unmarshler)
            {
              unmarshler >> value.correlation;
              unmarshler >> value.payload;
              unmarshler >> value.type;
              unmarshler >> value.offset;
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

                        cMockupDisconnect  = 100000, // avoid conflict with real messages
                        cMockupClear,

                     };

                     template< message::Type type>
                     struct basic_messsage
                     {
                        enum
                        {
                           message_type = type
                        };

                        Uuid correlation;

                        CASUAL_CONST_CORRECT_MARSHAL({
                           // no information
                        })
                     };

                     using Disconnect =  basic_messsage< cMockupDisconnect>;
                     using Clear =  basic_messsage< cMockupClear>;
                  }
               }
            }

            namespace implementation
            {

               struct Worker
               {

                  using cache_type = std::deque< common::ipc::message::Complete>;

                  using transform_type = ipc::transform_type;

                  struct State
                  {

                     State( common::ipc::receive::Queue&& source, common::platform::queue_id_type destination, transform_type transform)
                        : source( std::move( source)), destination( destination), transform( std::move( transform)) {}

                     common::ipc::receive::Queue source;
                     common::ipc::send::Queue destination;
                     transform_type transform;

                     cache_type cache;
                  };

                  void operator () ( common::ipc::receive::Queue&& source, common::platform::queue_id_type destination)
                  {
                     (*this)( std::move( source), destination, nullptr);
                  }

                  void operator () ( common::ipc::receive::Queue&& source, common::platform::queue_id_type destination, transform_type transform)
                  {
                     //!
                     //! Block all signals
                     //!
                     common::signal::thread::block();

                     State state( std::move( source), destination, std::move( transform));

                     try
                     {

                        while( true)
                        {
                           read( state);
                           process::sleep( std::chrono::microseconds( 10));
                           write( state);
                           process::sleep( std::chrono::microseconds( 10));
                        }
                     }
                     catch( const Disconnect&)
                     {
                        log::internal::ipc << "thread " << std::this_thread::get_id() << " disconnects - source: "
                              <<  state.source.id() << " destination: " << state.destination.id() << std::endl;
                     }
                     catch( ...)
                     {
                        // todo: Temp
                        std::cerr << "thread " << std::this_thread::get_id() << " got exception" << std::endl;
                        common::error::handler();
                     }
                  }

               private:


                  struct Disconnect : common::exception::Base
                  {
                     using common::exception::Base::Base;
                  };

                  void read( State& state)
                  {

                     // We block if the queue is empty
                     const long flags = state.cache.empty() ? 0 : common::ipc::receive::Queue::cNoBlocking;

                     log::internal::ipc << "read from source: " <<  state.source.id() << " flags: " << flags << std::endl;

                     auto message = state.source( flags);

                     if( message.empty())
                        return;

                     if( check( state, message.front()))
                     {
                        if( state.transform)
                        {
                           for( auto& complete : state.transform( message.front()))
                           {
                              state.cache.push_back( std::move( complete));
                           }
                        }
                        else
                        {
                           state.cache.push_back( std::move( message.front()));
                        }
                     }

                  }

                  void write( State& state)
                  {
                     if( ! state.cache.empty())
                     {
                        log::internal::ipc << "write to destination: " <<  state.destination.id() << " flags: " << common::ipc::send::Queue::cNoBlocking << std::endl;

                        if( state.destination( state.cache.front(), common::ipc::send::Queue::cNoBlocking))
                        {
                           state.cache.pop_front();
                        }
                     }
                  }

                  bool check( State& state, const common::ipc::message::Complete& message)
                  {
                     switch( message.type)
                     {
                        case local::message::Disconnect::message_type:
                        {
                           throw Disconnect( "disconnect", __FILE__, __LINE__);
                        }
                        case local::message::Clear::message_type:
                        {
                           decltype( state.cache) empty;
                           std::swap( state.cache, empty);

                           do
                           {
                              process::sleep( std::chrono::microseconds( 10));
                           }
                           while( ! state.source( common::ipc::receive::Queue::cNoBlocking).empty());

                           return false;
                        }
                     }
                     return true;
                  }
               };

               class Router
               {
               public:

                  Router( id_type destination, Worker::transform_type transform)
                     : destination( destination)
                  {
                     //
                     // We use an ipc-queue that does not check signals
                     //
                     common::ipc::receive::Queue ipc;
                     id = ipc.id();

                     m_thread = std::thread{ implementation::Worker{}, std::move( ipc), destination, std::move( transform)};
                  }

                  Router( id_type destination)
                     : Router( destination, nullptr)
                  {
                  }

                  ~Router()
                  {
                     try
                     {
                        common::queue::blocking::Writer send{ id};
                        local::message::Disconnect message;

                        bool resend = true;

                        while( resend)
                        {
                           try
                           {
                              send( message);
                              resend = false;
                           }
                           catch( const exception::signal::Base&) {}
                        }

                     }
                     catch( const std::exception& exception)
                     {
                        log::internal::ipc << "mockup - failed to send disconnect to thread: " << m_thread.get_id() << " - " << exception.what() << std::endl;
                     }

                     try
                     {
                        m_thread.join();
                     }
                     catch( const std::exception& exception)
                     {
                        log::internal::ipc << "mockup - failed to join thread: " << m_thread.get_id() << " - " << exception.what() << std::endl;
                     }
                  }

                  id_type id;
                  id_type destination;
                  std::thread m_thread;

               };

            } // implementation

            class Router::Implementation : public implementation::Router
            {
            public:
               using implementation::Router::Router;
            };

            Router::Router( id_type destination, transform_type transform)
              : m_implementation( destination, std::move( transform))
            {

            }
            Router::Router( id_type destination) : m_implementation( destination) {}
            Router::~Router() {}

            Router::Router( Router&&) noexcept = default;
            Router& Router::operator = ( Router&&) noexcept = default;

            id_type Router::id() const { return m_implementation->id;}

            id_type Router::destination() const { return m_implementation->destination;}



            class Link::Implementation
            {
            public:
               Implementation( id_type source, id_type destination)
                 : source( source), destination( destination)
               {

                  m_thread = std::thread{ []( id_type source, id_type destination)
                  {
                     std::deque< common::ipc::message::Transport> cache;

                     common::ipc::message::Transport transport;

                     while( true)
                     {
                        if( cache.empty())
                        {
                           //
                           // We block
                           //
                           common::ipc::message::receive( source, transport, 0);
                           cache.push_back( transport);
                        }
                        else if( common::ipc::message::receive( source, transport, common::ipc::message::Flags::cNoBlocking))
                        {
                           cache.push_back( transport);
                        }
                        else
                        {
                           common::process::sleep( std::chrono::microseconds{ 10});
                        }

                        if( transport.message.type == local::message::Disconnect::message_type)
                        {
                           common::ipc::message::send( destination, transport, common::ipc::message::Flags::cNoBlocking);
                           return;
                        }

                        if( ! cache.empty() && common::ipc::message::send( destination, cache.front(), common::ipc::message::Flags::cNoBlocking))
                        {
                           cache.pop_front();
                        }
                     }
                  }, source, destination};
               }

               ~Implementation()
               {
                  try
                  {
                     common::ipc::message::Transport transport;
                     transport.message.type = local::message::Disconnect::message_type;

                     bool resend = true;

                     while( resend)
                     {
                        try
                        {
                           common::ipc::message::send( source, transport, 0);
                           resend = false;
                        }
                        catch( const exception::signal::Base&) {}
                     }


                  }
                  catch( const std::exception& exception)
                  {
                     log::internal::ipc << "mockup - failed to send disconnect to thread: " << m_thread.get_id() << " - " << exception.what() << std::endl;
                  }

                  try
                  {
                     m_thread.join();
                  }
                  catch( const std::exception& exception)
                  {
                     log::internal::ipc << "mockup - failed to join thread: " << m_thread.get_id() << " - " << exception.what() << std::endl;
                  }
               }

               id_type source;
               id_type destination;
               std::thread m_thread;

            };

            Link::Link( id_type source, id_type destination)
            : m_implementation( source, destination)
            {}

            Link::~Link() = default;

            Link::Link( Link&&) noexcept = default;
            Link& Link::operator = ( Link&&) noexcept = default;

            id_type Link::source() const { return m_implementation->source;}
            id_type Link::destination() const { return m_implementation->destination;}







            struct Instance::Implementation
            {
               Implementation( platform::pid_type pid, transform_type transform) : pid( pid), router( receive.id(), std::move( transform))
               {
               }

               platform::pid_type pid;
               common::ipc::receive::Queue receive;
               Router router;
            };

            Instance::Instance( platform::pid_type pid, transform_type transform) : m_implementation( pid, std::move( transform)) {}
            Instance::Instance( platform::pid_type pid) : Instance( pid, nullptr) {}


            Instance::Instance( transform_type transform) : Instance( process::id(), std::move( transform)) {}
            Instance::Instance() : Instance( process::id(), nullptr) {}

            Instance::~Instance() {}

            Instance::Instance( Instance&&) noexcept = default;
            Instance& Instance::operator = ( Instance&&) noexcept = default;

            platform::pid_type Instance::pid() const
            {
               return m_implementation->pid;
            }

            id_type Instance::id() const
            {
               return m_implementation->router.id();
            }

            common::process::Handle Instance::server() const
            {
               return { pid(), id()};

            }

            common::ipc::receive::Queue& Instance::receive()
            {
               return m_implementation->receive;
            }

            void Instance::clear()
            {
               common::queue::blocking::Writer send{ id()};
               local::message::Clear message;
               send( message);

               std::size_t count = 0;

               do
               {
                  ++count;
                  process::sleep( std::chrono::milliseconds( 10));
               }
               while( ! m_implementation->receive( common::ipc::receive::Queue::cNoBlocking).empty());


               log::internal::ipc << "mockup - cleared " << count - 1 << " messages" << std::endl;
            }



            namespace broker
            {
               namespace local
               {
                  namespace
                  {
                     struct Instance : ipc::Instance
                     {
                        Instance() : ipc::Instance( 6666), m_path( common::environment::file::brokerQueue())
                        {
                           log::debug << "writing mockup broker queue file: " << m_path.path() << std::endl;

                           std::ofstream brokerQueueFile( m_path);

                           if( brokerQueueFile)
                           {
                              brokerQueueFile << id() << std::endl;
                              brokerQueueFile.close();
                           }
                           else
                           {
                              throw exception::NotReallySureWhatToNameThisException( "failed to write broker queue file: " + m_path.path());
                           }
                        }

                        file::scoped::Path m_path;
                     };
                  } // <unnamed>
               } // local


               platform::pid_type pid()
               {
                  return queue().pid();
               }

               ipc::Instance& queue()
               {
                  static local::Instance singleton;
                  return singleton;
               }

               id_type id()
               {
                  return queue().id();
               }

            } // broker

            namespace transaction
            {
               namespace manager
               {
                  platform::pid_type pid()
                  {
                     return queue().pid();
                  }

                  ipc::Instance& queue()
                  {
                     static Instance singleton( 7777);
                     return singleton;
                  }

                  id_type id()
                  {
                     return queue().id();
                  }

               } // manager

            } // transaction

            void clear()
            {
               broker::queue().clear();
               transaction::manager::queue().clear();
            }

         } // ipc
      } // mockup
   } // common
} // casual
