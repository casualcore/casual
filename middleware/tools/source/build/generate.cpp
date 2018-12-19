//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/generate.h"
#include "tools/common.h"

namespace casual
{
   namespace tools
   {
      namespace build
      {
         namespace generate
         {
            namespace local
            {
               namespace
               {
                  std::string before_main( const std::vector< Resource>& resource)
                  {
                     std::ostringstream out;

                     out << "\n\n";

                     // Declare the xa_struts
                     for( auto& rm : resource)
                     {
                        out << "extern struct xa_switch_t " << rm.xa_struct_name << ";" << '\n';
                     }
                     

                     return std::move( out).str();
                  }

                  std::string inside_main( const std::vector< Resource>& resource)
                  {
                     std::ostringstream out;

                     out << R"(
   struct casual_xa_switch_map xa_mapping[] = {)";

                  for( auto& rm : resource)
                  {
                     out << R"(
      { ")" << rm.key <<  R"(", ")" << rm.name <<  R"(", &)" << rm.xa_struct_name << "},";
                  }

                  out << R"(
      { 0, 0, 0} /* null ending */
   };
                     )";


                     return std::move( out).str();
                  }


                  
               } // <unnamed>
            } // local

            Content resources( const std::vector< Resource>& resources)
            {
               Trace trace{ "tools::build::generate::resources"};

               return { local::before_main( resources), local::inside_main( resources)};
            }

         } // generate
      } // build
   } // tools
} // casual