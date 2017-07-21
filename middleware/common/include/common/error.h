//!
//! casual_errorhandler.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ERRORHANDLER_H_
#define CASUAL_ERRORHANDLER_H_

#include <string>
#include <system_error>

namespace casual
{
   namespace common
   {
      namespace error
      {

         //!
         //! @return the last error number, ie from errno
         //!
         int last();

         //!
         //! Get the last error condition from the system
         //!
         //! @return the last error condition, if any
         //!
         std::error_condition condition();



         enum Errno
         {
            cNoChildProcesses = 10
         };

         int handler();

         //!
         //! @return string representation of errno
         //!
         std::string string();

         std::string string( int code);



      } // error
   } // common
} // casual



#endif /* CASUAL_ERRORHANDLER_H_ */
