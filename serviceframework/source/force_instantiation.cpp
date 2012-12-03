//!
//! casual_sf_force_instantiation.cpp
//!
//! Created on: Nov 12, 2012
//!     Author: Lazan
//!


#include "sf/archive_yaml_policy.h"

#include <istream>

namespace casual
{
   namespace sf
   {
         template class archive::basic_reader< policy::reader::Yaml< policy::reader::Relaxed> >;

         template class archive::basic_writer< policy::writer::Yaml>;

         void someTest()
         {
            std::istream* bla = nullptr;
            archive::basic_reader< policy::reader::Yaml< policy::reader::Relaxed> > test( *bla);
         }

   } // sf
} // casual



