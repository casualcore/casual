//!
//! casual 
//!

#include "gateway/handle.h"


namespace casual
{
   namespace gateway
   {
      namespace handle
      {

         Disconnect::Disconnect( std::thread& thread) : m_thread( thread) {}

         void Disconnect::operator() ( message_type& message)
         {
            common::log::internal::gateway << "got disconnect from worker thread\n";

            //
            // We got a disconnect from the worker thread, we wait for it
            // to finish
            //
            m_thread.join();

            // TODO: we may need to distinguish disconnect from shutdown...
            throw common::exception::Shutdown{ "disconnected"};
         }



      } // handle

   } // gateway

} // casual
