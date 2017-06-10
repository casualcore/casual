//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_

#include "domain/manager/state.h"


#include "common/message/type.h"
#include "common/message/domain.h"
#include "common/message/dispatch.h"


namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace ipc
         {
            const common::communication::ipc::Helper& device();
         } // ipc


         namespace handle
         {

            using dispatch_type = decltype( ipc::device().handler());


            namespace mandatory
            {
               namespace boot
               {
                  void prepare( State& state);
               } // boot

            } // mandatory


            void boot( State& state);
            void shutdown( State& state);




            struct Base
            {
               Base( State& state);
            protected:
               State& state();
               const State& state() const;
            private:
               std::reference_wrapper< State> m_state;
            };

            struct Shutdown : Base
            {
               using Base::Base;
               void operator () ( common::message::shutdown::Request& message);
            };


            namespace scale
            {

               void instances( State& state, state::Server& server);
               void instances( State& state, state::Executable& executable);


               namespace prepare
               {
                  struct Shutdown : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::domain::process::prepare::shutdown::Reply& message);
                  };
               } // prepare

            } // scale



            namespace event
            {
               namespace subscription
               {
                  struct Begin : Base
                  {
                     using Base::Base;

                     void operator () ( const common::message::event::subscription::Begin& message);
                  };

                  struct End : Base
                  {
                     using Base::Base;

                     void operator () ( const common::message::event::subscription::End& message);
                  };

               } // subscription


               namespace process
               {
                  void exit( const common::process::lifetime::Exit& exit);

                  struct Exit : Base
                  {
                     using Base::Base;

                     void operator () ( common::message::event::process::Exit& message);
                  };
               } // process

               struct Error : Base
               {
                  using Base::Base;

                  void operator () ( common::message::event::domain::Error& message);
               };

            } // event


            namespace process
            {
               struct Connect : public Base
               {
                  using Base::Base;

                  void operator () ( common::message::domain::process::connect::Request& message);
               };

               struct Lookup : public Base
               {
                  using Base::Base;

                  void operator () ( const common::message::domain::process::lookup::Request& message);
               };

            } // process

            namespace configuration
            {
               struct Domain : public Base
               {
                  using Base::Base;

                  void operator () ( const common::message::domain::configuration::Request& message);
               };


               struct Server : public Base
               {
                  using Base::Base;

                  void operator () ( const common::message::domain::configuration::server::Request& message);

               };

            } // configuration

         } // handle


         handle::dispatch_type handler( State& state);

      } // manager
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_
