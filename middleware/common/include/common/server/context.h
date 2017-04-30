//!
//! casual
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_



#include "common/server/service.h"


#include "common/platform.h"

#include "common/message/event.h"



#include "xatmi/defines.h"


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
                  platform::buffer::raw::type data = nullptr;
                  platform::buffer::raw::size::type size = 0;

               } buffer;

               struct State
               {
                  int value = 0;
                  long code = 0;
               } state;

               struct Forward
               {
                  std::string service;

               } forward;


               friend std::ostream& operator << ( std::ostream& out, const Jump& value);
            };
         } // state

         struct State
         {
            state::Jump jump;


            State() = default;

            State( const State&) = delete;
            State& operator = (const State&) = delete;

            std::deque< Service> physical_services;

            using service_mapping_type = std::unordered_map< std::string, std::reference_wrapper< Service>>;
            service_mapping_type services;


            message::event::service::Call event;

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
            void jump_return( int rval, long rcode, char* data, long len, long flags);

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
            //! Basic configuration for a server
            //!
            //! @param services
            void configure( const server::Arguments& arguments);


            //!
            //! Tries to find the physical service from it's original name
            //!
            //! @param name
            //! @return a pointer to the service if found, nullptr otherwise.
            server::Service* physical( const std::string& name);


            //!
            //! Tries to find the physical service from the associated callback function
            //!
            //! @param name
            //! @return a pointer to the service if found, nullptr otherwise.
            server::Service* physical( const server::Service::function_type& function);


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
