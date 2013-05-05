//!
//! casual_errorhandler.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ERRORHANDLER_H_
#define CASUAL_ERRORHANDLER_H_

#include <string>

namespace casual
{
   namespace utility
   {
      namespace error
      {

         int handler();

         std::string stringFromErrno();

         const std::string& tperrnoStringRepresentation( int error);

      }
   }
}



#endif /* CASUAL_ERRORHANDLER_H_ */
