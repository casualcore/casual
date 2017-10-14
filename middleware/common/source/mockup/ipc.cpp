//!
//! causal
//!

#include "common/mockup/ipc.h"
#include "common/mockup/log.h"


#include "common/communication/ipc.h"


#include "common/process.h"
#include "common/environment.h"
#include "common/file.h"
#include "common/signal.h"
#include "common/exception/system.h"
#include "common/exception/casual.h"
#include "common/exception/handle.h"

#include "common/log.h"

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

         namespace pid
         {
            strong::process::id next()
            {
               static auto pid = process::id().value() + 1000;
               return strong::process::id{ ++pid};
            }

         } // pid


         namespace ipc
         {




            namespace local
            {
               namespace
               {

                  template< typename T>
                  struct Queue
                  {
                     using value_type = T;
                     using lock_type = std::unique_lock< std::mutex>;

                     enum class State
                     {
                        empty,
                        content,
                        terminate,
                     };

                     Queue() = default;


                     ~Queue()
                     {
                        terminate();
                     }


                     void terminate() const
                     {
                        lock_type lock{ m_mutex};

                        m_state = State::terminate;
                        lock.unlock();
                        m_condition.notify_all();
                     }

                     void add( value_type&& message) const
                     {
                        lock_type lock{ m_mutex};

                        m_queue.push_back( std::move( message));

                        if( m_state == State::empty)
                        {
                           m_state = State::content;

                           lock.unlock();
                           m_condition.notify_one();
                        }
                     }


                     //!
                     //! Will block until there is something to get
                     //!
                     //! @return
                     value_type get() const
                     {
                        lock_type lock{ m_mutex};

                        m_condition.wait( lock, [&]{ return m_state != State::empty;});

                        if( m_state == State::terminate)
                        {
                           throw exception::casual::Shutdown{ "conditional variable wants to shutdown..."};
                        }

                        auto result = std::move( m_queue.front());
                        m_queue.pop_front();

                        m_state = m_queue.empty() ? State::empty : State::content;

                        return result;
                     }

                     bool empty() const
                     {
                        lock_type lock{ m_mutex};
                        return m_queue.empty();
                     }

                     void clear() const
                     {
                        lock_type lock{ m_mutex};
                        m_queue.clear();

                        if( m_state == State::content)
                        {
                           m_state = State::empty;

                           lock.unlock();
                           m_condition.notify_one();
                        }
                     }

                  private:
                     mutable std::mutex m_mutex;
                     mutable std::condition_variable m_condition;
                     mutable std::deque< value_type> m_queue;
                     mutable State m_state = State::empty;
                  };


                  void shutdown_thread( std::thread& thread, id_type input)
                  {
                     Trace trace{ "shutdown_thread"};

                     log << "thread id: " << thread.get_id() << " - ipc id: " << input << std::endl;

                     signal::thread::scope::Block block;

                     try
                     {
                        Trace trace{ "send disconnect message"};
                        message::mockup::Disconnect message;

                        communication::ipc::outbound::Device ipc{ input};
                        ipc.send( message, communication::ipc::policy::Blocking{});
                     }
                     catch( const std::exception& exception)
                     {
                        log << "mockup - failed to send disconnect to thread: " << thread.get_id() << " - " << exception.what() << std::endl;
                     }
                     catch( ...)
                     {
                        exception::handle();
                     }

                     try
                     {
                        Trace trace{ "thread join"};
                        thread.join();
                     }
                     catch( const std::exception& exception)
                     {
                        log << "mockup - failed to join thread: " << thread.get_id() << " - " << exception.what() << std::endl;
                     }
                  }


                  namespace eventually
                  {
                     struct Sender
                     {
                        struct Message
                        {
                           id_type destination;
                           communication::message::Complete message;
                        };

                        using queue_type = local::Queue< Message>;

                        static const Sender& instance()
                        {
                           static Sender singleton;
                           return singleton;
                        }

                        void send( id_type destination, communication::message::Complete&& complete) const
                        {
                           m_queue.add( { destination, std::move( complete)});
                        }

                        ~Sender()
                        {
                           Trace trace{ "mockup ipc::ventually::Sender dtor"};

                           m_queue.terminate();
                           m_sender.join();
                        }


                     private:
                        Sender() : m_sender{ &worker_thread, std::ref( m_queue)}
                        {

                        }

                        static void worker_thread( const queue_type& queue)
                        {
                           signal::thread::scope::Block block;

                           try
                           {

                              Trace trace{ "mockup ipc::eventually::Sender::worker_thread"};

                              while( true)
                              {
                                 auto message = queue.get();

                                 communication::ipc::outbound::Device ipc{ message.destination};

                                 try
                                 {
                                    log::debug << "mockup ipc::eventually::Sender::worker_thread ipc.put\n";

                                    ipc.put( message.message, communication::ipc::policy::Blocking{});
                                 }
                                 catch( const exception::system::communication::Unavailable&)
                                 {
                                    // no-op we just ignore it
                                 }
                              }
                           }
                           catch( ...)
                           {
                              exception::handle();
                           }
                        }

                        queue_type m_queue;
                        std::thread m_sender;
                     };
                  } // eventually

               } // unnamed
            } // local


            namespace eventually
            {


               Uuid send( id_type destination, communication::message::Complete&& complete)
               {
                  Trace trace{ "mockup ipc::eventually::send"};

                  auto correlation = complete.correlation;

                  local::eventually::Sender::instance().send( destination, std::move( complete));

                  return correlation;
               }


            } // eventually



            class Link::Implementation
            {
            public:

               using message_type = communication::ipc::message::Transport;
               using queue_type = local::Queue< message_type>;


               Implementation( id_type input, id_type output)
                 : input( input), output( output)
               {

                  if( ! ( communication::ipc::exists( input) && communication::ipc::exists( output)))
                  {
                     log::category::error << "mockup failed to set up link between [" << input << "] --> [" << output << "]" << std::endl;
                     return;
                  }

                  m_input_worker = std::thread{ []( id_type input, const queue_type& queue)
                  {
                     //
                     // We block all signals.
                     //
                     signal::thread::scope::Block block;

                     try
                     {
                        Trace trace{ "Link::input_worker"};

                        auto terminate = scope::execute( [&](){ queue.terminate();});

                        while( true)
                        {
                           message_type transport;

                           communication::ipc::native::receive( input, transport, {});

                           if( transport.type() == message::mockup::Disconnect::type())
                           {
                              log << "mockup link got disconnect message - action: shutdown thread\n";
                              return;
                           }
                           else if( transport.type() == message::mockup::Clear::type())
                           {
                              queue.clear();

                              while( communication::ipc::native::receive( input, transport, communication::ipc::native::Flag::non_blocking))
                                 ;

                              queue.add( std::move( transport));

                           }
                           else
                           {
                              queue.add( std::move( transport));
                           }
                        }
                     }
                     catch( ...)
                     {
                        exception::handle();
                     }
                  }, input, std::ref( m_queue)};


                  m_output_worker = std::thread{ []( id_type output, const queue_type& queue)
                  {
                     //
                     // We block all signals.
                     //
                     signal::thread::scope::Block block;

                     try
                     {
                        Trace trace{ "Link::output_worker"};

                        while( true)
                        {
                           auto transport = queue.get();

                           if( transport.type() == message::mockup::Clear::type())
                           {
                              while( communication::ipc::native::receive( output, transport, communication::ipc::native::Flag::non_blocking))
                                 ;
                           }
                           else
                           {
                              communication::ipc::native::send( output, transport, {});
                           }
                        }
                     }
                     catch( ...)
                     {
                        exception::handle();
                     }
                  }, output, std::ref( m_queue)};
               }

               ~Implementation()
               {
                  Trace trace{ "Link::~Implementation()"};

                  local::shutdown_thread( m_input_worker, input);
                  m_output_worker.join();
               }


               id_type input;
               id_type output;
               queue_type m_queue;
               std::thread m_input_worker;
               std::thread m_output_worker;

            };

            Link::Link( id_type input, id_type output)
               : m_implementation( input, output)
            {
               log << "link: " << *this << '\n';
            }

            Link::~Link() = default;

            Link::Link( Link&&) noexcept = default;
            Link& Link::operator = ( Link&&) noexcept = default;

            id_type Link::input() const { return m_implementation->input;}
            id_type Link::output() const { return m_implementation->output;}

            void Link::clear() const
            {
               communication::ipc::outbound::Device ipc{ input()};
               ipc.send( message::mockup::Clear{}, communication::ipc::policy::Blocking{});
            }

            std::ostream& operator << ( std::ostream& out, const Link& value)
            {
               return out << "{ input:" << value.m_implementation->input
                     << ", output: " << value.m_implementation->output
                     << '}';
            }


            struct Collector::Implementation
            {
               Implementation() : m_link{ m_input.connector().id(), m_output.connector().id()}, m_pid( mockup::pid::next())
               {
                  Trace trace{ "Collector::Implementation()"};

               }

               void clear()
               {
                  m_link.clear();
                  m_input.clear();
                  m_output.clear();
               }

               communication::ipc::inbound::Device m_input;
               communication::ipc::inbound::Device m_output;
               Link m_link;
               strong::process::id m_pid;
            };



            Collector::Collector()
            {
               log << "collector: " << *this << '\n';
            }
            Collector::~Collector() = default;

            id_type Collector::input() const { return m_implementation->m_input.connector().id();}
            communication::ipc::inbound::Device& Collector::output() const { return m_implementation->m_output;}


            process::Handle Collector::process() const
            {
               return { m_implementation->m_pid, input()};
            }

            void Collector::clear()
            {
               m_implementation->clear();
            }

            std::ostream& operator << ( std::ostream& out, const Collector& value)
            {
               return out << "{ process: " << value.process()
                     << ", input:" << value.m_implementation->m_input
                     << ", output: " << value.m_implementation->m_output
                     << ", link: " << value.m_implementation->m_link
                     << '}';
            }


            struct Replier::Implementation
            {
               Implementation( communication::ipc::dispatch::Handler&& replier) : process{ mockup::pid::next()}
               {
                  communication::ipc::inbound::Device ipc;
                  process.queue = ipc.connector().id();

                  m_thread = std::thread{ &worker_thread, std::move( ipc), std::move( replier), process.pid};
               }

               ~Implementation()
               {
                  Trace trace{ "Replier::~Implementation()"};

                  local::shutdown_thread( m_thread, process.queue);
               }

               static void worker_thread(
                     communication::ipc::inbound::Device&& ipc,
                     communication::ipc::dispatch::Handler&& replier,
                     strong::process::id pid)
               {
                  Trace trace{ "Replier::worker_thread"};

                  try
                  {
                     if( range::find( replier.types(), common::message::Type::mockup_need_worker_process))
                     {
                        message::mockup::thread::Process message;
                        message.process.pid = pid;
                        message.process.queue = ipc.connector().id();

                        replier( marshal::complete( message));
                     }

                     replier.insert( []( message::mockup::Disconnect&){
                        log << "Replier::worker_thread disconnect\n";
                        throw exception::casual::Shutdown{ "worker_thread disconnect"};
                     });

                     log << "dispatch handler: " << replier << '\n';

                     message::dispatch::blocking::pump( replier, ipc);
                  }
                  catch( ...)
                  {
                     exception::handle();
                  }
               }

               process::Handle process;

            private:
               std::thread m_thread;

            };

            Replier::Replier( communication::ipc::dispatch::Handler&& replier) : m_implementation{ std::move( replier)}
            {
               log << "replier: " << *this << '\n';
            }

            Replier::~Replier() = default;


            Replier::Replier( Replier&&) noexcept = default;
            Replier& Replier::operator = ( Replier&&) noexcept = default;


            process::Handle Replier::process() const { return m_implementation->process;}
            id_type Replier::input() const { return m_implementation->process.queue;}

            std::ostream& operator << ( std::ostream& out, const Replier& value)
            {
               return out << "{ process:" << value.m_implementation->process
                     << '}';
            }



         } // ipc
      } // mockup
   } // common
} // casual
