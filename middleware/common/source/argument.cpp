//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/argument.h"
#include "common/log/line.h"
#include "common/log.h"
#include "common/terminal.h"

#include <format>
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
                  State( std::span< O> options)
                     : hierarchy{ options}
                  {}

                  std::span< O> current() { return hierarchy.back();}

                  std::span< std::span< O>> parents() { return std::span( std::begin( hierarchy), std::prev( std::end( hierarchy)));}

                  friend State operator + ( State lhs, std::span< O> suboptions)
                  {
                     lhs.hierarchy.push_back( suboptions);
                     return lhs;
                  }
                  
                  std::vector< std::span< O>> hierarchy;
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
               while( ! arguments.empty())
               {
                  // first one has to be a key
                  auto key = arguments.front();
                  auto current = arguments.subspan( 1);

                  if( auto option = common::algorithm::find( state.current(), key))
                  {
                     // first we need to check the suboptions for the next "key"
                     if( auto found = find::next( option->suboptions(), current))
                     {
                        auto [ chosen, remains] = common::algorithm::divide_at( current, std::begin( found));

                        assign( *option, key, chosen);

                        // continue with the suboption
                        arguments = local::assign( state + option->suboptions(), remains, assign, assign_end);
                     }
                     // otherwise we check the siblings for the next "key"
                     else if( auto found = find::next( state.current(), current))
                     {
                        auto [ chosen, remains] = common::algorithm::divide_at( current, std::begin( found));

                        assign( *option, key, chosen);

                        // we've found the "next key" in this "level", we let the while loop continue
                        arguments = remains;
                     }
                     // otherwise we check parents for the next "key"
                     else if( auto found = find::parent( state.parents(), current))
                     {
                        auto [ chosen, remains] = common::algorithm::divide_at( current, std::begin( found));

                        assign( *option, key, chosen);

                        // we know that the key was found in some of the parents, let "parent assign" try to handle it
                        return remains;
                     }
                     // otherwise, we haven't found any "next key", we assign all arguments to option
                     else 
                     {
                        assign_end( *option, key, current, state);
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
               Trace trace{ "detail::assign"};

               std::vector< detail::option::Assigned> assigned;


               auto assign = [ &assigned]( auto& option, auto key, auto arguments)
               {
                  if( auto invocable = option.assign( key, arguments))
                     assigned.push_back( std::move( *invocable));
               };

               auto assign_end = [ assign]( auto& option, auto key, auto arguments, auto& state)
               {           
                  assign( option, key, arguments);
               };

               auto left = local::assign( traverse::State< Option>{ options}, arguments, assign, assign_end);

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
                     auto usage = option.usage();
                     detail::validate::cardinality( option.cardinality(), option.names().canonical(), usage);
                     
                     // if the option is assigned/used, we need to validate the suboptions. 
                     if( usage > 0)
                        validate::options( option.suboptions());
                  };

                  std::ranges::for_each( options, validate);
               }
               
            } // validate

            namespace help
            {
               constexpr platform::size::type indent_increment = 3;

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
                  if( cardinality.many()) 
                     return std::format( "{}..*", cardinality.min());
                  if( cardinality.fixed()) 
                     return string::to( cardinality.min());
                  return std::format( "{}..{}", cardinality.min(), cardinality.max());
               }

               std::string format_value_cardinality( const Cardinality& cardinality)
               {
                  if( cardinality == cardinality::zero())
                     return {};

                  if( cardinality.step() > 1)
                     return std::format( "{} {{{}}}", format_option_cardinality( cardinality), cardinality.step());
                  else
                     return format_option_cardinality( cardinality);
                     
               }

               void description( std::string_view description, platform::size::type indent)
               {
                  for( auto line : string::split( description, '\n'))
                     output( indent, "{}\n", line);
               }

               void print( std::span< const Option> options, platform::size::type indent);
               
               void print( const Option& option, platform::size::type indent = 0)
               {

                  if( option.names().active().empty() && ! option.names().deprecated().empty())
                     output( indent, "[deprecated] {} [{}]", string::join( option.names().deprecated(), ", "), format_option_cardinality( option.cardinality()));
                  else
                     output( indent, "{} [{}]", string::join( option.names().active(), ", "), format_option_cardinality( option.cardinality()));

                  auto information = option.complete( true, {});

                  if( ! information.empty())
                     output( "  ({}) [{}]", string::join( information, ", "), format_value_cardinality( option.value_cardinality()));

                  output( "\n");
                  description( option.description(), indent + 5);
                  output( "\n");

                  if( ! option.suboptions().empty())
                  {
                     output( indent + indent_increment, "SUB OPTIONS:\n\n");
                     help::print( option.suboptions(), indent + ( indent_increment * 2));
                  }
               }

               void print( std::span< const Option> options, platform::size::type indent = 0)
               {
                  for( auto& option : options)
                     print( option, indent);
               }

               auto print_all_formatter()
               {
                  auto format_name = []( const Option& option){ return option.names().canonical();};
                  auto format_arguments = []( const Option& option)
                  { 
                     return terminal::format::guard_empty( string::join( option.complete( true, {}), ','));
                  };

                  auto format_description = []( const Option& option)
                  {
                     if( auto found = algorithm::find( option.description(), '\n'))
                        return std::string{ std::begin( option.description()), std::begin( found)};
                     return terminal::format::guard_empty( option.description());
                  };

                  return terminal::format::formatter< const Option>::construct(
                     terminal::format::column( "name", format_name, terminal::color::no_color),
                     terminal::format::column( "value(s)", format_arguments, terminal::color::no_color),
                     terminal::format::column( "description", format_description, terminal::color::no_color)
                  );
               }


               void print_all( std::string_view description, std::span< const Option> options)
               {
                  output( "NAME\n");
                  output( indent_increment, "{}\n\n", process::path().filename().string());
                  output( "DESCRIPTION\n\n");
                  help::description( description, indent_increment);

                  output( "\nOPTIONS\n\n");

                  auto formatter = print_all_formatter();
                  formatter.print( std::cout, options);
               }

               void print( std::string_view description, std::span< const Option> options, range_type arguments)
               {
                  if( arguments.empty())
                     return print_all( description, options);

                  // use the assign algorithm to find the 'deepest' referenced options

                  auto discard_assign = []( auto& option, auto key, auto arguments)
                  {};

                  auto print_option = []( auto& option, auto key, auto arguments, auto& state)
                  {
                     print( option);
                  };

                  //! if the assign algorithm didn't consume all of the arguments, we didn't find anything.
                  if( ! local::assign( traverse::State< const Option>{ options}, arguments, discard_assign, print_option).empty())
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
            Trace trace{ "argument::parse"};
            log::line( verbose::log, "arguments: ", arguments);

            auto assigned = local::assign( options, arguments);

            local::validate::options( options);

            auto is_preemptive = []( auto& assigned){ return assigned.phase() == decltype( assigned.phase())::preemptive;};

            auto [ first, second] = common::algorithm::stable::partition( assigned, is_preemptive);

            std::ranges::for_each( first, std::mem_fn( &detail::option::Assigned::invoke));
            std::ranges::for_each( second, std::mem_fn( &detail::option::Assigned::invoke));
         }

         void complete( std::span< Option> options, range_type arguments)
         {
            // special case if no arguments provided -> first "level" of options are suggested
            if( arguments.empty())
            {
               for( auto& option : options)
                    std::cout << option.names().canonical() << '\n';
               return;
            }

            auto complete_assign = []( auto& option, auto key, auto arguments)
            {
               // fake "usage" the option 
               option.use();
            };

            auto complete_option = []( Option& option, auto key, range_type arguments, local::traverse::State< Option>& state)
            {
               // fake "usage" the option 
               option.use();

               auto cardinality = option.value_cardinality();

               if( std::ssize( arguments) < cardinality.max())
               {
                  // we got a few situations we can be in.
                  for( auto& suggestion : option.complete( false, arguments))
                     std::cout << suggestion << '\n';

                  // if we've not fullfil the min cardinality, only completion
                  // for the specific option is needed.
                  if( std::ssize( arguments) < cardinality.min())
                     return;

                  // if we're in mid cardinality step, only completion
                  // for the specific option is needed.
                  if( cardinality.mid_step( std::ssize( arguments)))
                     return;

                  // otherwise we traverse below...
               }

               auto print_suggestions = []( auto options)
               {
                  auto is_active = []( const auto& option){ return ! option.exhausted();};

                  for( auto& active : options | std::ranges::views::filter( is_active))
                    std::cout << active.names().canonical() << '\n';
               };

               // we need to add possible suboptions
               print_suggestions( option.suboptions());

               // traverse up in the option hierarchy and add all options that is not exhausted.
               for( auto options : std::views::reverse( state.hierarchy))
                  print_suggestions( options);
            };

            local::assign( local::traverse::State< Option>{ options}, arguments, complete_assign, complete_option);

         }

         void Policy::help( std::string_view description, std::span< const Option> options, range_type arguments)
         {
            local::help::print( description, options, arguments);
         }

         Option Policy::help_option( std::vector< std::string> names)
         {
            assert( ! names.empty());

            auto invoke = []( std::vector< std::string_view> arguments)
            {
               // no op. will never be invoked...
            };

            auto key = names.back();

            return Option{
               std::move( invoke),
               std::move( names),
               string::compose( R"(shows this help information
               
Use )", key , R"( <option> to see selected details on <option>
You can also use more precise help for deeply nested options
`)", key, R"( -a -b -c -d -e`
)")
            };

         }

         std::vector< std::string> Policy::help_names()
         {
            return { std::string{ reserved::name::help}};
         }
         

      } // detail

      
   } // argument
   
} // casual