//!
//! action.cpp
//!
//! Created on: Oct 20, 2013
//!     Author: Lazan
//!

#include "transaction/manager/action.h"

#include "common/ipc.h"
#include "common/process.h"

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
            void Proxie::operator () ( const std::shared_ptr< state::resource::Proxy>& proxy)
            {
               for( auto index = proxy->concurency; index > 0; --index)
               {
                  auto& info = m_state.xaConfig.at( proxy->key);

                  auto instance = std::make_shared< state::resource::Proxy::Instance>();

                  instance->id.pid = process::spawn(
                        info.server,
                        {
                              "--tm-queue", std::to_string( ipc::getReceiveQueue().id()),
                              "--rm-key", info.key,
                              "--rm-openinfo", proxy->openinfo,
                              "--rm-closeinfo", proxy->closeinfo,
                              "--rm-id", std::to_string( proxy->id)
                        }
                     );

                  m_state.instances.emplace( instance->id.pid, instance);
                  instance->proxy = proxy;

                  instance->state = state::resource::Proxy::Instance::State::started;

                  proxy->instances.emplace_back( std::move( instance));
               }
            }

         } // boot

      } // action
   } //transaction
} // casual
