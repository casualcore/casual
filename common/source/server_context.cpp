//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "common/server_context.h"

#include "common/message.h"
#include "common/queue.h"
#include "common/buffer_context.h"
#include "common/calling_context.h"

#include "utility/logger.h"
#include "utility/error.h"



#include "xatmi.h"

#include <algorithm>

extern "C"
{
   int tpsvrinit(int argc, char **argv)
   {
      casual::utility::logger::debug << "internal tpsvrinit called";
      return 0;
   }

   void tpsvrdone()
   {
      casual::utility::logger::debug << "internal tpsvrdone called";
   }
}


namespace casual
{
   namespace common
   {
      namespace server
      {
         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }



         Context::Context()
         {

         }


         void Context::longJumpReturn( int rval, long rcode, char* data, long len, long flags)
         {
            //
            // Prepare buffer.
            // We have to keep state, since there seems not to be any way to send information
            // via longjump...
            //

            m_state.reply.returnValue = rval;
            m_state.reply.userReturnCode = rcode;
            m_state.reply.buffer = buffer::Context::instance().extract( data);

            longjmp( m_state.long_jump_buffer, 1);
         }

         void Context::advertiseService( const std::string& name, tpservice function)
         {

            //
            // validate
            //

            auto findIter = m_state.services.find( name);

            if( findIter != m_state.services.end())
            {
               //
               // service name is already advertised
               // No error if it's the same function
               //
               if( findIter->second.m_function != function)
               {
                  throw utility::exception::xatmi::service::AllreadyAdvertised( "service name: " + name);
               }

            }
            else
            {
               message::service::Advertise message;

               message.serverId.queue_key = ipc::getReceiveQueue().getKey();
               // TODO: message.serverPath =
               message.services.push_back( message::Service( name));

               // TODO: make it consistence safe...
               queue::blocking::Writer writer( ipc::getBrokerQueue());
               writer( message);

               m_state.services.emplace( name, service::Context( name, function));
            }
         }

         void Context::unadvertiseService( const std::string& name)
         {
            if( m_state.services.erase( name) != 1)
            {
               throw utility::exception::xatmi::service::NoEntry( "service name: " + name);
            }

            message::service::Unadvertise message;
            message.serverId.queue_key = ipc::getReceiveQueue().getKey();
            message.services.push_back( message::Service( name));

            queue::blocking::Writer writer( ipc::getBrokerQueue());
            writer( message);
         }


         State& Context::getState()
         {
            return m_state;
         }


         void Context::finalize()
         {
            buffer::Context::instance().clear();
         }


      } // server
   } // common
} // casual

