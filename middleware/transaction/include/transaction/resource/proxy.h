//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef RESOURCE_PROXY_H_
#define RESOURCE_PROXY_H_

#include "transaction/resource/proxy_server.h"

#include "common/platform.h"
#include "common/strong/id.h"
#include "common/transaction/resource.h"


#include "sf/namevaluepair.h"

namespace casual
{
   namespace transaction
   {

      namespace resource
      {
         namespace proxy 
         {
            struct Settings
            {
               common::strong::resource::id::value_type id;
               std::string key;
               std::string openinfo;
               std::string closeinfo;
            };

            struct State
            {
               State( Settings&& settings, casual_xa_switch_mapping* switches);

               common::transaction::Resource resource;
            };
         } // proxy 


         class Proxy
         {
         public:

            Proxy( proxy::Settings&& settings, casual_xa_switch_mapping* switches);
            ~Proxy();

            void start();

            proxy::State& state() { return m_state;}

         private:
            proxy::State m_state;
         };



      } // resource

   } // transaction

} // casual

#endif // RESOURCE_PROXY_H_
