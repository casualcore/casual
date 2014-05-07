//!
//! action.cpp
//!
//! Created on: Oct 20, 2013
//!     Author: Lazan
//!

#include "transaction/manager/action.h"
#include "transaction/manager/handle.h"

#include "common/ipc.h"
#include "common/process.h"
#include "common/internal/log.h"

#include <string>


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace action
      {
         namespace boot
         {
            void Proxie::operator () ( state::resource::Proxy& proxy)
            {
               for( auto index = proxy.concurency; index > 0; --index)
               {
                  auto& info = m_state.xaConfig.at( proxy.key);

                  state::resource::Proxy::Instance instance;//( proxy.id);
                  instance.id = proxy.id;

                  instance.server.pid = process::spawn(
                        info.server,
                        {
                              "--tm-queue", std::to_string( ipc::receive::id()),
                              "--rm-key", info.key,
                              "--rm-openinfo", proxy.openinfo,
                              "--rm-closeinfo", proxy.closeinfo,
                              "--rm-id", std::to_string( proxy.id),
                              "--domain", common::environment::domain::name()
                        }
                     );

                  instance.state = state::resource::Proxy::Instance::State::started;

                  proxy.instances.push_back( std::move( instance));
               }
            }

         } // boot


         namespace pending
         {

            bool Send::operator () ( state::pending::Reply& message) const
            {
               queue::non_blocking::Writer write{ message.target, m_state};
               if( ! write.send( message.message))
               {
                  common::log::internal::transaction << "failed to send reply - type: " << message.message.type << " to: " << message.target << "\n";
                  return false;
               }
               return true;
            }


         } // pending

      } // action
   } //transaction
} // casual
