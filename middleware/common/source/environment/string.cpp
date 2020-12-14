//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/environment/string.h"
#include "common/environment.h"

#include "common/string/view.h"
#include "common/algorithm.h"
#include "common/range/adapter.h"
#include "common/log.h"


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
                  
                  // split the range to the next beginning of the variable, if any
                  auto next_range = []( auto range)
                  { 
                     const auto delimiter = std::string_view{ "${"};

                     auto result = algorithm::divide_search( range, delimiter);
                     if( std::get< 0>( result).empty() && ! std::get< 1>( result).empty())
                     {
                        auto second = std::get< 1>( result);
                        auto found = algorithm::search( second + 2, delimiter);

                        return std::make_tuple( range::make( std::begin( second), std::begin( found)), found);
                     }
                     return result;
                  };

                  auto handle_part = [&out, &get_variable]( auto range)
                  {
                     // check if this 'sub-range' is a _variable indirection_ (minimal possible variable is ${X} )
                     auto is_variable_declaration = []( auto range)
                     {
                        if( range.size() < 4)
                           return false;

                        return algorithm::equal( range::make( std::begin( range), 2), std::string_view{ "${"});
                     };

                     if( ! is_variable_declaration( range))
                     {
                        out << string::view::make( range);
                        return;
                     }

                     // find the end of the declaration
                     auto found = algorithm::find( range, '}');

                     if( ! found)
                     {
                        // not a correct variable, we dont replace
                        out << string::view::make( range);
                        return;
                     }

                     // ok, now we have a "correct" variable, lets resolve it...

                     // get rid of the "${"
                     range.advance( 2);

                     // replace '}' with '\0' to be c-compatible, for variable 'lookup'
                     *found = '\0';

                     auto variable = string::view::make( std::begin( range), std::begin( found));

                     // let the functor take care of extracting the variable
                     // it might be from a "local repository"
                     // `variable` is now a std::string_view with null termination.
                     get_variable( out, variable);

                     // handle the rest, which might be _noting_
                     out << string::view::make( std::begin( found + 1), std::end( range));
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