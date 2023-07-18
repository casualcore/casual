//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/argument.h"
#include "common/process.h"
#include "common/log.h"
#include "common/terminal.h"
#include "common/string/compose.h"
#include "common/algorithm/container.h"

#include "common/code/raise.h"


namespace casual
{
   namespace common
   {
      namespace argument
      {
         namespace local
         {
            namespace
            {
               namespace representation
               {
                  using range_type = range::type_t< std::vector< detail::Representation>>;

                  range_type find( range_type scope, const std::string& key)
                  {
                     auto found = algorithm::find_if( scope, [&]( auto& s){
                        return s.keys == key;
                     });

                     if( ! found)
                     {
                        for( auto& s : scope)
                        {
                           found = representation::find( range::make( s.nested), key);

                           if( found)
                              return range::zero_one( found);
                        }
                     }
                     return range::zero_one( found);
                  }

                  template< typename D>
                  void dispatch( range_type scope, D& dispatcher)
                  {
                     for( auto& s : scope)
                     {
                        if( dispatcher( s))
                        {
                           dispatcher.scope_start();
                           dispatch( range::make( s.nested), dispatcher);
                           dispatcher.scope_end();
                        }
                     }
                  }
               } // representation

               namespace help
               {
                  struct Dispatch 
                  {
                     Dispatch( std::ostream& out) : m_out( out) {}

                     bool operator () ( const detail::Representation& option)
                     {
                        if( option.keys.active().empty() && ! option.keys.deprecated().empty())
                           stream::write( indent(), "[deprecated] ", string::join( option.keys.deprecated(), ", "), " [", option.cardinality, ']');
                        else
                           stream::write( indent(), string::join( option.keys.active(), ", "), " [", option.cardinality, ']');

                        {
                           auto information = option.invocable.complete( {}, true);

                           if( ! information.empty())
                           {
                              m_out << "  (";
                              algorithm::for_each_interleave( information,
                                 [&]( auto& value){ m_out << value;},
                                 [&](){ m_out << ", ";});
                              
                              m_out << ") [" << option.invocable.cardinality() << ']';
                           }

                        }
                        m_out << '\n';
                        
                        
                        algorithm::for_each( string::split( option.description, '\n'), [&]( auto& row)
                        {
                           indent( 6)  << row << "\n";
                        });

                        if( ! option.keys.deprecated().empty() && ! option.keys.active().empty())
                        {
                           m_out << '\n';
                           stream::write( indent( 6), "deprecated: ", option.keys.deprecated(), "\n");
                        }


                        m_out << '\n';
                        
                        return ! option.nested.empty();
                     }

                     void scope_start() 
                     { 
                        indent( 2) << "SUB OPTIONS\n";
                        m_indentation += 4;
                     }
                     void scope_end() { m_indentation -= 4;}

                     std::ostream& indent( size_type offset = 0) { return m_out << std::string( m_indentation + offset, ' ');}


                     std::ostream& m_out;
                     size_type m_indentation = 2;
                  };

                  namespace dispatch
                  {

                     auto brief()
                     {
                        auto print_value = []( auto& option){
                           auto information = option.invocable.complete( {}, true);

                           switch( information.size())
                           {
                              case 0: return std::string{};
                              case 1: return information.front();
                              default: return string::compose( information);
                           }
                        };

                        auto print_option_cardinality = []( const auto& r) -> std::string
                        {
                           if( r.cardinality == cardinality::zero_one{}) return "?";
                           if( r.cardinality == cardinality::one_many{}) return "+";
                           if( r.cardinality == cardinality::any{}) return "*";
                           if( r.cardinality.fixed()) return string::to( r.cardinality.min());
                           return string::compose( r.cardinality);
                        };

                        auto print_value_cardinality = []( const auto& r) -> std::string
                        {
                           auto cardinality = r.invocable.cardinality();
                           if( cardinality == cardinality::zero{}) return {};
                           if( cardinality == cardinality::zero_one{}) return "?";
                           if( cardinality == cardinality::one_many{}) return "+";
                           if( cardinality == cardinality::any{}) return "*";
                           if( cardinality.fixed()) return string::to( cardinality.min());
                           return string::compose( cardinality);
                        };

                        auto print_description = []( const auto& r){
                           auto rows = string::split( r.description, '\n');
                              if( ! rows.empty())
                                 return rows.front();
                              return std::string{};
                        };

                        return terminal::format::formatter< detail::Representation>::construct(
                           terminal::format::column( "OPTIONS", []( const auto& r){ return string::join( r.keys.active(), ", ");}, terminal::color::yellow),
                           terminal::format::column( "c", print_option_cardinality, terminal::color::no_color),
                           terminal::format::column( "value", print_value, terminal::color::no_color),
                           terminal::format::column( "vc", print_value_cardinality, terminal::color::no_color, terminal::format::Align::right),
                           terminal::format::column( "description", print_description, terminal::color::no_color)
                        );
                     }
                  } // dispatch


                  void full( representation::range_type options, const std::string& description)
                  {
                     if( ! description.empty())
                     {
                        std::cout << "DESCRIPTION\n";

                        algorithm::for_each( string::split( description, '\n'), [&]( auto& row){
                           std::cout << std::string( 2, ' ')  << row << "\n";
                        });
                     }
                  
                     std::cout << "\n";

                     auto dispatch = local::help::dispatch::brief();
                     dispatch.print( std::cout, options);

                     std::cout << '\n';
                  }

                  void partial( range_type arguments, representation::range_type options)
                  {
                     local::help::Dispatch dispatch( std::cout);

                     for( auto& argument : arguments)
                        local::representation::dispatch( representation::find( options, argument), dispatch);
                  }

                  namespace global
                  {
                     const auto keys = option::keys( { reserved::name::help()}, {});
                  } // global
                        

                  detail::Representation representation()
                  { 
                     detail::Representation result( detail::invoke::create( []( const std::vector< std::string>&){}));
                     result.keys = help::global::keys;
                     result.description = "use --help <option> to see further details\n\nYou can also use <program> <opt-A> --help to get detailed help on <opt-A>";
                     result.cardinality = argument::cardinality::zero_one();
                     return result;
                  }

                  void invoke( range_type arguments, std::vector< detail::Representation> options, const std::string& description)
                  {
                     Trace trace{ "common::argument::local::help::invoke"};
                     log::line( log::debug, "options", options);

                     std::cout << std::setw( 0);

                     options.push_back( representation());

                     if( arguments.empty())
                        full( range::make( options), description);
                     else 
                        partial( arguments, range::make( options));
                  }

               } // help

               namespace completion
               {
                  using invoked_range = range::type_t< std::vector< detail::Invoked>>;

                  std::vector< std::string> keys() 
                  {
                     return { reserved::name::completion()};
                  }

                  bool value_completed( const detail::Invoked& value)
                  {
                     auto cardinality = value.invocable.cardinality();

                     return value.values.size() >= cardinality.max();
                  }

                  bool value_optional( const detail::Invoked& value)
                  {
                     auto cardinality = value.invocable.cardinality();

                     return value.values.size() >= cardinality.min();
                  }

                  struct Mapper
                  {
                     Mapper( const detail::Representation& representation)
                        : key{ representation.keys.canonical()}, cardinality{ representation.cardinality} {}
                     

                     std::string key;
                     Cardinality cardinality; 
                     size_type invoked = 0;

                     friend bool operator == ( const Mapper& lhs, const std::string& rhs) { return lhs.key == rhs;}

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( key);
                        CASUAL_SERIALIZE( cardinality);
                        CASUAL_SERIALIZE( invoked);
                     )
                  };

                  std::tuple< invoked_range, std::vector< Mapper>> map_invoked( invoked_range invoked, representation::range_type options)
                  {
                     Trace trace{ "common::argument::local::completion::map_invoked"};

                     // We'll try to remove all options that is 'completed', and only keep those that is 
                     // available for the user. An option is completed if its used at least as many times as the
                     // cardinality indicates

                     std::vector< Mapper> result;

                     auto find_or_add = [&result]( auto& representation) -> Mapper&
                     {
                        if( auto found = algorithm::find( result, representation.keys.canonical()))
                           return *found;

                        result.emplace_back( representation);
                        return range::back( result);
                     };

                     auto unused = invoked;

                     while( invoked)
                     {

                        if( auto found = algorithm::find( options, range::front( invoked).key))
                        {
                           log::line( verbose::log, "found: ", *found);
                           log::line( verbose::log, "invoked: ", *invoked);

                           auto& mapper = find_or_add( *found);
                           ++mapper.invoked;

                           // this invoked is consumed, we rotate it to the end.
                           invoked = std::get< 0>( algorithm::rotate( invoked, range::next( invoked)));
                           
                           if( ! found->nested.empty())
                           {
                              auto sub = map_invoked( invoked, range::make( found->nested));
                              algorithm::container::append( std::get< 1>( sub), result);
                              invoked = std::get< 0>( sub);
                           }

                           // this option is consumed, we rotate it
                           algorithm::rotate( options, found);
                           ++options;

                        }
                        else 
                        {
                           ++invoked;
                        }
                     }

                     algorithm::container::append( options, result);
                     return std::tuple< invoked_range, std::vector< Mapper>>{ range::make( std::begin( unused), std::end( invoked)), std::move( result)};
                  }

                  void parse_options( invoked_range invoked, std::vector< detail::Representation>&& options)
                  {
                     Trace trace{ "common::argument::local::completion::parse_options"};

                     auto mapped = std::get< 1>( map_invoked( invoked, range::make( options)));
                     log::line( verbose::log, "mapped: ", mapped);

                     auto unconsumed = algorithm::remove_if( mapped, []( auto& map)
                     {
                        return  map.invoked >= map.cardinality.max();
                     });

                     log::line( verbose::log, "unconsumed: ", unconsumed);

                     for( auto& available : unconsumed)
                        std::cout << available.key << '\n';
                  }

                  void print_suggestion( const detail::Invoked& value)
                  {  
                     for( auto suggestion : value.invocable.complete( value.values, false))
                        std::cout << suggestion << '\n';
                  }



                  void invoke( range_type arguments, detail::Holder&& holder)
                  {
                     Trace trace{ "common::argument::local::completion::invoke"};

                     log::line( verbose::log, "arguments: ", arguments);
                     
                     std::vector< detail::Invoked> invoked;

                     detail::traverse( holder, arguments, [&invoked]( auto& option, auto& key, range_type arguments)
                     {
                        invoked.push_back( option.completion( key, arguments));
                     });

                     log::line( verbose::log, "invoked: ", invoked);

                     // 
                     // We've got three states we could be in.
                     // 
                     // 1) Last option needs more values, hence thats the only 
                     //   thing we'll suggest to the user
                     //
                     // 2) Last option can take more values, but it's optional,
                     //   hence we suggest values and other options.
                     //
                     // 3) Last option (or none) can't take any more values
                     //   We only suggest other options
                     //

                     if( ! invoked.empty() && ! value_completed( range::back( invoked)))
                     {
                        // 1
                        print_suggestion( range::back( invoked));

                        // 2
                        if( value_optional( range::back( invoked)))
                           parse_options( range::make( invoked), holder.representation());
                     }
                     else 
                     {
                        // 3
                        parse_options( range::make( invoked), holder.representation());
                     }
                  }
               } // completion

            } // <unnamed>
         } // local

         namespace exception
         {  
            void correlation( const std::string& key)
            {
               code::raise::error( code::casual::invalid_argument, "failed to correlate option ", key);
            }

         } // exception


         namespace detail
         {
            namespace key
            {
               std::ostream& operator << ( std::ostream& out, const Names& value)
               {
                  return stream::write( out, "{ active: ", value.m_active,
                     ", deprecated: ", value.m_deprecated,
                     '}');
               }
              
            } // key

            std::ostream& operator << ( std::ostream& out, const Representation& value)
            {
               return stream::write( out, "{ keys: ", value.keys, 
                  ", cardinality.option: ", value.cardinality, 
                  ", cardinality.values: ", value.invocable.cardinality(), 
                  ", nested: ", value.nested, 
                  '}');
            }
            
            std::ostream& operator << ( std::ostream& out, const Invoked& value)
            {
               return stream::write( out, "{ key: ", value.key, 
                  ", parent: ", value.parent, 
                  ", values: ", value.values, 
                  ", cardinality: ", value.invocable.cardinality(),
                  '}');
            }

            std::ostream& operator << ( std::ostream& out, const basic_cardinality& value)
            {
               return out << "{ cardinality: " << value.m_cardinality
                  << ", assigned: " << value.m_assigned
                  << '}';
            }

            namespace validate
            {
               void cardinality( const std::string& key, const Cardinality& cardinality, size_type value)
               {
                  if( ! cardinality.valid( value))
                     code::raise::error( code::casual::invalid_argument, "cardinality not satisfied for option: ", key);
               }

               namespace value
               {
                  void cardinality( const std::string& key, const Cardinality& cardinality, range_type values)
                  {
                     if( cardinality.valid( values.size()))
                        return;

                     code::raise::error( code::casual::invalid_argument, 
                        "cardinality not satisfied for values to option: ", key, 
                        " - ", cardinality, ", values: ", values);
                  }
               } // value

            } // validate

         } // detail

         namespace policy
         {
            Default::Default( std::string description) : description( std::move( description)) {}

            bool Default::overrides( range_type arguments, common::unique_function< detail::Holder()> callback)
            {
               // help
               if( auto found = algorithm::find_first_of( arguments, local::help::global::keys.active()))
               {
                  algorithm::rotate( arguments, found);
                  local::help::invoke( ++arguments, callback().representation(), description);
                  return true;
               }
                  
               // bash-completion                  
               if( auto found = algorithm::find_first_of( arguments, local::completion::keys()))
               {
                  algorithm::rotate( arguments, found);
                  local::completion::invoke( ++arguments, callback());
                  return true;
               }
               return false;
            }
         } // policy
         
      } // argument
   } // common
} // casual
