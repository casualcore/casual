//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_TRANSFORM_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_TRANSFORM_H_

#include "configuration/message/domain.h"
#include "configuration/domain.h"

namespace casual
{
   namespace configuration
   {
      namespace transform
      {

         message::Domain configuration( const configuration::domain::Domain& domain);


      } // transform
   } // configuration


} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_TRANSFORM_H_
