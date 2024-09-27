//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/argument.h"
#include "common/log/line.h"
#include "common/log.h"

#include <print>
#include <iostream>

namespace casual
{
   using namespace common;

   namespace argument
   {
      namespace local
      {
         namespace
         {
            namespace traverse
            {
               template< typename O>
               struct State
               {
                  State suboptions( std::span< O> suboptions) const
                  {
                     State result;
                     result.parents = parents;
                     result.parents.push_back( options);
                     result.options = suboptions;
                     return result;
                  }
                  
                  std::span< O> options;
                  std::vector< std::span< O>> parents;
               };
            } // traverse



            namespace find
            {
               auto next( std::span< const Option> options, range_type arguments)
               {
                  return common::algorithm::find_if( arguments, [ &options]( auto argument)
                  {
                     return common::algorithm::contains( options, argument);
                  });
               };

               auto parent( auto parents, range_type arguments)
               {
                  for( auto options : std::views::reverse( parents))
                  {
                     if( auto found = next( options, arguments))
                        return found;
                  }
                  return decltype( next( parents.front(), arguments)){};
               };
               
            } // find

            inline range_type assign( auto state, range_type arguments, auto assign, auto assign_end)
            {

               constexpr static auto option_assign = []( auto& option, std::string_view key, range_type current, range_type found, auto assign)
               {
                  // pinpoint the argument range that we'll assign
                  auto [ chosen, left] = common::algorithm::divide_at( current, std::begin( found));

                  assign( option, key, chosen);

                  return left;
               };

               while( ! arguments.empty())
               {
                  // first one has to be a key
                  auto key = arguments.front();
                  auto current = arguments.subspan( 1);

                  if( auto option = common::algorithm::find( state.options, key))
                  {
                     // first we need to check the suboptions for the next "key"
                     if( auto found = find::next( option->suboptions(), current))
                     {
                        auto left = option_assign( *option, key, current, found, assign);

                        // continue with the suboption
                        arguments = local::assign( state.suboptions( option->suboptions()), left, assign, assign_end);
                     }
                     // otherwise we check the siblings for the next "key"
                     else if( auto found = find::next( state.options, current))
                     {
                        // we've found the "next key" in this "level", we let the while loop continue
                        arguments = option_assign( *option, key, current, found, assign);
                     }
                     // otherwise we check parents for the next "key"
                     else if( auto found = find::parent( state.parents, current))
                     {
                        // we know that the key was found in some of the parents, let "parent assign" try to handle it
                        return option_assign( *option, key, current, found, assign);
                     }
                     // otherwise, we haven't found any "next key", we assign all arguments to option
                     else 
                     {
                        assign_end( *option, key, current);
                        return {};
                     }
                  }
                  else
                  {
                     // we couldn't find the key, let "parent assign" try to handle it.
                     return arguments;
                  }
               }
               return {};
            }

            std::vector< detail::option::Assigned> assign( std::span< Option> options, range_type arguments)
            {
               std::vector< detail::option::Assigned> assigned;

               auto assign = [ &assigned]( auto& option, auto key, auto arguments)
               {
                  if( auto invocable = option.assign( key, arguments))
                     assigned.push_back( std::move( *invocable));
               };

               auto left = local::assign( traverse::State< Option>{ .options = options}, arguments, assign, assign);

               if( ! left.empty())
                  common::code::raise::error( common::code::casual::invalid_argument, "failed to find option: ", left.front());

               return assigned;
            }

            namespace validate
            {
               inline void options( std::span< const Option> options)
               {
                  auto validate = []( const auto& option)
                  {
                     auto assigned = option.assigned();
                     detail::validate::cardinality( option.cardinality(), option.names().canonical(), assigned);
                     
                     // if the option is assigned/used, we need to validate the suboptions. 
                     if( assigned > 0)
                        validate::options( option.suboptions());
                  };

                  std::ranges::for_each( options, validate);
               }
               
            } // validate

            namespace help
            {
               template< typename... Ts>
               constexpr void output( platform::size::type indent, std::format_string< Ts...> format, Ts&&... ts)
               {
                  // std::string space( indent, ' ');
                  // std::print( "{}", space);
                  // we can't capture stdout right now. We need to use ostream to be able to unittest
                  //std::print( format, std::forward< Ts>( ts)...);
                  std::string out( indent, ' ');
                  std::format_to( std::back_inserter( out), format, std::forward< Ts>( ts)...);
                  std::cout << out;
               }

               template< typename... Ts>
               constexpr void output( std::format_string< Ts...> format, Ts&&... ts)
               {
                  output( 0, format, std::forward< Ts>( ts)...);
               }

               std::string format_option_cardinality( const Cardinality& cardinality)
               {
                  if( cardinality.many()) return std::format( "{}..*", cardinality.min());
                  if( cardinality.fixed()) return string::to( cardinality.min());
                  return std::format( "{}..{}", cardinality.min(), cardinality.max());
               }

               void description( std::string_view description, platform::size::type indent)
               {
                  for( auto line : string::split( description, '\n'))
                     output( indent, "{}\n", line);
               }

               void print( std::span< const Option> options, platform::size::type indent);
               
               void print( const Option& option, platform::size::type indent = 0)
               {
                  output( indent, "{} [{}]\n", string::join( option.names().active(), ", "), format_option_cardinality( option.cardinality()));
                  description( option.description(), indent + 3);
                  output( "\n");

                  if( ! option.suboptions().empty())
                  {
                     output( indent + 3, "SUB OPTIONS:\n\n");
                     help::print( option.suboptions(), indent + 6);
                  }


               }

               void print( std::span< const Option> options, platform::size::type indent = 0)
               {
                  for( auto& option : options)
                     print( option, indent);
               }

               void print_all( std::string_view description, std::span< const Option> options)
               {
                  output( "NAME\n   {}\n\n", process::path().filename().string());
                  output( "DESCRIPTION\n\n");
                  help::description( description, 3);

                  output( "\nOPTIONS\n\n");
                  print( options, 3);
               }

               void print( std::string_view description, std::span< const Option> options, range_type arguments)
               {
                  if( arguments.empty())
                     return print_all( description, options);

                  // use the assign algorithm to find the 'deepest' referenced options

                  auto discard_assign = []( auto& option, auto key, auto arguments)
                  {};

                  auto print_option = []( auto& option, auto key, auto arguments)
                  {
                     print( option);
                  };

                  //! if the assign algorithm didn't consume all of the arguments, we didn't find anything.
                  if( ! local::assign( traverse::State< const Option>{ .options = options}, arguments, discard_assign, print_option).empty())
                     print_all( description, options);
               }
            } // help

            
         } // <unnamed>
      } // local

      namespace detail
      {
         namespace validate
         {
            void cardinality( const Cardinality& cardinality, std::string_view key, size_type value)
            {
               if( ! cardinality.valid( value))
                  common::code::raise::error( common::code::casual::invalid_argument, "cardinality not satisfied for option: ", key);
            }

            namespace value
            {
               void cardinality( const Cardinality& cardinality, std::string_view key, range_type values) 
               {
                  if( ! cardinality.valid( values.size()))
                     common::code::raise::error( common::code::casual::invalid_argument, "cardinality not satisfied for values to option: '", key, 
                        "' - ", cardinality, ", values: ", values);
               }
            } // value

         } // validate

         std::vector< detail::option::Assigned> assign( std::span< Option> options, range_type arguments)
         {
            return local::assign( options, arguments);
         }

         void parse( std::span< Option> options, range_type arguments)
         {
            auto assigned = local::assign( options, arguments);

            local::validate::options( options);

            auto is_preemptive = []( auto& assigned){ return assigned.phase() == decltype( assigned.phase())::preemptive;};

            auto [ first, second] = common::algorithm::stable::partition( assigned, is_preemptive);

            std::ranges::for_each( first, std::mem_fn( &detail::option::Assigned::invoke));
            std::ranges::for_each( second, std::mem_fn( &detail::option::Assigned::invoke));
         }

         bool Policy::help( std::string_view description, std::span< Option> options, range_type arguments, std::span< std::string_view> keys)
         {
            if( auto found = algorithm::find_first_of( arguments, keys))
            {
               algorithm::rotate( arguments, found);
               local::help::print( description, options, arguments.subspan( 1));
               return true;
            }

            return false;
         }

         std::span< std::string_view> Policy::help_names()
         {
            static std::array< std::string_view, 1> names{ reserved::name::help};
            return names;
         }
         

      } // detail

      
   } // argument
   
} // casual