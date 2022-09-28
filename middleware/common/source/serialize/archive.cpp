//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/archive.h"

#include "common/exception/capture.h"

namespace casual
{
   namespace common::serialize
   {
      Reader::~Reader() = default;
      Writer::~Writer() = default;

      Writer::Writer( Writer&&) noexcept = default;
      Writer& Writer::operator = ( Writer&&) noexcept = default;

   } // common::serialize
} // casual

