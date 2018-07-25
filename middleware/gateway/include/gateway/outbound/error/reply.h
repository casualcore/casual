//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/outbound/route.h"

namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         namespace error
         {
            //!
            //! Tries to send error replise to the in-flight routes  
            //!
            //! @param route 
            //!
            void reply( const route::Route& route);
            void reply( const route::service::Route& route);

         } // error
      } // outbound
   } // gateway
} // casual