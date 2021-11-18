//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/user.h"
#include "configuration/model.h"

namespace casual
{
   namespace configuration::example
   {

      namespace user::part
      {
         configuration::user::Model system();

         namespace domain
         {
            //! server, executable, environment, name...
            configuration::user::Model general();
            configuration::user::Model service();
            configuration::user::Model transaction();
            configuration::user::Model queue();
            configuration::user::Model gateway();
         } // domain
         
      } // user::part

      configuration::Model model();

   } // configuration::example
} // casual


