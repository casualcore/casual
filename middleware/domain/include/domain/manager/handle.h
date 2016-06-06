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


            namespace mandatory
            {

               //!
               //! Boots the mandatory parts
               //!
               //! @param state
               //!
               void boot( State& state);
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


            namespace scale
            {

               struct Executable : Base
               {
                  using Base::Base;

                  void operator () ( common::message::domain::scale::Executable& executable);
               };

            } // scale





            namespace process
            {
               namespace termination
               {
                  struct Registration : Base
                  {
                     using Base::Base;

                     void operator () ( const common::message::domain::process::termination::Registration& message);
                  };


                  void event( const common::process::lifetime::Exit& exit);

                  struct Event : Base
                  {
                     using Base::Base;

                     void operator () ( common::message::domain::process::termination::Event& message);
                  };
               } // termination

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
               namespace transaction
               {
                  struct Resource : public Base
                  {
                     using Base::Base;

                     void operator () ( const common::message::domain::configuration::transaction::resource::Request& message);
                  };

               } // transaction

               struct Gateway : public Base
               {
                  using Base::Base;

                  void operator () ( const common::message::domain::configuration::gateway::Request& message);
               };

            } // configuration

         } // handle


         common::message::dispatch::Handler handler( State& state);

      } // manager
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_
