//!
//! casual_sf_force_instantiation.cpp
//!
//! Created on: Nov 12, 2012
//!     Author: Lazan
//!


#include "casual_archive_yaml_policy.h"

#include <istream>

namespace casual
{
   namespace sf
   {
         template class archive::basic_reader< policy::yaml::Reader>;

         template class archive::basic_writer< policy::yaml::Writer>;

         void someTest()
         {
            std::istream* bla;
            archive::basic_reader< policy::yaml::Reader> test( *bla);
         }

   } // sf
} // casual



