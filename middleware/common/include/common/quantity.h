//! 
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include <string>

namespace casual
{
   namespace common::quantity
   {
      namespace bytes
      {
         using bytes_type = platform::size::type;

         namespace unit
         {
            constexpr bytes_type byte = 1;

            constexpr bytes_type kilobyte = 1000;
            constexpr bytes_type kibibyte = 1024;

            constexpr bytes_type megabyte = 1'000'000;
            constexpr bytes_type mebibyte = 1'048'576;

            constexpr bytes_type gigabyte = 1'000'000'000;
            constexpr bytes_type gibibyte = 1'073'741'824;

            constexpr bytes_type terabyte = 1'000'000'000'000;
            constexpr bytes_type tebibyte = 1'099'511'627'776;
         } // unit

         namespace symbol
         {
            constexpr auto byte = "B";

            constexpr auto kilobyte = "kB";  // lower case intentional, see: IEC 80000-13
            constexpr auto kibibyte = "KiB";

            constexpr auto megabyte = "MB";
            constexpr auto mebibyte = "MiB";

            constexpr auto gigabyte ="GB";
            constexpr auto gibibyte = "GiB";

            constexpr auto terabyte = "TB";
            constexpr auto tebibyte = "TiB";
         } // symbol

         namespace from
         {
            bytes_type string( std::string value);
         } // from

         namespace to
         {
            std::string string( bytes_type value);
         } // to

      } // bytes
   } // common::quantity
} // casual
