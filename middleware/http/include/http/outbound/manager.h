//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "http/outbound/configuration.h"
#include "http/outbound/state.h"

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         namespace manager
         {
            struct Settings
            {
               std::vector< std::string> configurations;
            };
         } // manager

         struct Manager : common::traits::unrelocatable
         {
            Manager( manager::Settings settings);
            ~Manager();


            void run();

         private:
            State m_state;
         };

      } // outbound
   } // http
} // casual