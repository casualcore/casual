//!
//! service.cpp
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#include "sf/service.h"

namespace casual
{
   namespace sf
   {
      namespace service
      {
         Interface::~Interface()
         {

         }

         bool Interface::call()
         {
            return doCall();
         }
         void Interface::finalize()
         {
            doFinalize();
         }

         Interface::Input& Interface::input()
         {
            return doInput();
         }

         Interface::Output& Interface::output()
         {
            return doOutput();
         }

      }


 // service
   } // sf
} // casual

