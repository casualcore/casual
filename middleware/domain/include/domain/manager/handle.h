//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_

#include "domain/manager/state.h"


#include "common/message/type.h"
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

            namespace adjust
            {
               void instances( State& state);




            } // adjust




            namespace mandatory
            {

               //!
               //! Boots the mandatory parts
               //!
               //! @param state
               //!
               void boot( State& state);
            } // mandatory



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


            namespace scale
            {

               struct Executable : Base
               {
                  using Base::Base;

                  void operator () ( state::Executable& executable) const;
               };

            } // scale





            namespace process
            {
               namespace termination
               {
                  struct Registration : Base
                  {
                     using Base::Base;

                     void operator () ( const common::message::process::termination::Registration& message);
                  };


                  void event( const common::process::lifetime::Exit& exit);

                  struct Event : Base
                  {
                     using Base::Base;

                     void operator () ( common::message::process::termination::Event& message);
                  };
               } // termination

               struct Connect : public Base
               {
                  using Base::Base;

                  void operator () ( common::message::inbound::ipc::Connect& message);
               };

               struct Lookup : public Base
               {
                  using Base::Base;

                  void operator () ( const common::message::process::lookup::Request& message);
               };

            } // process

         } // handle


         common::message::dispatch::Handler handler( State& state);

      } // manager
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_
