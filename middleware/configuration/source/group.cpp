//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/group.h"

#include "common/algorithm.h"

namespace casual
{
   namespace configuration
   {
      bool operator == ( const Group& lhs, const Group& rhs)
      {
         return lhs.name == rhs.name;
      }

      Group& Group::operator += ( const Group& rhs)
      {
         if( dependencies && rhs.dependencies)
         {
            auto& l_range = dependencies.value();
            auto& r_range = rhs.dependencies.value();

            l_range.insert( std::end( l_range), std::begin( r_range), std::end( r_range));

            common::algorithm::trim( l_range, common::algorithm::unique( common::algorithm::sort( l_range)));
         }
         else
         {
            dependencies = common::coalesce( rhs.dependencies, dependencies);
         }

         return *this;
      }
   } // configuration
} // casual
