//!
//! test_vo.h
//!
//! Created on: Mar 31, 2013
//!     Author: Lazan
//!

#ifndef TEST_VO_H_
#define TEST_VO_H_


#include "sf/namevaluepair.h"

namespace casual
{
   namespace test
   {

      struct SimpleVO
      {
         long m_long;
         std::string m_string;
         short m_short;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( m_long);
            archive & CASUAL_MAKE_NVP( m_string);
            archive & CASUAL_MAKE_NVP( m_short);
         }

      };

   }

}



#endif /* TEST_VO_H_ */
