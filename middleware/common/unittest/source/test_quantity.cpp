//! 
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/quantity.h"

#include "casual/platform.h"

#include <string>
#include <utility>

namespace casual::common::quantity
{
   TEST( common_quantity, bytes_from_valid_string)
   {
      const std::vector< std::pair< std::string, platform::size::type>> representation_map{
         { "2TiB", 2'199'023'255'552},
         { "4 TB", 4'000'000'000'000},
         { "14 GiB", 15'032'385'536},
         { "2GB", 2'000'000'000},
         { "400MiB", 419'430'400},
         { "8MB", 8'000'000},
         { "42KiB", 43'008},
         { "8 kB", 8'000},
         { "59 382 B", 59382},
         { "666", 666}
      };

      for( auto& [ representation, value] : representation_map)
      {
         auto result = bytes::from::string( representation);
         EXPECT_TRUE( result == value) << "Expected: " << value << ", actual: " << result;
      }
   }

   TEST( common_quantity, bytes_from_invalid_string__expect_raise_error)
   {
      const std::vector< std::string> invalid_representations{
         { "t2TiB"},
         { "15 FiB"},
         { "1.4 GiB"},
         { "KiB"},
         { "4,1MiB"},
         { "8M4B"},
         { "__"},
         { "     "},
         { "-43kB"},
         { "<666>"}
      };

      for( auto& representation : invalid_representations)
         EXPECT_THROW( bytes::from::string( representation), std::system_error);
   }

   TEST( common_quantity, bytes_to_string)
   {
      const std::vector< std::pair< platform::size::type, std::string>> value_map{
         { 2'199'023'255'552, "2TiB"},
         { 4'000'000'000'000, "4TB"},
         { 15'032'385'536, "14GiB", },
         { 2'000'000'000, "2GB"},
         { 419'430'400, "400MiB"},
         { 8'000'000, "8MB"},
         { 43'008, "42KiB"},
         { 8'000, "8kB",},
         { 59382, "59382B"},
         { 666, "666B"}
      };

      for( auto& [ value, representation] : value_map)
      {
         auto result = bytes::to::string( value);
         EXPECT_TRUE( result == representation) << "Expected: " << representation << ", actual: " << result;
      }
   }
} // casual::common::quantity
