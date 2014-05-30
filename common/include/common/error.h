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
   namespace common
   {
      namespace error
      {
         enum Errno
         {
            cNoChildProcesses = 10
         };

         int handler();

         //!
         //! @return string representation of errno
         //!
         std::string string();


         namespace xatmi
         {
            std::string error( int error);
         } // xatmi

         namespace xa
         {
            const char* error( int code);
         } // xa

         namespace tx
         {
            const char* error( int code);
         } // tx

      } // error
   } // common
} // casual



#endif /* CASUAL_ERRORHANDLER_H_ */
