//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/common.h"
#include "domain/pending/message/environment.h"
#include "domain/pending/message/message.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/handle.h"
#include "common/exception/handle.h"

namespace casual
{
   using namespace common;

   namespace domain
   {      
      namespace pending
      {
         namespace message
         {
            namespace local
            {
               namespace
               {
                  struct State 
                  {
                     auto timeout() const 
                     {
                        if( messages.empty())
                           return platform::time::unit::min();

                        return std::chrono::duration_cast< platform::time::unit>( std::chrono::milliseconds{ 500});
                     }

                     std::vector< message::Request> messages;
                  };

                  
                  namespace ipc
                  {
                     bool send( message::Request& request)
                     {
                        return common::message::pending::non::blocking::send( request.message);
                     }
                  } // ipc
                  
                  namespace handle
                  {
                     struct Base
                     {
                        Base( State& state) : m_state( state) {}

                     protected:
                        State& m_state;
                     };

                     struct Request : Base
                     {
                        using Base::Base;

                        void operator () ( message::Request& message)
                        {
                           Trace trace{ "domain::pending::message::local::handle::Request"};

                           log::line( verbose::log, "message: ", message);
                           
                           // we block all signals during non-blocking-send
                           signal::thread::scope::Block block;

                           // we try to send the message directly
                           if( ! ipc::send( message))
                              m_state.messages.push_back( std::move( message));
                        }
                     };

                     void timeout( State& state)
                     {
                        Trace trace{ "domain::pending::message::local::handle::timeout"};

                        // we block all signals during non-blocking-send
                        signal::thread::scope::Block block;

                        common::algorithm::trim( state.messages, common::algorithm::filter( state.messages, &ipc::send));
                     }

                     struct Timeout : Base
                     {
                        using Base::Base;

                        void operator() ()
                        {
                           try
                           {
                              throw;
                           }
                           catch( const exception::signal::Timeout&)
                           {
                              // Timeout has occurred, lets try to send the delayed messages
                              timeout( m_state);
                           }
                        }
                     };

                  } // handle

                     
                  void start()
                  {
                     Trace trace{ "domain::pending::message::local::start"};

                     State state;

                     communication::ipc::Helper ipc{ handle::Timeout{ state}};

                     // connect process
                     {
                        pending::message::Connect connect;
                        connect.process = common::process::handle();
                        ipc.blocking_send( communication::instance::outbound::domain::manager::device(), connect);
                     }
                     
                     // Connect singleton to domain
                     communication::instance::connect( message::environment::identification);


                     auto handler = ipc.handler(
                        common::message::handle::defaults( ipc),
                        handle::Request{ state}
                     );

                     while( true)
                     {
                        // Set timeout, if any
                        signal::timer::set( state.timeout());

                        handler( ipc.blocking_next());
                     }
                  }

               } // <unnamed>
            } // local

            int main( int argc, const char **argv)
            {
               return common::exception::guard( []()
               {
                  local::start();
               });
            }
         } // message
      } // pending
   } // domain
} // casual

int main( int argc, const char **argv)
{
   return casual::domain::pending::message::main( argc, argv);
}

