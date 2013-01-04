//!
//! template_vo.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef TEMPLATE_VO_H_
#define TEMPLATE_VO_H_



#include <sf/namevaluepair.h>

#include <string>


namespace test
{

   namespace vo
   {
      struct Value
      {
         std::string someString;
         long someLong;
         short someShort;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( someString);
            archive & CASUAL_MAKE_NVP( someLong);
            archive & CASUAL_MAKE_NVP( someShort);
         }
      };


   }
}


#endif /* TEMPLATE_VO_H_ */
