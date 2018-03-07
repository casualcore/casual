//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_TRANSFORM_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_TRANSFORM_H_


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

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_TRANSFORM_H_
