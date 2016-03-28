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

                  auto send_event = [=]( gateway::message::manager::listener::Event::State state){
                     gateway::message::manager::listener::Event event;
                     event.state = state;
                     event.correlation = correlation;

                     communication::ipc::outbound::Device ipc{ communication::ipc::inbound::id()};
                     ipc.send( event, communication::ipc::policy::ignore::signal::Blocking{});
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
                  catch( const exception::signal::Terminate&)
                  {
                     send_event( gateway::message::manager::listener::Event::State::exit);
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
            : m_address{ std::move( address)},
              m_thread{ local::listener_thread, m_address, m_correlation}
         {
            if( m_thread.joinable())
            {
               message::manager::listener::Event message;
               communication::ipc::blocking::receive( communication::ipc::inbound::device(), message, m_correlation);

               event( message);
            }
         }

         Listener::~Listener()
         {
            try
            {
               terminate();
            }
            catch( ...)
            {
               error::handler();
            }
         }

         Listener::Listener( Listener&&) noexcept = default;
         Listener& Listener::operator = ( Listener&&) noexcept = default;

         void Listener::terminate()
         {
            log::internal::gateway << "terminate listener: " << *this << std::endl;

            if( m_thread.joinable())
            {
               signal::thread::send( m_thread, signal::Type::terminate);

               message::manager::listener::Event event;
               communication::ipc::inbound::device().receive( event, m_correlation, communication::ipc::policy::ignore::signal::Blocking{});

               m_thread.join();
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
               case message::manager::listener::Event::State::exit:
               {
                  m_state = State::exit;
                  m_thread.join();
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

         bool operator < ( const Listener& lhs, const common::Uuid& rhs)
         {
            return lhs.m_correlation < rhs;
         }
         bool operator == ( const Listener& lhs, const common::Uuid& rhs)
         {
            return lhs.m_correlation == rhs;
         }

         std::ostream& operator << ( std::ostream& out, const Listener& value)
         {
            auto get_state = []( const Listener& l){
               switch( l.m_state)
               {
                  case Listener::State::error: return "error";
                  case Listener::State::spawned: return "spawned";
                  case Listener::State::running: return "running";
                  case Listener::State::exit: return "exit";
               }
            };
            return out << "{ thread: " << value.m_thread.get_id()
                  << ", correlation: " << value.m_correlation
                  << ", address: " << value.address()
                  << ", state: " << get_state( value)
                  << '}';
         }

      } // manager

   } // gateway


} // casual
