//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/transaction/resource/proxy/server.h"
#include "casual/transaction/id.h"

#include "transaction/resource.h"

#include "casual/platform.h"


#include "common/serialize/macro.h"

namespace casual
{
   namespace transaction::resource
   {
      namespace proxy 
      {
         struct Settings
         {
            resource::id id{};
         };

         struct State
         {
            Resource resource;
         };
      } // proxy 


      class Proxy
      {
      public:

         Proxy( proxy::Settings settings, casual_xa_switch_mapping* switches);
         ~Proxy();

         void start();

         proxy::State& state() { return m_state;}

      private:
         proxy::State m_state;
      };

   } // transaction::resource
} // casual

