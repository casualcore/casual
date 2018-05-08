//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "configuration/domain.h"

#include "common/message/domain.h"

namespace casual
{
   namespace configuration
   {
      namespace transform
      {

         common::message::domain::configuration::Domain configuration( const configuration::domain::Manager& domain);


      } // transform
   } // configuration


} // casual


