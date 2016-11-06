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

               struct Queue : public Base
               {
                  using Base::Base;

                  void operator () ( const common::message::domain::configuration::queue::Request& message);
               };

            } // configuration

         } // handle


         handle::dispatch_type handler( State& state);

      } // manager
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_HANDLE_H_
