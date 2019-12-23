//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/environment/string.h"
#include "common/environment.h"

#include "common/algorithm.h"
#include "common/range/adapter.h"

namespace casual
{
   namespace common
   {
      namespace environment
      {
         namespace local
         {
            namespace
            {
               template< typename F>
               std::string string( std::string value, F&& get_variable)
               {
                  std::ostringstream out;

                  auto range = range::make( value);
                  
                  // replace all '}' with '\0' to be c-compatible
                  // we'll use the view::String later to find the actual
                  // environment variable via c-api (with const char*), hence
                  // it need to be null terminated.
                  algorithm::replace( range, '}', '\0');
                  
                  // split the range to _the end of variable_ (if any) (which is now '\0')
                  auto next_range = []( auto range){ return algorithm::split( range, '\0');};
                  auto handle_part = [&out, &get_variable]( auto range)
                  {
                     // find _the beginning of variable_ (if any).
                     auto split = algorithm::divide_first( range, "${");

                     // might be the whole value/string
                     out << view::String{ std::get< 0>( split)};

                     auto variable = view::String{ std::get< 1>( split)};
                     
                     if( ! variable)
                        return;

                     // get rid of the found "${"
                     variable.advance( 2);

                     // let the functor take care of extracting the variable
                     // it might be from a "local repository"
                     // `variable` is now a view::String with null termination.
                     get_variable( out, variable);
                  };

                  algorithm::for_each( 
                     range::adapter::make( next_range, range),
                     handle_part); 

                  return std::move( out).str();

               }

            } // <unnamed>
         } // local
         
         std::string string( std::string value)
         {
            return local::string( std::move( value), []( auto& out, auto variable)
            {
               out << variable::get( variable);
            });
         }

         std::string string( std::string value, const std::vector< environment::Variable>& local)
         {
            return local::string( std::move( value), [&local]( auto& out, auto variable)
            {
               auto find_local = [variable]( auto& v){ return v.name() == variable;};
               // try to find it in local repository first
               if( auto found = algorithm::find_if( local, find_local))
                  out << found->value();
               else
                  out << variable::get( variable);
            });
         }

      } // environment
   } // common
} // casual