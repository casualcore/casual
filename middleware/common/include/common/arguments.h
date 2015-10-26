//!
//! arguments.h
//!
//! Created on: Jan 5, 2013
//!     Author: Lazan
//!

#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_


#include <vector>
#include <memory>
#include <limits>
#include <algorithm>
#include <functional>
#include <sstream>
#include <type_traits>
#include <iostream>
#include <iomanip>


#include "common/process.h"
#include "common/algorithm.h"
#include "common/terminal.h"
#include "common/string.h"


namespace casual
{
   namespace common
   {
      namespace argument
      {
         namespace internal
         {
            template< std::size_t min, std::size_t max>
            struct basic_cardinality
            {
               static_assert( min <= max, "not a valid cardinality");
               enum
               {
                  min_value = min,
                  max_value = max
               };
            };

            struct value_cardinality
            {
               value_cardinality() = default;
               value_cardinality( std::size_t min, std::size_t max) : min( min), max( max) {}

               std::size_t min;
               std::size_t max;
            };

         }

         namespace cardinality
         {
            template< std::size_t min, std::size_t max>
            using Range = internal::basic_cardinality< min, max>;

            template< std::size_t size>
            using Fixed = Range< size, size>;

            template< std::size_t size>
            using Min = Range< size, std::numeric_limits< std::size_t>::max()>;

            template< std::size_t size>
            using Max = Range< 0, size>;

            using One = Fixed< 1>;

            using Zero = Max< 0>;

            using Any = Min< 0>;

            using OneMany = Min< 1>;

         }


         namespace option
         {
            struct Holder
            {

               template< typename T>
               Holder( T&& option) : m_option( std::make_shared< holder_type< T>>( std::forward< T>( option)))
               {

               }

               bool option( const std::string& option) const
               {
                  return m_option->option( option);
               }
               void assign( const std::string& option, std::vector< std::string>&& values) const
               {
                  m_option->assign( option, std::move( values));
               }
               bool consumed() const
               {
                  return m_option->consumed();
               }
               void dispatch() const
               {
                  m_option->dispatch();
               }

               bool valid() const
               {
                  return m_option->valid();
               }

               void information( std::ostream& out) const
               {
                  m_option->information( out);
               }

            private:
               class base_type
               {
               public:
                  virtual ~base_type() {}
                  virtual bool option( const std::string& option) const = 0;
                  virtual void assign( const std::string& option, std::vector< std::string>&& values) = 0;
                  virtual bool consumed() const = 0;
                  virtual void dispatch() const = 0;
                  virtual bool valid() const = 0;
                  virtual void information( std::ostream& out) const = 0;
               };

               template< typename T>
               struct holder_type : public base_type
               {
                  template< typename O>
                  holder_type( O&& option) : m_option_type{ std::forward< T>( option)} {}

               private:
                  bool option( const std::string& option) const override
                  {
                     return m_option_type.option( option);
                  }
                  void assign( const std::string& option, std::vector< std::string>&& values) override
                  {
                     m_option_type.assign( option, std::move( values));
                  }
                  bool consumed() const override
                  {
                     return m_option_type.consumed();
                  }
                  void dispatch() const override
                  {
                     m_option_type.dispatch();
                  }
                  bool valid() const override
                  {
                     return m_option_type.valid();
                  }

                  void information( std::ostream& out) const override
                  {
                     m_option_type.information( out);
                  }

                  T m_option_type;
               };

               std::shared_ptr< base_type> m_option;

            };

         } // option


         namespace internal
         {

            template< typename T>
            struct from_string
            {
               T operator () ( const std::string& value) const
               {
                  std::istringstream converter( value);
                  T result{};
                  converter >> result;
                  return result;
               }
            };

            template<>
            struct from_string< std::string>
            {
               const std::string& operator () ( const std::string& value) const
               {
                  return value;
               }
            };

            struct base_dispatch
            {
               virtual ~base_dispatch() = default;
               virtual internal::value_cardinality cardinality() const = 0;
               virtual void operator () ( const std::vector< std::string>& values) const = 0;

            };

            template< typename T, typename F>
            void call( F& caller, const std::vector< std::string>& values, cardinality::Zero)
            {
               caller();
            }

            template< typename T, typename F>
            void call( F& caller, const std::vector< std::string>& values, cardinality::One)
            {
               caller( from_string< T>()( values.at( 0)));
            }

            template< typename T, typename F, typename C>
            void call( F& caller, const std::vector< std::string>& values, C)
            {

               std::vector< T> converted;

               std::transform(
                  std::begin( values),
                  std::end( values),
                  std::back_inserter( converted),
                  from_string< T>());

               caller( converted);
            }


            template< typename C, typename F, typename T>
            struct dispatch : public base_dispatch
            {
               typedef C cardinality_type;
               typedef T argument_type;

               dispatch( F caller) : m_caller( caller) {}

               void operator () ( const std::vector< std::string>& values) const
               {
                  call< argument_type>( m_caller, values, cardinality_type());
               }

               internal::value_cardinality cardinality() const
               {
                  return { cardinality_type::min_value, cardinality_type::max_value};
               }

            private:
               F m_caller;
            };

            namespace value
            {
               template< typename V>
               struct Holder
               {
                  Holder( V& value) : m_value( &value) {}

                  template< typename T>
                  void operator () ( T&& value) const
                  {
                     (*m_value) = std::forward< T>( value);
                  }

                  V* m_value;
               };

               template<>
               struct Holder< bool>
               {
                  Holder( bool& value) : m_value( &value) {}

                  void operator () () const
                  {
                     (*m_value) = true;
                  }

                  bool* m_value;
               };
            } // value


            namespace deduce
            {
               template< typename T>
               struct helper
               {
                  typedef cardinality::One cardinality;
                  typedef typename std::decay< T>::type type;
               };

               template< typename T>
               struct helper< std::vector< T>>
               {
                  typedef cardinality::OneMany cardinality;
                  typedef typename std::decay< T>::type type;
               };

               template<>
               struct helper< void>
               {
                  typedef cardinality::Zero cardinality;
                  typedef void type;
               };

               template<>
               struct helper< bool>
               {
                  typedef cardinality::Zero cardinality;
                  typedef bool type;
               };


               template< typename O>
               cardinality::Zero cardinality( O&, void (O::*)(void)) { return cardinality::Zero();}

               cardinality::Zero cardinality( void (*)(void)) { return cardinality::Zero();}

               template< typename O, typename T>
               auto cardinality( O&, void (O::*)( T)) -> typename helper< typename std::decay< T>::type>::cardinality
               {
                  return typename helper< typename std::decay< T>::type>::cardinality();
               }

               template< typename T>
               auto cardinality( void (*)( T)) -> typename helper< typename std::decay< T>::type>::cardinality
               {
                  return typename helper< typename std::decay< T>::type>::cardinality();
               }

               template< typename T>
               auto cardinality( T& value ) -> typename helper< typename std::decay< T>::type>::cardinality
               {
                  return typename helper< typename std::decay< T>::type>::cardinality();
               }
            } // deduce


            using namespace std::placeholders;

            template< typename C>
            struct maker
            {
               template< typename O, typename T>
               auto static make( O& object, void (O::*member)( T)) -> dispatch< C, decltype( std::bind( member, &object, _1)), typename deduce::helper< typename std::decay< T>::type >::type>
               {
                  typedef dispatch< C, decltype( std::bind( member, &object, _1)), typename deduce::helper< typename std::decay< T>::type>::type> result_type;
                  return result_type( std::bind( member, &object, _1));
               }

               template< typename T>
               auto static make( void (*function)( T)) -> dispatch< C, std::function<void( T)>, typename deduce::helper< typename std::decay< T>::type >::type>
               {
                  typedef dispatch< C, std::function<void( T)>, typename deduce::helper< typename std::decay< T>::type >::type> result_type;
                  return result_type( function);
               }

               template< typename T>
               auto static make( T& value) -> dispatch< C, value::Holder< T>, T>
               {
                  typedef dispatch< C, value::Holder< T>, T> result_type;
                  return result_type( value);
               }
            };


            template<>
            struct maker< cardinality::Zero>
            {

               template< typename O>
               auto static make( O& object, void (O::*member)(void)) -> dispatch< cardinality::Zero, decltype( std::bind( member, &object)), void>
               {
                  typedef dispatch< cardinality::Zero, decltype( std::bind( member, &object)), void> result_type;
                  return result_type( std::bind( member, &object));
               }

               auto static make( void (*function)(void)) -> dispatch< cardinality::Zero, std::function<void()>, void>
               {
                  typedef dispatch< cardinality::Zero, std::function<void()>, void> result_type;
                  return result_type( function);
               }

               auto static make( bool& value) -> dispatch< cardinality::Zero, value::Holder< bool>, bool>
               {
                  typedef dispatch< cardinality::Zero, value::Holder< bool>, bool> result_type;
                  return result_type( value);
               }

            };



            template< typename C, typename ...Args>
            auto make( C cardinality, Args&&... args) -> decltype( maker< C>::make( std::forward< Args>( args)...))
            {
               return maker< C>::make( std::forward< Args>( args)...);
            }



            struct Option
            {
               Option( const std::string& option) : option( option) {}

               template< typename T>
               bool operator () ( T&& value) const
               {
                  return value.option( option);
               }
            private:
               const std::string& option;
            };


            struct HasOption
            {
               bool operator() ( const std::string& option, const option::Holder& holder) const
               {
                  return holder.option( option);
               }
            };


            namespace format
            {
               std::ostream& cardinality( std::ostream& out, internal::value_cardinality cardinality)
               {
                  if( cardinality.min == cardinality.max)
                  {
                     if( cardinality.min == 1)
                     {
                        out << " <value>";
                     }
                     else if( cardinality.min > 1 && cardinality.min <= 3)
                     {
                        for( auto count = cardinality.min; count > 0; --count)
                        {
                           out << " <value>";
                        }
                     }
                     else if( cardinality.min > 3)
                     {
                        out << " <value>..{ " << cardinality.min << "}";
                     }
                  }
                  else
                  {
                     out << " <value> " << cardinality.min << "..";
                     if( cardinality.max == std::numeric_limits< std::size_t>::max())
                     {
                        out << '*';
                     }
                     else
                     {
                        out << cardinality.max;
                     }
                  }
                  return out;
               }


               std::ostream& description( std::ostream& out, const std::string& description, int indent = 6)
               {
                  for( auto& row : string::split( description, '\n'))
                  {
                     out << std::string( indent, ' ') << row << "\n";
                  }
                  return out;
               }

               template< typename T>
               void help( std::ostream& out, const T& directive)
               {

                  auto option_string = string::join( directive.options(), ", ");

                  if( option_string.empty())
                  {
                     option_string = "<empty>";
                  }

                  out << "   " << terminal::color::white << option_string;
                  format::cardinality( out, directive.cardinality()) << std::endl;

                  format::description( out, directive.description()) << std::endl;
               }

            } // format


         } // internal


         class Directive
         {
         public:

            template< typename C, typename... Args>
            Directive( C cardinality, const std::vector< std::string>& options, const std::string& description, Args&&... args)
               : m_options( options),
                 m_description( description),
                 m_dispatch( new decltype( internal::make( cardinality, std::forward< Args>( args)...))( internal::make( cardinality, std::forward< Args>( args)...)))
                  {}


            Directive( Directive&&) = default;


            bool option( const std::string& option) const
            {
               return ! range::find( range::make( m_options), option).empty();
            }

            void assign( const std::string& option, std::vector< std::string>&& values)
            {
               std::move( std::begin( values), std::end( values), std::back_inserter( m_values));
               //m_values = std::move( values);
               m_assigned = true;
            }

            bool consumed() const
            {
               return false; //m_assigned;
            }

            void dispatch() const
            {
               if( m_assigned)
               {
                  (*m_dispatch)( m_values);
               }
            }

            bool valid() const
            {
               return m_values.size() >= m_dispatch->cardinality().min &&
                     m_values.size() <= m_dispatch->cardinality().max;
            }


            void information( std::ostream& out) const
            {
               internal::format::help( out, *this);
            }


            const std::vector< std::string> options() const { return m_options;}
            const std::string& description() const { return m_description;}
            internal::value_cardinality cardinality() const { return m_dispatch->cardinality();}

         private:
            const std::vector< std::string> m_options;
            const std::string m_description;
            std::vector< std::string> m_values;
            bool m_assigned = false;

            std::unique_ptr< internal::base_dispatch> m_dispatch;

         };

         template< typename C, typename... Args>
         Directive directive( C cardinality, const std::vector< std::string>& options, const std::string& description, Args&&... args)
         {
            return Directive{ cardinality, options, description, std::forward< Args>( args)...};
         }

         template< typename... Args>
         Directive directive( const std::vector< std::string>& options, const std::string& description, Args&&... args)
         {
            return Directive{ internal::deduce::cardinality( std::forward< Args>( args)...), options, description, std::forward< Args>( args)...};
         }


         //template< typename C>
         class Group
         {
         public:

            //typedef C correlation_type;
            typedef std::vector< option::Holder> groups_type;


            template< typename T, typename... Args>
            void add( T&& directive, Args&&... args)
            {
               m_groups.emplace_back( std::forward< T>( directive));
               add( std::forward< Args>(args)...);
            }


            void information( std::ostream& out) const
            {
               range::for_each( m_groups, std::bind( &option::Holder::information, std::placeholders::_1, std::ref( out)));
            }

         protected:

            friend struct option::Holder;

            bool option( const std::string& option) const
            {
               return range::any_of( m_groups, internal::Option{ option});
            }

            void assign( const std::string& option, std::vector< std::string>&& values)
            {
               auto found = range::find_if( range::make( m_groups), internal::Option{ option});

               if( found)
               {
                  found.first->assign( option, std::move( values));
               }
            }

            bool consumed() const
            {
               return range::all_of( m_groups, std::mem_fn( &option::Holder::consumed));
            }

            void dispatch() const
            {
               range::for_each( m_groups, std::mem_fn( &option::Holder::dispatch));
            }


            bool valid() const
            {
               return range::all_of( m_groups, std::mem_fn( &option::Holder::valid));
            }



            void add() {}

            groups_type m_groups;
         };



         class Help
         {
         public:

            Help( const Group* group, std::string description, std::vector< std::string> options, std::ostream& out = std::cout)
               : m_group( group), m_description( std::move( description)),
                 m_options{ std::move( options)}, m_out( out)
            {

            }

            Help( const Group* group, std::string description) : Help( group, std::move( description), { "--help"})
            {

            }

            Help( const Group* group) : Help( group, "")
            {

            }

            bool option( const std::string& option) const
            {
               return ! range::find( range::make( m_options), option).empty();
            }

            void assign( const std::string& option, std::vector< std::string>&& values)
            {
               m_invoked = true;
            }

            bool consumed() const
            {
               return false; //m_assigned;
            }

            void dispatch() const
            {
               if( m_invoked)
               {
                  m_out << "NAME\n   ";
                  m_out << terminal::color::white << file::name::base( process::path());

                  m_out << "\n\nDESCRIPTION\n";

                  internal::format::description( m_out, m_description, 3) << "\nOPTIONS\n";

                  if( m_group)
                  {
                     m_group->information( m_out);
                  }

                  internal::format::help( m_out, *this);
               }
            }

            bool valid() const
            {
               return true;
            }


            void information( std::ostream& out) const
            {
               // no-op
            }

            const std::vector< std::string> options() const { return m_options;}
            std::string description() const { return "shows this help information";}
            internal::value_cardinality cardinality() const { return {0, 0};}

         private:

            const Group* m_group = nullptr;
            std::string m_description;
            std::vector< std::string> m_options;
            std::ostream& m_out;
            bool m_invoked = false;

         };

         namespace no
         {
            struct Help
            {

            };

            Help help() { return Help{};}
         } // no

      } // argument

      class Arguments : private argument::Group
      {
      public:



         Arguments()
         {
            argument::Group::add( argument::Help{ this});
         }

         Arguments( const std::string& description)
         {
            argument::Group::add( argument::Help{ this, description});
         }

         Arguments( const std::string& description, std::vector< std::string> help_option)
         {
            argument::Group::add( argument::Help{ this, description, std::move( help_option)});
         }


         Arguments( argument::no::Help)
         {
         }


         template< typename... Args>
         void add( Args&&... args)
         {
            argument::Group::add( std::forward< Args>( args)...);
         }


         //!
         //! @attention assumes first argument is process name.
         //!
         bool parse( int argc, char** argv)
         {
            if( argc > 0)
            {
               std::vector< std::string> arguments{ argv + 1, argv + argc};
               return parse( argv[ 0], arguments);
            }
            return false;
         }

         bool parse( const std::vector< std::string>& arguments)
         {
            return parse( process::path(), arguments);
         }


         bool parse( const std::string& process, const std::vector< std::string>& arguments)
         {
            process::path( process);


            //
            // start divide and conquer the arguments and try to find handlers for'em
            //

            auto argumentRange = range::make( arguments);

            while( argumentRange)
            {

               //
               // Try to find a handler for this argument
               //
               auto found = range::find_if( m_groups, argument::internal::Option{ *argumentRange});

               if( ! found)
               {
                  throw exception::invalid::Argument{ "invalid argument: " + *argumentRange};
               }

               //
               // pin the current found argument, and consume it so we can continue to
               // search
               //
               auto argument = *argumentRange;
               ++argumentRange;

               //
               // Find the end of values associated with this option
               //
               auto slice = range::divide_first( argumentRange, m_groups, argument::internal::HasOption());

               found->assign( argument, range::to_vector( std::get< 0>( slice)));

               if( ! found->valid())
               {
                  throw exception::invalid::Argument{ "invalid values for: " + argument};
               }

               argumentRange = std::get< 1>( slice);
            }

            dispatch();

            return true;
         }

         const std::string& processName() { return process::path();}
      };



   } // utility
} // casual



#endif /* ARGUMENTS_H_ */
