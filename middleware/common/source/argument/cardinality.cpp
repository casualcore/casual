//!
//! casual
//!

#include "common/argument/cardinality.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace argument
      {
         namespace cardinality
         {
            static_assert( zero_one() + one_many() == one_many(), "");
            static_assert( fixed< 4>() + one_many() == min< 5>(), "");
            static_assert( one_many() + fixed< 4>() == min< 5>(), "");

            static_assert( fixed< 20>() + fixed< 22>() == fixed< 42>(), "");
         }


         std::ostream& operator << ( std::ostream& out, Cardinality value)
         {
            out  << value.min();
            if( value.fixed())
               return out;

            if( value.many()) 
               out << "..*";
            else 
               out << ".." << value.max();

            if( value.step() > 1)
               return out << " {" << value.step() << "}";
            
            return out;
         }
      }
   }
}