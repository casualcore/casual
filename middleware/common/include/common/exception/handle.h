//!
//! casual
//!

#ifndef CASUAL_ERRORHANDLER_H_
#define CASUAL_ERRORHANDLER_H_

#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace exception
      {

         //!
         //! throws pending exception, catches it and log based on the exception. returns the 
         //! corresponding error code.
         //!
         int handle() noexcept;

         int handle( std::ostream& out) noexcept;

      } // error
   } // common
} // casual



#endif /* CASUAL_ERRORHANDLER_H_ */
