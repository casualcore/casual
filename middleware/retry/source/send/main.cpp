//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "retry/send/common.h"
#include "retry/send/message.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/handle.h"
#include "common/exception/handle.h"

namespace casual
{
   using namespace common;

   namespace retry
   {
      namespace send
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
                        return common::platform::time::unit::min();

                     return std::chrono::duration_cast< common::platform::time::unit>( std::chrono::milliseconds{ 500});
                  }

                  std::vector< message::Request> messages;
               };

               
               namespace ipc
               {
                  bool send( const message::Request& request)
                  {
                     try
                     {
                        common::communication::ipc::outbound::Device ipc{ request.destination.ipc};
                        return ! ipc.put( request.message, common::communication::ipc::policy::non::Blocking{}).empty();
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        log::line( log, "destination unavailable: ", request.destination);
                        return true;
                     }
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
                        Trace trace{ "retry::send::local::handle::Request"};

                        log::line( verbose::log, "message: ", message);

                        // we try to send the message directly
                        if( ! ipc::send( message))
                           m_state.messages.push_back( std::move( message));
                     }
                  };

                  void timeout( State& state)
                  {
                     Trace trace{ "retry::send::local::handle::timeout"};

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
                  Trace trace{ "retry::send::local::start"};

                  State state;

                  // Connect to domain
                  communication::instance::connect( send::identification);

                  communication::ipc::Helper ipc{ handle::Timeout{ state}};

                  auto handler = ipc.handler(
                     common::message::handle::ping(),
                     common::message::handle::Shutdown{},
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
            try 
            {  
               local::start();
            }
            catch( ...)
            {
               return common::exception::handle();
            }
            return 0;
         }
      } // send
      
   } // retry

} // casual

int main( int argc, const char **argv)
{
   return casual::retry::send::main( argc, argv);
}

