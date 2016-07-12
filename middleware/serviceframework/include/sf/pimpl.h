//!
//! casual
//!

#ifndef SF_PIMPL_H_
#define SF_PIMPL_H_

#include "common/pimpl.h"


namespace casual
{

   namespace sf
   {
      template< typename T>
      using Pimpl = common::basic_pimpl< T>;

   } // sf
} // casual

#endif // PIMPL_H_
