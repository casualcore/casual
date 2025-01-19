//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <optional>
#include <filesystem>

namespace casual
{
   namespace file
   {
      inline namespace v1  
      {
         inline namespace blocking
         {
            //! reserve a path with blocking wait
            [[nodiscard]] auto reserve( std::filesystem::path path) -> std::filesystem::path;
         } // blocking


         namespace non
         {
            namespace blocking
            {
               //! try to reserve a path
               [[nodiscard]] auto reserve( std::filesystem::path path) -> std::optional< std::filesystem::path>;
            } // blocking
         } // non
      } // v1
   } // file
} // casual


