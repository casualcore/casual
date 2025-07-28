//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "server/service.h"
#include "server/argument.h"

#include "casual/platform.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"

#include <unordered_map>
#include <functional>


namespace casual
{
   namespace server
   {

      using dispatch_type = common::message::dispatch::basic_handler< common::communication::ipc::message::Complete>;
 
      struct State
      {
         std::deque< Service> physical_services;

         using service_mapping_type = std::unordered_map< std::string, std::reference_wrapper< Service>>;
         service_mapping_type services;
      };

      namespace detail
      {
         // only exposed for unittests
         void finalize_transaction( bool commit);
      } // detail


      struct Context
      {
         server::dispatch_type initialize( server::Arguments arguments) &;

         static Context& instance();

         Context( const Context&) = delete;


         void advertise( Service service);

         //! Being called from tpunadvertise
         void unadvertise( const std::string& service);

         //! Basic configuration for a server
         void configure( const server::Arguments& arguments);


         //! Share state with callee::handle::basic_call for now...
         //! if this "design" feels good, we should expose needed functionality
         //! to callee::handle::basic_call
         State& state();

         void finalize();

      private:
         Context();
         State m_state;
      };

      inline Context& context() { return Context::instance();}

   } // server
} // casual




