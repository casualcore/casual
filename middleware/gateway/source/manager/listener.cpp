//!
//! casual 
//!

#include "gateway/manager/listener.h"
#include "gateway/message.h"

#include "common/communication/ipc.h"
#include "common/trace.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               void listener_thread( communication::tcp::Address address, Uuid correlation)
               {
                  Trace trace{ "listener_thread", log::internal::gateway};

                  signal::thread::scope::Mask block{ signal::set::filled( { signal::Type::user})};


                  auto send_event = [=]( gateway::message::manager::listener::Event::State state){
                     gateway::message::manager::listener::Event event;
                     event.state = state;
                     event.correlation = correlation;

                     communication::ipc::outbound::Device ipc{ communication::ipc::inbound::id()};
                     ipc.send( event, communication::ipc::policy::Blocking{});
                  };


                  try
                  {
                     communication::tcp::Listener listener{ std::move( address)};

                     //
                     // let manager know we're up'n running
                     //
                     send_event( gateway::message::manager::listener::Event::State::running);


                     while( true)
                     {

                        auto socket = listener();

                        log::internal::gateway << "socket connect: " << socket << std::endl;

                        if( socket)
                        {
                           gateway::message::tcp::Connect message;
                           message.descriptor = socket.descriptor();

                           communication::ipc::blocking::send( communication::ipc::inbound::id(), message);

                           socket.release();
                        }
                     }
                  }
                  catch( const exception::signal::User&)
                  {
                     send_event( gateway::message::manager::listener::Event::State::signal);
                  }
                  catch( ...)
                  {
                     error::handler();
                     send_event( gateway::message::manager::listener::Event::State::error);
                  }
               }


            } // <unnamed>
         } // local

         Listener::Listener( common::communication::tcp::Address address)
            : m_address{ std::move( address)}
         {
         }

         Listener::~Listener()
         {
            Trace trace{ "gateway::manager::Listener::~Listener()", log::internal::gateway};
            try
            {
               if( m_thread.joinable())
               {
                  log::internal::gateway << "listener still active: " << *this << '\n';

                  // TODO: should we try to shutdown?
               }
            }
            catch( ...)
            {
               error::handler();
            }
         }

         Listener::Listener( Listener&&) noexcept = default;
         Listener& Listener::operator = ( Listener&&) noexcept = default;


         void Listener::start()
         {
            Trace trace{ "gateway::manager::Listener::start()", log::internal::gateway};

            if( m_thread.joinable())
            {
               throw exception::invalid::Semantic{ "trying to start a listener that is already started", CASUAL_NIP( *this)};
            }

            m_thread = std::thread{ local::listener_thread, m_address, m_correlation};
            m_state = State::spawned;
         }

         void Listener::shutdown()
         {
            Trace trace{ "gateway::manager::Listener::shutdown()", log::internal::gateway};

            log::internal::gateway << "shutdown listener: " << *this << std::endl;

            if( m_thread.joinable() && m_state != State::signaled)
            {
               signal::thread::send( m_thread, signal::Type::user);
               m_state = State::signaled;

            }
         }

         void Listener::event( const message::manager::listener::Event& event)
         {
            log::internal::gateway << "Listener::event event: " << event << std::endl;

            if( event.correlation != m_correlation)
            {
               throw exception::invalid::Argument{ "failed to correlate event"};
            }

            switch( event.state)
            {
               case message::manager::listener::Event::State::running:
               {
                  m_state = State::running;
                  break;
               }
               case message::manager::listener::Event::State::signal:
               {
                  m_thread.join();

                  if( m_state == State::signaled)
                  {
                     m_state = State::exit;
                  }
                  else
                  {
                     //
                     // Listener got signal, and we didn't send it. Our semantics is that
                     // some other process send a sig-term
                     //
                     m_state = State::exit;
                     throw exception::Shutdown{ "listener got terminate signal"};
                  }

                  break;
               }
               case message::manager::listener::Event::State::exit:
               {
                  m_thread.join();
                  m_state = State::exit;
                  break;
               }
               case message::manager::listener::Event::State::error:
               {
                  m_state = State::error;
                  m_thread.join();
                  break;
               }
            }
         }

         Listener::State Listener::state() const
         {
            return m_state;
         }

         const common::communication::tcp::Address& Listener::address() const
         {
            return m_address;
         }

         bool Listener::running() const
         {
            switch( m_state)
            {
               case State::spawned:
               case State::running:
               case State::signaled:
               {
                  return true;
               }
               default:
               {
                  return false;
               }
            }
         }

         bool operator < ( const Listener& lhs, const common::Uuid& rhs)
         {
            return lhs.m_correlation < rhs;
         }
         bool operator == ( const Listener& lhs, const common::Uuid& rhs)
         {
            return lhs.m_correlation == rhs;
         }

         std::ostream& operator << ( std::ostream& out, const Listener::State& value)
         {
            switch( value)
            {
               case Listener::State::absent: return out << "absent";
               case Listener::State::spawned: return out << "spawned";
               case Listener::State::running: return out << "running";
               case Listener::State::signaled: return out << "signaled";
               case Listener::State::exit: return out << "exit";
               case Listener::State::error: return out << "error";
            }
            return out;
         }
         std::ostream& operator << ( std::ostream& out, const Listener& value)
         {
            return out << "{ thread: " << value.m_thread.get_id()
                  << ", correlation: " << value.m_correlation
                  << ", address: " << value.m_address
                  << ", state: " << value.m_state
                  << '}';
         }

      } // manager

   } // gateway


} // casual
