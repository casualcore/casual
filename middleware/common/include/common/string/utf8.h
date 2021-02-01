//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <string>

//! TODO: Even though this might be an efficient way to use for dispatch of
//! UTF-8-content, this should be removed with C++20 (in favour of 
//! std::u8string only) to reduce clutter in the casual code base, despite it
//! initially might induce an extra copy in some cisrcumstances, but if user 
//! really want UTF-8, the way to go is to bow down to the history of C++ and
//! use std::u8string from the beginning 

namespace casual
{
   namespace common
   {
      namespace string
      {
         template<typename T>
         struct utf8_wrapper : std::reference_wrapper< T>
         {
            using std::reference_wrapper< T>::reference_wrapper;
         };

         using utf8 = utf8_wrapper< std::string>;

         namespace immutable
         {
            using utf8 = utf8_wrapper< const std::string>;
         } // immutable
      } // string
   } // common
} // casual