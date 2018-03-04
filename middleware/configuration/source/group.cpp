//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/group.h"


namespace casual
{
   namespace configuration
   {
      namespace group
      {

         Group::Group() = default;
         Group::Group( std::function< void(Group&)> foreign) { foreign( *this);}


         bool operator == ( const Group& lhs, const Group& rhs)
         {
            return lhs.name == rhs.name;
         }


      } // group
   } // configuration
} // casual
