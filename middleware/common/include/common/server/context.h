//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "common/server/service.h"


#include "casual/platform.h"

#include "casual/xatmi/defines.h"

// for definition of Service::Invoke::Parameter. Needed to save 
// the TPSVCINFO for possible retrieval by TPSVCSTART in Cobol 
// API. 
#include "common/service/invoke.h"


#include <unordered_map>
#include <functional>


namespace casual
{
   namespace common
   {
      namespace server
      {
         struct Arguments;

         namespace state
         {
            struct Jump
            {
               enum Location : int
               {
                  c_no_jump = 0,
                  c_return = 10,
                  c_forward = 20
               };

               platform::jump::buffer environment;

               struct Buffer
               {
                  buffer::handle::type data;
                  platform::buffer::raw::size::type size = 0;
               } buffer;

               struct State
               {
                  flag::xatmi::Return value = flag::xatmi::Return::success;
                  long code = 0;
               } state;

               struct Forward
               {
                  std::string service;

               } forward;

               friend std::ostream& operator << ( std::ostream& out, const Jump& value);
            };

            struct Parameter
            {
            //XXX   service::invoke::Parameter argument;
            TPSVCINFO argument;
            };


         } // state

         struct State
         {
            state::Jump jump;
            // saved copy of service routine argument. Needed to support TPSVCSTART
            // that retrieves this information in a "callback" from the service. Part
            // of Cobol api support. 
            state::Parameter information;
            // The Cobol API TPSVCSTART also need the buffer type and subtype
            // so we save them also... 
            std::string buffer_type;
            std::string buffer_subtype;

            // The C api returns to casual via a long_jump in "tpreturn", while
            // the COBOL api uses a normal return after calling TPRETURN.
            // Casual does not know if a service uses the COBOL api or the
            // C api. To allow casual to detect that TPRETURN was called
            // before a normal return this flag will be set to false 
            // invocation of a service. It will be set to true if/when
            // the service return data is set via TPRETURN and
            // Context::normal_return(). It is left unchanged in
            // Context::jump_return().
            // NOTE: In theory a sequence 
            //   call TPRETURN
            //   call tpreturn (without a "return" after the call to TPRETURN)
            // is possible. I have ignored this for now. It is an illegal
            // sequence and is very unliklely as the Cobol COPY that calls TPRETURN
            // includes an EXIT PROGRAM statement.
            // If it should happen the jump_return will overwrite the information
            // saved by TPRETURN, leading to a resource leak.
            //
            // NOTE: Should perhaps be renamed to "service_normal_return" or something
            // something like that. It is set to true by Context::normal_return()...  
            bool TPRETURN_called;

            State() = default;

            State( const State&) = delete;
            State& operator = (const State&) = delete;

            std::deque< Service> physical_services;

            using service_mapping_type = std::unordered_map< std::string, std::reference_wrapper< Service>>;
            service_mapping_type services;
         };

         class Context
         {
         public:

            static Context& instance();

            Context( const Context&) = delete;

            //! Being called from tpreturn
            void jump_return( flag::xatmi::Return rval, long rcode, char* data, long len);

            //! Being called from TPRETURN (via ...)
            void normal_return( flag::xatmi::Return rval, long rcode, char* data, long len);

            //! called from extern casual_service_forward
            void forward( const char* service, char* data, long size);

            //! Being called from tpadvertise
            void advertise( const std::string& service, void (*adress)( TPSVCINFO *));

            //! Being called from tpunadvertise
            void unadvertise( const std::string& service);

            //! Basic configuration for a server
            void configure( const server::Arguments& arguments);

            //! Tries to find the physical service from it's original name
            //!
            //! @param name
            //! @return a pointer to the service if found, nullptr otherwise.
            server::Service* physical( const std::string& name);

            //! Tries to find the physical service from the associated callback function
            //!
            //! @param name
            //! @return a pointer to the service if found, nullptr otherwise.
            server::Service* physical( const server::xatmi::function_type& function);

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
   } // common
} // casual




