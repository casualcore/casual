//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_



#include "common/server/service.h"


#include "common/message/traffic.h"

#include "common/platform.h"


#include "xatmi.h"


//
// std
//
#include <unordered_map>
#include <functional>


namespace casual
{
   namespace common
   {
      namespace server
      {

         struct State
         {
            struct jump_t
            {
               enum From
               {
                  c_no_jump = 0,
                  c_return = 10,
                  c_forward = 20,
               };

               struct buffer_t
               {
                  platform::raw_buffer_type data = nullptr;
                  long len = 0;

               } buffer;

               struct state_t
               {
                  int value = 0;
                  long code = 0;
               } state;

               struct forward_t
               {
                  std::string service;

               } forward;

               friend std::ostream& operator << ( std::ostream& out, const jump_t& value);

            } jump;


            State() = default;

            State( const State&) = delete;
            State& operator = (const State&) = delete;

            std::deque< Service> physical_services;

            typedef std::unordered_map< std::string, std::reference_wrapper< Service>> service_mapping_type;

            service_mapping_type services;
            common::platform::long_jump_buffer_type long_jump_buffer;

            message::traffic::Event traffic;

            std::function<void()> server_done;

         };

         class Context
         {
         public:


            static Context& instance();

            Context( const Context&) = delete;


            //!
            //! Being called from tpreturn
            //!
            void long_jump_return( int rval, long rcode, char* data, long len, long flags);

            //!
            //! called from extern casual_service_forward
            //!
            void forward( const char* service, char* data, long size);

            //!
            //! Being called from tpadvertise
            //!
            void advertise( const std::string& service, void (*adress)( TPSVCINFO *));



            //!
            //! Being called from tpunadvertise
            //!
            void unadvertise( const std::string& service);

            //!
            //! Share state with callee::handle::basic_call for now...
            //! if this "design" feels good, we should expose needed functionality
            //! to callee::handle::basic_call
            //!
            State& state();

            void finalize();


         private:

            Context();

            State m_state;
         };

      } // server
	} // common
} // casual



#endif /* CASUAL_SERVER_CONTEXT_H_ */
