//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/buffer/internal/field.h"

#include "common/environment.h"
#include "common/unittest/file.h"

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace field
      {

         TEST( buffer_field_table, no_files)
         {
            unittest::Trace trace;

            {
               auto table = internal::detail::id_to_name( {});
               EXPECT_TRUE( table.empty());
            }
            {
               auto table = internal::detail::name_to_id( {});
               EXPECT_TRUE( table.empty());
            }

         }

         namespace local
         {
            namespace
            {
               auto file_1() 
               {
                  return unittest::file::temporary::content( ".yaml", R"(
groups:
   - base: 1000
     fields:
      - id: 1
        name: FLD_SHORT1
        type: short

      - id: 2
        name: FLD_SHORT2
        type: short

   - base: 2000
     fields:
      - id: 1
        name: FLD_STRING1
        type: string

      - id: 2
        name: FLD_STRING2
        type: string 

                  )");
               }

               auto file_2() 
               {
                  return unittest::file::temporary::content( ".json", R"(
{
  "groups": [
    {
      "base": 3000,
      "fields": [
        {
          "id": 1,
          "name": "FLD_LONG1",
          "type": "long"
        },
        {
          "id": 2,
          "name": "FLD_LONG2",
          "type": "long"
        }
      ]
    }
  ]
}
                  )");
               }
            } // <unnamed>
         } // local


         TEST( buffer_field_table, one_file)
         {
            unittest::Trace trace;

            auto file = local::file_1();

            {
               auto table = internal::detail::id_to_name( { file.path()});
               EXPECT_TRUE( table.size() == 4);
            }
            {
               auto table = internal::detail::name_to_id( { file.path()});
               EXPECT_TRUE( table.size() == 4);
            }
         }


         TEST( buffer_field_table, two_files)
         {
            unittest::Trace trace;

            auto file_1 = local::file_1();
            auto file_2 = local::file_2();

            {
               auto table = internal::detail::id_to_name( { file_1.path(), file_2.path()});
               EXPECT_TRUE( table.size() == 6);
            }
            {
               auto table = internal::detail::name_to_id( { file_1.path(), file_2.path()});
               EXPECT_TRUE( table.size() == 6);
            }
         }

         TEST( buffer_field_table, two_files_via_CASUAL_FIELD_TABLE)
         {
            unittest::Trace trace;

            auto file_1 = local::file_1();
            auto file_2 = local::file_2();

            environment::variable::set( "CASUAL_FIELD_TABLE", string::compose( file_1, '|', file_2));

            {
               auto table = internal::detail::id_to_name();
               EXPECT_TRUE( table.size() == 6);
            }
            {
               auto table = internal::detail::name_to_id();
               EXPECT_TRUE( table.size() == 6);
            }
         }
         
      } // field
   } // buffer
   
} // casual