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
            void Proxie::operator () ( const state::resource::Proxy& proxy)
            {
               for( auto index = proxy.concurency; index > 0; --index)
               {
                  auto& info = m_state.xaConfig.at( proxy.key);

                  state::resource::Proxy::Instance instance( proxy.id);

                  instance.server.pid = process::spawn(
                        info.server,
                        {
                              "--tm-queue", std::to_string( ipc::receive::id()),
                              "--rm-key", info.key,
                              "--rm-openinfo", proxy.openinfo,
                              "--rm-closeinfo", proxy.closeinfo,
                              "--rm-id", std::to_string( proxy.id)
                        }
                     );

                  instance.state = state::resource::Proxy::Instance::State::started;

                  auto order = std::lower_bound( std::begin( m_state.instances), std::end( m_state.instances), instance);

                  m_state.instances.insert( order, std::move( instance));
               }
            }

         } // boot

      } // action
   } //transaction
} // casual
