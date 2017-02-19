//!
//! casual
//!

#ifndef CHRONOLOGY_H_
#define CHRONOLOGY_H_

#include "common/platform.h"

#include <string>

namespace casual
{

   namespace common
   {

      namespace chronology
      {
         std::string local();
         std::string local( const platform::time::point::type& time);
         std::string universal();
         std::string universal( const platform::time::point::type& time);


         namespace from
         {
            std::chrono::microseconds string( const std::string& value);
         } // from


      }

   }

}


#endif /* CHRONOLOGY_H_ */
