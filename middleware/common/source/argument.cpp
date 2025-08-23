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
            namespace hierarchy
            {
               template< typename O>
               struct State
               {
                  State( std::span< O> options)
                     : m_hierarchy{ options}
                  {}

                  std::span< O> current() { return m_hierarchy.back();}
                  std::span< std::span< O>> parents() { return std::span( std::begin( m_hierarchy), std::prev( std::end( m_hierarchy)));}
                  std::span< std::span< O>> all() { return m_hierarchy;}

                  friend State operator + ( State lhs, std::span< O> suboptions)
                  {
                     lhs.m_hierarchy.push_back( suboptions);
                     return lhs;
                  }

               private:
                  std::vector< std::span< O>> m_hierarchy;
               };
            } // hierarchy

            namespace find
            {
               auto next( range_type arguments, std::span< const Option> options)
               {
                  return common::algorithm::find_if( arguments, [ &options]( auto argument)
                  {
                     return common::algorithm::contains( options, argument);
                  });
               };

               auto parent( range_type arguments, auto parents)
               {
                  for( auto options : std::views::reverse( parents))
                  {
                     if( auto found = next( arguments, options))
                        return found;
                  }
                  return decltype( next( arguments, parents.front())){};
               };
               
            } // find

            //! Consumes all immediate/direct flags for suboptions to `option`
            //!  --a -immediate-flag <arg-to-a> <arg-to-a> 
            //!       consumes -immediate-flag and returns [ <arg-to-a>, <arg-to-a>] to 
            //!       be assign/consumed to `--a` 
            //! @returns arguments that are not consumed from `arguments`.
            template< typename O>
            range_type consume_immediate_flags( range_type arguments, O& option, auto callback)
            {
               // if the current option takes no argument, we don't need to hustle with immediate flags.
               // -> all suboptions are immediate, if any...
               if( option.value_cardinality() == cardinality::zero())
                  return arguments;

               while( ! std::empty( arguments))
               {
                  auto key = arguments.front();
                  auto current = arguments.subspan( 1);

                  // if we've got no more arguments, we don't need special treatment for immediate flags
                  //  -> they can be treated as regular options, if any.
                  if( std::empty( current))
                     return arguments;
                     
                  if( auto found = algorithm::find( option.suboptions().options, key); found && found->pure_flag())
                  {
                     // consume the immediate flag, and continue to search for more.
                     callback( *found, key, range_type{});
                     arguments = current;
                  }
                  else
                     return arguments;
               }
               return arguments;
            }


            inline range_type traverse( auto state, range_type arguments, auto callback, auto callback_end, std::optional< Cardinality> sibling_cardinality = {})
            {
               while( ! std::empty( arguments))
               {
                  // first one has to be a key
                  auto key = arguments.front();

                  if( auto option = common::algorithm::find( state.current(), key))
                  {
                     auto current = consume_immediate_flags( arguments.subspan( 1), *option, callback);

                     // first we need to check the suboptions for the next "key"
                     if( auto found = find::next( current, option->suboptions().options))
                     {
                        auto [ chosen, remains] = common::algorithm::divide_at( current, std::begin( found));

                        callback( *option, key, chosen);

                        // continue with the suboption
                        arguments = local::traverse( state + option->suboptions().options, remains, callback, callback_end, option->suboptions().cardinality);
                     }
                     // otherwise we check the siblings for the next "key"
                     else if( auto found = find::next( current, state.current()))
                     {
                        auto [ chosen, remains] = common::algorithm::divide_at( current, std::begin( found));

                        callback( *option, key, chosen);

                        // we've found the "next key" in this "level", we let the while loop continue
                        arguments = remains;
                     }
                     // otherwise we check parents for the next "key"
                     else if( auto found = find::parent( current, state.parents()))
                     {
                        auto [ chosen, remains] = common::algorithm::divide_at( current, std::begin( found));

                        callback( *option, key, chosen);

                        // we know that the key was found in some of the parents, let "parent assign" try to handle it
                        return remains;
                     }
                     // otherwise, we haven't found any "next key", we assign all arguments to option
                     else 
                     {
                        callback_end( *option, key, current, state, sibling_cardinality);
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

               auto assign_end = [ assign]( auto& option, auto key, auto arguments, auto& state, auto&& sibling_cardinality)
               {           
                  assign( option, key, arguments);
               };

               auto left = local::traverse( hierarchy::State< Option>{ options}, arguments, assign, assign_end);

               if( ! left.empty())
                  common::code::raise::error( common::code::casual::invalid_argument, "failed to find option: ", left.front());

               return assigned;
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

            platform::size::type unique_used_options( std::span< const Option> options)
            {
               return algorithm::accumulate( options, 0l, []( platform::size::type count, const auto& option)
               {
                  if( option.usage() > 0)
                     return count + 1;
                  return count;
               });
            }


            namespace filter
            {
               auto is_used = []( const auto& option)
               {
                  return option.usage() > 0;
               };

               auto not_exhausted = []( const auto& option)
               {
                  return ! option.exhausted();
               };

               auto satisfied = []( const auto& option)
               {
                  return option.satisfied_min();
               };

            } // filter

            namespace validate
            {
               inline void options( std::span< const Option> options)
               {
                  constexpr static auto validate_suboptions = []( const Option& option)
                  {
                     auto suboption_names = []( std::span< const Option> suboptions)
                     {
                        return algorithm::transform( suboptions, []( const Option& suboption)
                        {
                           return suboption.names().canonical();
                        });
                     };

                     auto used_suboptions = unique_used_options( option.suboptions().options);

                     auto& options = option.suboptions().options;
                     auto cardinality = option.suboptions().cardinality;

                     if( ! cardinality.valid( used_suboptions))
                        common::code::raise::error( common::code::casual::invalid_argument, "cardinality not satisfied for suboptions: ", 
                           suboption_names( options), " [", format_option_cardinality( cardinality), ']');

                     // recursive validation
                     validate::options( options);
                  };

                  auto validate = []( const auto& option)
                  {
                     auto usage = option.usage();
                     detail::validate::cardinality( option.cardinality(), option.names().canonical(), usage);
                     
                     // if the option is assigned/used, we need to validate the suboptions. 
                     if( usage > 0)
                        validate_suboptions( option);
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

               void description( std::string_view description, platform::size::type indent)
               {
                  for( auto line : string::split( description, '\n'))
                     output( indent, "{}\n", line);
               }

               void print( std::span< const Option> options, platform::size::type indent, std::optional< int> depth);
               
               void print( const Option& option, platform::size::type indent, std::optional< int> depth)
               {
                  if( depth)
                     *depth -= 1;

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

                  if( ! option.suboptions().options.empty())
                  {
                     if( depth && *depth <= 0)
                        return;

                     auto cardinality = option.suboptions().cardinality;
                     
                     if( cardinality != argument::cardinality::any())
                        output( indent + indent_increment, "SUB OPTIONS [{}]:\n\n", format_option_cardinality( option.suboptions().cardinality));
                     else
                        output( indent + indent_increment, "SUB OPTIONS:\n\n");

                     help::print( option.suboptions().options, indent + ( indent_increment * 2), depth);
                  }
               }

               void print( std::span< const Option> options, platform::size::type indent, std::optional< int> depth)
               {
                  for( auto& option : options)
                     print( option, indent, depth);
               }


               void print_all( std::string_view description, std::span< const Option> options)
               {
                  output( "NAME\n");
                  output( indent_increment, "{}\n\n", process::path().filename().string());
                  output( "DESCRIPTION\n\n");
                  help::description( description, indent_increment);

                  output( "\nOPTIONS\n\n");

                  print( options, indent_increment, 1);
               }

               void print( std::string_view description, std::span< const Option> options, range_type arguments)
               {
                  if( arguments.empty())
                     return print_all( description, options);

                  // use the assign algorithm to find the 'deepest' referenced options

                  auto discard_assign = []( auto& option, auto key, auto arguments)
                  {};

                  auto print_option = []( auto& option, auto key, auto arguments, auto& state, auto&& sibling_cardinality)
                  {
                     print( option, 0, std::nullopt);
                  };

                  //! if the assign algorithm didn't consume all of the arguments, we didn't find anything.
                  if( ! local::traverse( hierarchy::State< const Option>{ options}, arguments, discard_assign, print_option).empty())
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
            log::debug( "arguments: ", arguments);

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

            // this will only be called once, when the _traverse_ has
            // exhausted all options down the hierarchy
            auto complete_option = []( Option& option, auto key, range_type arguments, local::hierarchy::State< Option>& state, std::optional< Cardinality> sibling_cardinality)
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
                  for( auto& active : options | std::ranges::views::filter( local::filter::not_exhausted))
                    std::cout << active.names().canonical() << '\n';
               };

               // take care of suboptions
               if( ! option.suboptions().options.empty())
               {
                  // we need to add possible suboptions
                  print_suggestions( option.suboptions().options);

                  // if we have a min cardinality > 0, we only suggest suboptions
                  if( option.suboptions().cardinality.min() > 0)
                     return;
               }

               // if current option has sibling cardinality, we use this to deduce if 
               // we can restrict suggestions.
               if( sibling_cardinality)
               {
                  auto siblings = state.current();

                  auto used_siblings_count = local::unique_used_options( siblings);

                  if( used_siblings_count < sibling_cardinality->min())
                  {
                     // we only suggest siblings
                     print_suggestions( siblings);
                     return;
                  }
                  else if( used_siblings_count >= sibling_cardinality->max())
                  {
                     // sibling cardinality is fulfilled, we still need to check if
                     // the actual option cardinality still allow for sibling suggestions.
                     auto used_siblings = siblings | std::ranges::views::filter( local::filter::is_used);

                     // suggest not exhausted siblings.
                     print_suggestions( used_siblings);

                     // if all used siblings are satisfied, we suggest parents
                     if( std::ranges::all_of( used_siblings, local::filter::satisfied))
                     {
                        for( auto options : std::views::reverse( state.parents()))
                           print_suggestions( options);
                     }

                     return;
                  }
                  // otherwise we suggest all options below
               }
       
               
               // traverse up in the option hierarchy and add all options that is not exhausted.
               for( auto options : std::views::reverse( state.all()))
                  print_suggestions( options);
            };

            local::traverse( local::hierarchy::State< Option>{ options}, arguments, complete_assign, complete_option);

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