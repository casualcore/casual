//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/generate.h"
#include "tools/common.h"

namespace casual
{
   using namespace common;
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
                  struct callbacks
                  {
                     using function_type = std::function< void( std::ostream&)>;
                     std::vector< function_type> top;
                     std::vector< function_type> before_main;
                     std::vector< function_type> inside_main;
                     std::vector< function_type> after_main;
                  };
                  auto before_main( const std::vector< model::Resource>& resources)
                  {
                     return [&resources]( std::ostream& out)
                     {
                        out << "\n";

                        // Declare the xa_struts
                        for( auto& rm : resources)
                        {
                           out << "extern struct xa_switch_t " << rm.xa_struct_name << ";" << '\n';
                        }
                     };
                  }

                  auto before_main( const std::vector< model::Service>& services)
                  {
                     return [&services]( std::ostream& out)
                     {
                        out << "\n";

                        // declare services
                        for( auto& service : services)
                        {
                           out << "extern void " << service.function << "( TPSVCINFO *context);" << '\n';
                        }
                     };
                  }
                  
                  auto inside_main( const std::vector< model::Resource>& resources)
                  {
                     return [&resources]( std::ostream& out)
                     {
                        out << R"(
   struct casual_xa_switch_map xa_mapping[] = {)";

                        for( auto& rm : resources)
                        {
                           out << R"(
      { ")" << rm.key <<  R"(", ")" << rm.name <<  R"(", &)" << rm.xa_struct_name << "},";
                        }

                        out << R"(
      { 0, 0, 0} /* null ending */
   };
                     )";
                        
                     };
                  }

                  auto inside_main( const std::vector< model::Service>& services)
                  {
                     return [&services]( std::ostream& out)
                     {
                        out << R"(
     struct casual_service_name_mapping service_mapping[] = {)";

                  for( auto& service : services)
                  {
                     out << R"(
      {&)" << service.function << R"(, ")" << service.name << R"(", ")" << service.category << R"(", )" << common::cast::underlying( service.transaction) << "},";
                  }

                  out << R"(
      { 0, 0, 0, 0} /* null ending */
   };
                        
                        )";

                     };
                  }

                  auto inside_main_start_server()
                  {
                     return []( std::ostream& out)
                     {
                        out << R"(

   struct casual_server_arguments arguments = {
         service_mapping,
         &tpsvrinit,
         &tpsvrdone,
         argc,
         argv,
         xa_mapping
   };

   return casual_run_server( &arguments);
)";

                     };
                  }

                  template< typename T>
                  auto text( T&& text)
                  {
                     return [text = std::move( text)]( std::ostream& out)
                     {
                        out << text;
                     };
                  }

                  void main_stream( std::ostream& out, const local::callbacks& callback)
                  {
                     auto invoke = [&out]( auto& f){ f( out);};

                     out << license::c << '\n';

                     algorithm::for_each( callback.top, invoke);
                     algorithm::for_each( callback.before_main, invoke);

                     out << "\n\nint main( int argc, char** argv)\n{\n";

                     algorithm::for_each( callback.inside_main, invoke);

                     out << "\n}\n";

                     algorithm::for_each( callback.after_main, invoke);
                  }
                  
               } // <unnamed>
            } // local


             void server( std::ostream& out, 
               const std::vector< model::Resource>& resources, 
               const std::vector< model::Service>& services)
            {
                Trace trace{ "tools::build::generate::server"};

                local::callbacks callback;
                callback.top.emplace_back( local::text( R"(   
#include <xatmi.h>
#include <xatmi/server.h>

#ifdef __cplusplus
extern "C" {
#endif

)"
               ));

               // before main
               callback.before_main.emplace_back( local::before_main( resources));
               callback.before_main.emplace_back( local::before_main( services));

               // inside main
               callback.inside_main.emplace_back( local::inside_main( resources));
               callback.inside_main.emplace_back( local::inside_main( services));
               callback.inside_main.emplace_back( local::inside_main_start_server());

               callback.after_main.emplace_back( local::text( R"(
#ifdef __cplusplus
}
#endif

)"
               ));

               local::main_stream( out, callback);
            }

            void executable( std::ostream& out, 
               const std::vector< model::Resource>& resources, 
               const std::string& entrypoint)
            {
                Trace trace{ "tools::build::generate::executable"};

                local::callbacks callback;
                callback.top.emplace_back( local::text( R"(   
#include <xatmi/executable.h>

#ifdef __cplusplus
extern "C" {
#endif

)"
               ));

               // before main
               callback.before_main.emplace_back( local::before_main( resources));
               callback.before_main.emplace_back( [&entrypoint]( std::ostream& out)
               {
                  out << "extern int " << entrypoint << "( int, char**);" << '\n';
               });

               // inside main
               callback.inside_main.emplace_back( local::inside_main( resources));
               callback.inside_main.emplace_back( [&entrypoint]( std::ostream& out)
               {
                  out << R"(

   struct casual_executable_arguments arguments = {
         )";
                     out << "&" << entrypoint << R"(,
         argc,
         argv,
         xa_mapping
   };

   return casual_run_executable( &arguments);
)";
               });

               callback.after_main.emplace_back( local::text( R"(
#ifdef __cplusplus
}
#endif

)"
               ));

               local::main_stream( out, callback);
            }


         } // generate
      } // build
   } // tools
} // casual