//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/service/model.h"


namespace casual
{
   namespace tools
   {
      namespace service
      {
         namespace describe
         {
            std::vector< serviceframework::service::Model> invoke( const std::vector< std::string>& services);   
         } // describe
      } // service
   } // tools
} // casual


