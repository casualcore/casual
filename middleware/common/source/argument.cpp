//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/argument.h"
#include "common/file.h"
#include "common/process.h"
#include "common/log.h"
#include "common/terminal.h"


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
                        return ! algorithm::find( s.keys, key).empty();
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
                        indent() << string::join( option.keys, ", ") << "  " << option.cardinality;

                        {
                           auto information = option.invocable.complete( {}, true);

                           switch( information.size())
                           {
                              case 0: break;
                              case 1: m_out << "  (" << information.front() << ") [" << option.invocable.cardinality() << ']'; break;
                              default: m_out << "  (" << range::make( information) << ") ["  << option.invocable.cardinality() << ']'; break;
                           }

                        }
                        m_out << '\n';
                        
                        algorithm::for_each( string::split( option.description, '\n'), [&]( auto& row){
                           this->indent( 6)  << row << "\n";
                        });

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
                              default: return string::compose( range::make( information));
                           }
                        };

                        auto print_option_cardinality = []( const auto& r){
                           if( r.cardinality == cardinality::zero_one{}) return std::string{ "?"};
                           if( r.cardinality == cardinality::one_many{}) return std::string{ "+"};
                           if( r.cardinality == cardinality::any{}) return std::string{ "*"};
                           if( r.cardinality.fixed()) return to_string( r.cardinality.min());
                           return string::compose( r.cardinality);
                        };

                        auto print_value_cardinality = []( const auto& r){
                           auto cardinality = r.invocable.cardinality();
                           if( cardinality == cardinality::zero{}) return std::string{};
                           if( cardinality == cardinality::zero_one{}) return std::string{ "?"};
                           if( cardinality == cardinality::one_many{}) return std::string{ "+"};
                           if( cardinality == cardinality::any{}) return std::string{ "*"};
                           if( cardinality.fixed()) return to_string( cardinality.min());
                           return string::compose( cardinality);
                        };

                        auto print_description = []( const auto& r){
                           auto rows = string::split( r.description, '\n');
                              if( ! rows.empty())
                                 return rows.front();
                              return std::string{};
                        };

                        return terminal::format::formatter< detail::Representation>::construct(
                           terminal::format::column( "OPTIONS", []( const auto& r){ return string::join( r.keys, ", ");}, terminal::color::yellow),
                           terminal::format::column( "c", print_option_cardinality, terminal::color::no_color),
                           terminal::format::column( "value", print_value, terminal::color::no_color),
                           terminal::format::column( "vc", print_value_cardinality, terminal::color::no_color, terminal::format::Align::right),
                           terminal::format::column( "description", print_description, terminal::color::no_color)
                        );
                     }
                  } // dispatch


                  void full( representation::range_type options, const std::string& description)
                  {
                     std::cout << "NAME\n   ";
                     std::cout << file::name::base( process::path()) << '\n';

                     if( ! description.empty())
                     {
                        std::cout << "\nDESCRIPTION\n";

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
                     {
                        local::representation::dispatch( representation::find( options, argument), dispatch);
                     }

                  }

                  std::vector< std::string> keys() 
                  {
                     return { reserved::name::help()};
                  }

                  detail::Representation representation()
                  { 
                     detail::Representation result( detail::invoke::create( []( const std::vector< std::string>&){}));
                     result.keys = help::keys();
                     result.description = "use --help <option> to see further details\n\nYou can also use <program> <opt-A> --help to get detailed help on <opt-A>";
                     result.cardinality = argument::cardinality::zero_one();
                     return result;
                  }

                  void invoke( range_type arguments, std::vector< detail::Representation> options, const std::string& description)
                  {
                     std::cout << std::setw( 0);

                     options.push_back( representation());

                     if( arguments.empty())
                        full( range::make( options), description);
                     else 
                        partial( arguments, range::make( options));

                     throw exception::user::Help{ "built in help was invoked"};
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
                     Mapper( const detail::Representation& representation, const detail::Invoked& invoked)
                        : option( representation), invoked( invoked) {}
                     
                     Mapper( const detail::Representation& representation)
                        : option( representation) {}

                     struct Option
                     {
                        Option( const detail::Representation& r)
                           : key( r.keys.back()), cardinality( r.cardinality) {}

                        std::string key;
                        Cardinality cardinality; 
                     } option;

                     struct Invoked 
                     {
                        Invoked( const detail::Invoked& invoked)
                           : invoked( invoked.invoked) {}
                        
                        Invoked() = default;

                        explicit operator bool () const { return invoked >= 0;}

                        size_type invoked = -1;
                     } invoked;

                  };

                  std::tuple< invoked_range, std::vector< Mapper>> map_invoked( invoked_range invoked, representation::range_type options)
                  {
                     Trace trace{ "common::argument::local::completion::map_invoked"};



                     //
                     // We'll try to remove all options that is 'completed', and only keep those that is 
                     // available for the user. An option is completed if its used at least as many times as the
                     // cardinality indicates
                     //

                     std::vector< Mapper> result;

                     auto unused = invoked;

                     while( invoked)
                     {
                        log::line( verbose::log, "invoked: ", invoked);
                        log::line( verbose::log, "options: ", options);

                        auto found = algorithm::find_if( options, [&]( auto& option){
                           return algorithm::find( option.keys, invoked.front().key);
                        });

                        if( found)
                        {
                           result.emplace_back( *found, *invoked);

                           // this invoked is consumed, we rotate it to the end.
                           invoked = std::get< 0>( algorithm::rotate( invoked, invoked + 1));
                           
                           if( ! found->nested.empty())
                           {
                              auto sub = map_invoked( invoked, range::make( found->nested));
                              algorithm::append( std::get< 1>( sub), result);
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
                     algorithm::copy( options, result);
                     return std::tuple< invoked_range, std::vector< Mapper>>{ range::make( std::begin( unused), std::end( invoked)), std::move( result)};
                  }

                  void parse_options( invoked_range invoked, std::vector< detail::Representation>&& options)
                  {
                     auto mapped = std::get< 1>( map_invoked( invoked, range::make( options)));

                     auto unconsumed = algorithm::remove_if( mapped, []( auto& map){
                        if( map.invoked && map.invoked.invoked >= map.option.cardinality.max())
                        {
                           return true;
                        }
                        return false;
                     });

                     for( auto& available : unconsumed)
                     {
                        std::cout << available.option.key << '\n';
                     }
                  }

                  void print_suggestion( const detail::Invoked& value)
                  {  
                     for( auto suggestion : value.invocable.complete( value.values, false))
                     {
                        std::cout << suggestion << '\n';
                     }
                  }



                  void invoke( range_type arguments, detail::Holder&& holder)
                  {
                     Trace trace{ "common::argument::local::completion::invoke"};

                     log::line( verbose::log, "arguments: ", arguments);
                     
                     std::vector< detail::Invoked> invoked;

                     detail::traverse( holder, arguments, [&invoked]( auto& option, auto& key, range_type arguments){
                        invoked.push_back( option.completion( key, arguments));
                     });

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

                     if( ! invoked.empty() && ! value_completed( invoked.back()))
                     {
                        // 1
                        print_suggestion( invoked.back());

                        // 2
                        if( value_optional( invoked.back()))
                           parse_options( range::make( invoked), holder.representation());
                     }
                     else 
                     {
                        // 3
                        parse_options( range::make( invoked), holder.representation());
                     }

                     throw exception::user::bash::Completion{ "user abort - bash-completion"};
                  }
               } // completion

            } // <unnamed>
         } // local

         namespace exception
         {  
            void correlation( const std::string& key)
            {
               throw invalid::Argument{ "failed to correlate option " + key};
            }

         } // exception


         namespace detail
         {
            std::ostream& operator << ( std::ostream& out, const Representation& value)
            {
               return out << "{ keys: " << range::make( value.keys)
                  << ", cardinality.option: " << value.cardinality
                  << ", cardinality.values: " << value.invocable.cardinality()
                  << ", nested: " << range::make( value.nested)
                  << '}';
            }
            
            std::ostream& operator << ( std::ostream& out, const Invoked& value)
            {
               return out << "{ key: " << value.key
                  << ", parent: " << value.parent
                  << ", values: " << range::make( value.values)
                  << ", cardinality: " << value.invocable.cardinality()
                  << '}';
            }

            namespace validate
            {
               void cardinality( const std::string& key, const Cardinality& cardinality, size_type value)
               {
                  if( ! cardinality.valid( value))
                     throw exception::invalid::Argument{ string::compose( "cardinality not satisfied for option: ", key)};
               }

               namespace value
               {
                  void cardinality( const std::string& key, const Cardinality& cardinality, range_type values)
                  {
                     if( ! cardinality.valid( values.size()))
                        throw exception::invalid::Argument{ 
                           string::compose( "cardinality not satisfied for values to option: ", key, 
                              " - ", cardinality, ", values: ", values)};
                  }
               } // value

            } // validate

         } // detail

         namespace policy
         {
            Default::Default( std::string description) : description( std::move( description)) {}

            void Default::overrides( range_type arguments, callback_type callback)
            {
               // help
               {
                  auto found = algorithm::find_first_of( arguments, local::help::keys());
                  if( found)
                  {
                     algorithm::rotate( arguments, found);
                     local::help::invoke( ++arguments, callback().representation(), description);
                  }
               }
                  
               // bash-completion
               {
                  auto found = algorithm::find_first_of( arguments, local::completion::keys());
                  if( found)
                  {
                     algorithm::rotate( arguments, found);
                     local::completion::invoke( ++arguments, callback());
                  }
               }
            }
         } // policy
         
      } // argument
   } // common
} // casual
