//!
//! arguments.h
//!
//! Created on: Jan 5, 2013
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_ARGUMENTS_H_
#define CASUAL_COMMON_ARGUMENTS_H_


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

         class Group;

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

         namespace visitor
         {
            struct Base;

         } // visitor

         namespace option
         {
            struct Holder
            {

               template< typename T>
               Holder( T option) : m_option( std::make_shared< holder_type< T>>( std::move( option)))
               {

               }

               bool option( const std::string& option) const;
               void assign( const std::string& option, std::vector< std::string>&& values) const;
               bool consumed() const;
               void dispatch() const;
               bool valid() const;

               const std::vector< std::string>& options() const;
               const std::string& description() const;
               internal::value_cardinality cardinality() const;

               void visit( visitor::Base& visitor) const;

            private:



               class base_type
               {
               public:
                  virtual ~base_type() = default;
                  virtual bool option( const std::string& option) const = 0;
                  virtual void assign( const std::string& option, std::vector< std::string>&& values) const = 0;
                  virtual bool consumed() const = 0;
                  virtual void dispatch() const = 0;
                  virtual bool valid() const = 0;

                  virtual const std::vector< std::string>& options() const = 0;
                  virtual const std::string& description() const = 0;
                  virtual internal::value_cardinality cardinality() const = 0;

                  virtual void visit( visitor::Base& visitor) const = 0;
               };


               template< typename T>
               struct holder_type : public base_type
               {
                  holder_type( T option) : m_option_type{ std::move( option)} {}

               private:
                  bool option( const std::string& option) const override
                  {
                     return m_option_type.option( option);
                  }
                  void assign( const std::string& option, std::vector< std::string>&& values) const override
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
                  const std::vector< std::string>& options() const override
                  {
                     return m_option_type.options();
                  }
                  const std::string& description() const override
                  {
                     return m_option_type.description();
                  }
                  internal::value_cardinality cardinality() const override
                  {
                     return m_option_type.cardinality();
                  }
                  void visit( visitor::Base& visitor) const override
                  {
                     m_option_type.visit( visitor);
                  }

                  mutable T m_option_type;
               };

               using pimpl_type = std::shared_ptr< const base_type>;

               pimpl_type m_option;
            };
         } // option


         namespace internal
         {

            struct from_string
            {
               using values_type = const std::vector< std::string>&;

               from_string( values_type values) : m_values{ values} {}

               template< typename T>
               operator T() const
               {
                  assert( ! m_values.empty());
                  return convert< T>{}( m_values.at( 0));
               }

               template< typename T>
               operator std::vector< T>() const
               {
                  std::vector< T> result;

                  std::transform(
                     std::begin( m_values),
                     std::end( m_values),
                     std::back_inserter( result),
                     convert< T>{});

                  return result;
               }

               operator const std::string&() const
               {
                  assert( ! m_values.empty());
                  return m_values.at( 0);
               }

               operator values_type() const
               {
                  return m_values;
               }

            private:

               template< typename T>
               struct convert
               {
                  T operator () ( const std::string& value) const
                  {
                     std::istringstream converter( value);
                     T result{};
                     converter >> result;
                     return result;
                  }
               };

               values_type m_values;
            };


            template< typename F>
            void call( F& caller, const std::vector< std::string>& values, cardinality::Zero)
            {
               caller();
            }

            template< typename F, typename C>
            void call( F& caller, const std::vector< std::string>& values, C)
            {
               caller( from_string{ values});
            }

            namespace deduce
            {
               template< typename T>
               struct helper
               {
                  using cardinality = argument::cardinality::One;
                  using type = typename std::decay< T>::type;
               };

               template< typename T>
               struct helper< std::vector< T>>
               {
                  using cardinality = argument::cardinality::OneMany;
                  using type = typename std::decay< T>::type;
               };

               template<>
               struct helper< void>
               {
                  using cardinality = argument::cardinality::Zero;
                  using type = void;
               };

               template<>
               struct helper< bool>
               {
                  using cardinality = argument::cardinality::Zero;
                  using type = bool;
               };


               template< typename O>
               cardinality::Zero cardinality( O&, void (O::*)(void)) { return cardinality::Zero();}

               cardinality::Zero cardinality( void (*)(void)) { return cardinality::Zero();}

               cardinality::Zero cardinality( std::function<void()>) { return cardinality::Zero();}



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


            namespace value
            {
               template< typename V, typename T>
               void assign( V& variable, T&& value)
               {
                  variable = std::forward< T>( value);
               }

               template< typename V, typename T>
               void assign( std::vector< V>& variable, T&& value)
               {
                  range::copy( value, std::back_inserter( variable));
               }
            } // value


            using namespace std::placeholders;

            template< typename C>
            struct maker
            {

               template< typename O, typename T>
               auto static make( O& object, void (O::*member)( T)) -> std::function<void( T)>
               {
                  return std::bind( member, &object, _1);
               }

               template< typename T>
               auto static make( void (*function)( T)) -> std::function<void( T)>
               {
                  return std::function<void( T)>{ function};
               }

               //
               // For variables
               //
               template< typename T>
               auto static make( T& variable) -> std::function<void( T)>
               {
                  //
                  // We bind directly to the variable
                  //
                  return [&]( const T& values){ value::assign( variable, values);};
               }
            };


            template<>
            struct maker< cardinality::Zero>
            {
               using zero_result_type = std::function<void()>;

               template< typename O>
               static zero_result_type make( O& object, void (O::*member)(void))
               {
                  return zero_result_type( std::bind( member, &object));
               }

               static zero_result_type make( void (*function)(void))
               {
                  return zero_result_type( function);
               }

               static zero_result_type make( std::function<void()> function)
               {
                  return zero_result_type( std::move( function));
               }

               //
               // For variable bool
               //
               static zero_result_type make( bool& value)
               {
                  return zero_result_type( [&](){ value = true;});
               }
            };



            template< typename C, typename ...Args>
            auto make( C cardinality, Args&&... args) -> decltype( maker< C>::make( std::forward< Args>( args)...))
            {
               return maker< C>::make( std::forward< Args>( args)...);
            }


            struct base_directive
            {

               base_directive( std::vector< std::string> options, std::string description);

               const std::vector< std::string>& options() const;
               const std::string& description() const;

               bool option( const std::string& option) const;

               void visit( visitor::Base& visitor) const;

               virtual internal::value_cardinality cardinality() const = 0;

            private:

               std::vector< std::string> m_options;
               std::string m_description;
            };

            template< typename dispatch_type, typename cardinality_type>
            class basic_directive : public internal::base_directive
            {
            public:

               basic_directive( std::vector< std::string> options, std::string description, dispatch_type dispatch)
                  : internal::base_directive( std::move( options), std::move( description)),
                    m_dispatch{ std::move( dispatch)}
                     {}


               internal::value_cardinality cardinality() const override { return { cardinality_type::min_value, cardinality_type::max_value};}

            private:

               friend struct option::Holder;


               void assign( const std::string& option, std::vector< std::string>&& values)
               {
                  std::move( std::begin( values), std::end( values), std::back_inserter( m_values));
                  m_assigned = true;
               }

               bool consumed() const { return false;}
               void dispatch() const
               {
                  if( m_assigned)
                     internal::call( m_dispatch, m_values, cardinality_type{});
               }
               bool valid() const
               {
                  return m_values.size() >= cardinality_type::min_value &&
                        m_values.size() <= cardinality_type::max_value;
               }

               std::vector< std::string> m_values;
               bool m_assigned = false;

               dispatch_type m_dispatch;

            };

         } // internal

         namespace visitor
         {
            struct Base
            {
               virtual ~Base();

               void visit( const internal::base_directive& option);
               void visit( const Group& group);

            private:
               virtual void do_visit( const internal::base_directive&) = 0;
               virtual void do_visit( const Group&) = 0;
            };

         } // visitor


         class Group
         {
         public:

            using options_type = std::vector< argument::option::Holder>;

            Group( options_type options);


            void visit( visitor::Base& visitor) const;

            void operator() () { dispatch();}

         protected:

            friend struct option::Holder;

            bool option( const std::string& option) const;
            void assign( const std::string& option, std::vector< std::string>&& values);
            bool consumed() const;
            void dispatch() const;
            bool valid() const;

            options_type m_options;
         };


         template< typename C, typename... Args>
         option::Holder directive( C cardinality, std::vector< std::string> options, std::string description, Args&&... args)
         {
            auto caller = internal::make( cardinality, std::forward< Args>( args)...);
            return internal::basic_directive< decltype( caller), C>{ std::move( options), std::move( description), std::move( caller)};
         }

         template< typename... Args>
         option::Holder directive( std::vector< std::string> options, std::string description, Args&&... args)
         {
            auto cardinality = internal::deduce::cardinality( std::forward< Args>( args)...);
            return directive( cardinality, std::move( options), std::move( description), std::forward< Args>( args)...);
         }

         /*
         template< typename... Args>
         option::Holder directive( std::vector< std::string> options, std::string description, Group group, Args&&... args)
         {
            return directive( cardinality::Zero{}, std::move( options), std::move( description), std::forward< Args>( args)...);
         }
         */


         namespace no
         {
            struct Help
            {
               void operator () () {};
            };

            Help help() { return Help{};}
         } // no

      } // argument

      class Arguments : private argument::Group
      {
      public:

         using options_type = std::vector< argument::option::Holder>;

         Arguments( options_type&& options);
         Arguments( std::string description, options_type options);
         Arguments( std::string description, std::vector< std::string> help_option, options_type options);
         Arguments( argument::no::Help, options_type options);



         //!
         //! @attention assumes first argument is process name.
         //!
         void parse( int argc, char** argv);

         void parse( const std::vector< std::string>& arguments);


         void parse( const std::string& process, const std::vector< std::string>& arguments);

         //!
         //! @return process name
         //!
         const std::string& process() const;


      };

   } // common
} // casual



#endif /* ARGUMENTS_H_ */
