//!
//! resource_proxy.h
//!
//! Created on: Aug 5, 2013
//!     Author: Lazan
//!

#ifndef RESOURCE_PROXY_H_
#define RESOURCE_PROXY_H_

#include "transaction/resource/proxy_server.h"

#include "common/platform.h"

namespace casual
{
   namespace transaction
   {

      namespace resource
      {


         struct State
         {
            std::string rm_key;
            std::string rm_openinfo;
            std::string rm_closeinfo;

            casual_xa_switch_mapping* xaSwitches = nullptr;
            common::platform::queue_id_type tm_queue = 0;
         };

         namespace validate
         {
            void state( const State& state);
         } // validate


         class Proxy
         {
         public:

            Proxy( State&& state);

            void start();

            State& state() { return m_state;}

         private:
            State m_state;
         };



      } // resource

   } // transaction

} // casual

#endif // RESOURCE_PROXY_H_
