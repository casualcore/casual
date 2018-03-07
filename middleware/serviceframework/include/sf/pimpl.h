//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef SF_PIMPL_H_
#define SF_PIMPL_H_

#include "common/pimpl.h"


namespace casual
{

   namespace sf
   {
      namespace move
      {
         template< typename T>
         using Pimpl = common::move::basic_pimpl< T>;

      } // move

      template< typename T>
      using Pimpl = common::basic_pimpl< T>;

   } // sf
} // casual

#endif // PIMPL_H_
