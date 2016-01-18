//!
//! ipc.cpp
//!
//! Created on: May 30, 2014
//!     Author: Lazan
//!

#include "common/mockup/ipc.h"


//#include "common/queue.h"

#include "common/communication/ipc.h"

#include "common/process.h"
#include "common/environment.h"
#include "common/file.h"
#include "common/signal.h"
#include "common/exception.h"

#include "common/internal/log.h"
#include "common/internal/trace.h"

#include "common/message/type.h"


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

      namespace communication
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

            }

            template< typename M>
            void casual_unmarshal_value( Complete& value, M& unmarshler)
            {
              unmarshler >> value.correlation;
              unmarshler >> value.payload;
              unmarshler >> value.type;
            }
         } // message
      } // communication



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
                     using Disconnect =  common::message::basic_message< common::message::Type::mockup_disconnect>;
                     using Clear =  common::message::basic_message< common::message::Type::mockup_clear>;
                  }
               }
            }

            namespace implementation
            {

               struct Disconnect : common::exception::base
               {
                  using common::exception::base::base;
               };

               struct Worker
               {

                  using cache_type = std::deque< communication::message::Complete>;

                  using transform_type = ipc::transform_type;

                  struct State
                  {

                     State( communication::ipc::inbound::Device&& source, common::platform::queue_id_type destination, transform_type transform)
                        : source( std::move( source)), destination( destination), transform( std::move( transform)) {}

                     communication::ipc::inbound::Device source;
                     //common::ipc::receive::Queue source;
                     communication::ipc::outbound::Device destination;
                     //common::ipc::send::Queue destination;
                     transform_type transform;

                     cache_type cache;
                  };

                  void operator () ( communication::ipc::inbound::Device&& source, common::platform::queue_id_type destination)
                  {
                     (*this)( std::move( source), destination, nullptr);
                  }

                  void operator () ( communication::ipc::inbound::Device&& source, common::platform::queue_id_type destination, transform_type transform)
                  {
                     //!
                     //! Block all signals
                     //!
                     common::signal::thread::block();

                     State state( std::move( source), destination, std::move( transform));

                     try
                     {
                        Trace trace{ "implemenation::Worker::operator()", log::internal::ipc};

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
                              <<  state.source.connector() << " destination: " << state.destination.connector() << std::endl;
                     }
                     catch( ...)
                     {
                        // todo: Temp
                        std::cerr << "thread " << std::this_thread::get_id() << " got exception" << std::endl;
                        common::error::handler();
                     }
                  }

               private:

                  void read( State& state)
                  {

                     // We block if the queue is empty
                     auto next = [&](){
                        if( state.cache.empty())
                        {
                           return state.source.next( communication::ipc::policy::ignore::signal::Blocking{});
                        }
                        return  state.source.next( communication::ipc::policy::ignore::signal::non::Blocking{});
                     };


                     auto message = next();

                     if( ! message)
                        return;

                     log::internal::ipc << "read from source: " <<  state.source.connector() << " - message: " << message << std::endl;


                     if( check( state, message))
                     {
                        if( state.transform)
                        {
                           auto transformed = state.transform( message);

                           if( transformed.empty())
                           {
                              state.cache.push_back( std::move( message));
                           }
                           else
                           {
                              for( auto& complete : state.transform( message))
                              {
                                 state.cache.push_back( std::move( complete));
                              }
                           }

                        }
                        else
                        {
                           state.cache.push_back( std::move( message));
                        }
                     }

                  }

                  void write( State& state)
                  {
                     if( ! state.cache.empty())
                     {
                        log::internal::ipc << "write to destination: " <<  state.destination.connector() << " - message: " << state.cache.front() << std::endl;

                        if( state.destination.put( state.cache.front(), communication::ipc::policy::ignore::signal::non::Blocking{}))
                        {
                           state.cache.pop_front();
                        }
                     }
                  }

                  bool check( State& state, const communication::message::Complete& message)
                  {
                     switch( message.type)
                     {
                        case local::message::Disconnect::type():
                        {
                           throw Disconnect( "disconnect", __FILE__, __LINE__);
                        }
                        case local::message::Clear::type():
                        {
                           Trace trace{ "Worker::check clear queue", log::internal::ipc};

                           decltype( state.cache) empty;
                           std::swap( state.cache, empty);

                           do
                           {
                              process::sleep( std::chrono::microseconds( 10));
                           }
                           while( state.source.next( communication::ipc::policy::ignore::signal::non::Blocking{}));

                           return false;
                        }
                        default: return true;
                     }
                  }
               };



               void shutdown_thread( std::thread& thread, id_type input)
               {
                  Trace trace{ "shutdown_thread", log::internal::ipc};

                  log::internal::ipc << "thread id: " << thread.get_id() << " - ipc id: " << input << std::endl;

                  try
                  {
                     Trace trace{ "send disconnect message", log::internal::ipc};
                     local::message::Disconnect message;

                     communication::ipc::blocking::send( input, message);
                  }
                  catch( const std::exception& exception)
                  {
                     log::internal::ipc << "mockup - failed to send disconnect to thread: " << thread.get_id() << " - " << exception.what() << std::endl;
                  }
                  catch( ...)
                  {
                     error::handler();
                  }

                  try
                  {
                     Trace trace{ "thread join", log::internal::ipc};
                     thread.join();
                  }
                  catch( const std::exception& exception)
                  {
                     log::internal::ipc << "mockup - failed to join thread: " << thread.get_id() << " - " << exception.what() << std::endl;
                  }
               }


               class Router
               {
               public:

                  Router( id_type output, Worker::transform_type transform)
                     : output( output)
                  {
                     //
                     // We use an ipc-queue that does not check signals
                     //
                     communication::ipc::inbound::Device ipc;
                     input = ipc.connector().id();

                     m_thread = std::thread{ implementation::Worker{}, std::move( ipc), output, std::move( transform)};
                  }

                  Router( id_type output)
                     : Router( output, nullptr)
                  {
                  }

                  ~Router()
                  {
                     Trace trace{ "~Router()", log::internal::ipc};

                     shutdown_thread( m_thread, input);
                  }

                  id_type input;
                  id_type output;
                  std::thread m_thread;
               };

               struct Replier
               {

                  void operator () ( communication::ipc::inbound::Device&& input, reply::Handler replier)
                  {
                     //
                     // we need to block all signals
                     //
                     signal::thread::scope::Block block;


                     try
                     {
                        while( true)
                        {
                           auto check_message = [&]( communication::message::Complete& message)
                              {
                                 switch( message.type)
                                 {
                                    case local::message::Disconnect::type():
                                    {
                                       throw Disconnect( "disconnect", __FILE__, __LINE__);
                                    }
                                    case local::message::Clear::type():
                                    {
                                       m_pending.clear();

                                       do
                                       {
                                          process::sleep( std::chrono::microseconds{ 10});
                                       }
                                       while( ! input.next( communication::ipc::policy::ignore::signal::non::Blocking{}));
                                       break;
                                    }
                                    default:
                                    {
                                       // no -op
                                       break;
                                    }
                                 }
                              };

                           auto send_messages = [&]( std::vector< reply::result_t> messages)
                              {
                                 for( auto& message : messages)
                                 {
                                    communication::ipc::outbound::Device send{ message.queue};
                                    if( ! send.put( message.complete, communication::ipc::policy::ignore::signal::non::Blocking{}))
                                    {
                                       m_pending.push_back( std::move( message));
                                    }
                                 }
                              };

                           if( m_pending.empty())
                           {
                              auto message = input.next( communication::ipc::policy::ignore::signal::Blocking{});

                              check_message( message);

                              send_messages( replier( message));
                           }
                           else
                           {

                              auto message = input.next( communication::ipc::policy::ignore::signal::non::Blocking{});

                              if( message)
                              {
                                 check_message( message);

                                 send_messages( replier( message));
                              }
                              process::sleep( std::chrono::microseconds{ 10});

                              //
                              // Try to send all pending
                              //
                              {
                                 std::vector< reply::result_t> pending;
                                 std::swap( m_pending, pending);

                                 send_messages( std::move( pending));
                              }
                           }

                        }
                     }
                     catch( const Disconnect&)
                     {
                        log::internal::ipc << "mockup::ipc::implementation::Replier disconnect\n";
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }

                  std::vector< reply::result_t> m_pending;
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

            id_type Router::input() const { return m_implementation->input;}
            id_type Router::output() const { return m_implementation->output;}




            struct Replier::Implementation
            {
               Implementation( reply::Handler replier)
               {
                  communication::ipc::inbound::Device ipc;
                  input = ipc.connector().id();

                  m_thread = std::thread{ implementation::Replier{}, std::move( ipc), std::move( replier)};
               }

               ~Implementation()
               {
                  Trace trace{ "Replier::~Implementation()", log::internal::ipc};

                  implementation::shutdown_thread( m_thread, input);
               }

               id_type input;
            private:
               std::thread m_thread;

            };

            Replier::Replier( reply::Handler replier) : m_implementation{ std::move( replier)} {}

            Replier::~Replier() = default;


            Replier::Replier( Replier&&) noexcept = default;
            Replier& Replier::operator = ( Replier&&) noexcept = default;

            //!
            //! input-queue is owned by the Replier
            //!
            id_type Replier::input() const { return m_implementation->input;}




            class Link::Implementation
            {
            public:
               Implementation( id_type input, id_type output)
                 : input( input), output( output)
               {

                  m_thread = std::thread{ []( id_type input, id_type output)
                  {
                     try
                     {
                        Trace trace{ "Link::Implementation thread function", log::internal::ipc};

                        signal::thread::scope::Block block_signals;


                        if( ! ( communication::ipc::exists( input) && communication::ipc::exists( output)))
                        {
                           log::error << "mockup failed to set up link between [" << input << "] --> [" << output << "]" << std::endl;
                           return;
                        }
                        log::internal::ipc << "mockup link between [" << input << "] --> [" << output << "] established" << std::endl;

                        using message_type = communication::ipc::message::Transport;
                        std::deque< message_type> cache;

                        message_type transport;

                        while( true)
                        {
                           if( cache.empty())
                           {
                              //
                              // We block
                              //
                              communication::ipc::native::receive( input, transport, 0);
                              cache.push_back( transport);
                           }
                           else if( communication::ipc::native::receive( input, transport, communication::ipc::native::c_non_blocking))
                           {
                              cache.push_back( transport);
                           }
                           else
                           {
                              common::process::sleep( std::chrono::microseconds{ 10});
                           }

                           if( transport.type() == local::message::Disconnect::type())
                           {
                              //communication::ipc::native::send( output, transport, communication::ipc::native::c_non_blocking);

                              //log::internal::ipc << "mockup link got disconnect messsage - send it forward to [" << output << "] and return from thread\n";
                              log::internal::ipc << "mockup link got disconnect message - action: shutdown thread\n";
                              return;
                           }

                           if( ! cache.empty() && communication::ipc::native::send( output, cache.front(), communication::ipc::native::c_non_blocking))
                           {
                              cache.pop_front();
                           }
                        }
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }, input, output};
               }

               ~Implementation()
               {
                  Trace trace{ "Link::~Implementation()", log::internal::ipc};

                  implementation::shutdown_thread( m_thread, input);
               }

               id_type input;
               id_type output;
               std::thread m_thread;

            };

            Link::Link( id_type input, id_type output)
               : m_implementation( input, output)
            {}

            Link::~Link() = default;

            Link::Link( Link&&) noexcept = default;
            Link& Link::operator = ( Link&&) noexcept = default;

            id_type Link::input() const { return m_implementation->input;}
            id_type Link::output() const { return m_implementation->output;}







            struct Instance::Implementation
            {
               Implementation( platform::pid_type pid, transform_type transform)
                   : router( output.connector().id(), std::move( transform)), process{ pid, router.input()}
               {
               }

               communication::ipc::inbound::Device output;
               Router router;
               common::process::Handle process;
            };

            Instance::Instance( platform::pid_type pid, transform_type transform) : m_implementation( pid, std::move( transform)) {}
            Instance::Instance( platform::pid_type pid) : Instance( pid, nullptr) {}


            Instance::Instance( transform_type transform) : Instance( process::id(), std::move( transform)) {}
            Instance::Instance() : Instance( process::id(), nullptr) {}

            Instance::~Instance() {}

            Instance::Instance( Instance&&) noexcept = default;
            Instance& Instance::operator = ( Instance&&) noexcept = default;


            const common::process::Handle& Instance::process() const
            {
               return m_implementation->process;
            }

            id_type Instance::input() const
            {
               return m_implementation->router.input();
            }

            communication::ipc::inbound::Device& Instance::output()
            {
               return m_implementation->output;
            }

            void Instance::clear()
            {
               Trace trace{ "mockup::Instance::clear", log::internal::ipc };

               communication::ipc::outbound::Device ipc{ input()};
               local::message::Clear message;
               ipc.send( message, communication::ipc::policy::ignore::signal::Blocking{});

               std::size_t count = 0;

               do
               {
                  ++count;
                  process::sleep( std::chrono::milliseconds( 10));
               }
               while( m_implementation->output.next( communication::ipc::policy::ignore::signal::non::Blocking{}));


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




               ipc::Instance& queue()
               {
                  static local::Instance singleton;
                  return singleton;
               }

               platform::pid_type pid()
               {
                  return queue().process().pid;
               }

               id_type id()
               {
                  return queue().input();
               }

            } // broker

            namespace transaction
            {
               namespace manager
               {
                  platform::pid_type pid()
                  {
                     return queue().process().pid;
                  }


                  ipc::Instance& queue()
                  {
                     static Instance singleton( 7777);
                     return singleton;
                  }

                  id_type id()
                  {
                     return queue().input();
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
