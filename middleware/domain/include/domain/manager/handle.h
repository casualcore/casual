//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"
#include "domain/manager/ipc.h"
#include "domain/manager/task.h"
#include "domain/manager/admin/model.h"

#include "common/message/type.h"
#include "common/message/domain.h"
#include "common/message/dispatch.h"


namespace casual
{
   namespace domain
   {
      namespace manager
      {
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

            namespace start
            {
               namespace pending
               {
                  common::Process message();
               } // pending
            } // start

            
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

               void shutdown( State& state, std::vector< common::process::Handle> processes);

               void instances( State& state, state::Server& server);
               void instances( State& state, state::Executable& executable);

               std::vector< common::strong::task::id> aliases( State& state, std::vector< admin::model::scale::Alias> aliases);

               namespace prepare
               {
                  struct Shutdown : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::domain::process::prepare::shutdown::Reply& message);
                  };
               } // prepare

            } // scale

            

            namespace restart
            {
               std::vector< common::strong::task::id> instances( State& state, std::vector< std::string> aliases);
            } // restart

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


