//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/argument/cardinality.h"
#include "common/argument/exception.h"

#include "common/algorithm.h"
#include "common/functional.h"

#include "common/string.h"
#include "common/compare.h"

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <iostream>


namespace casual 
{
   namespace common 
   {
      namespace argument
      {
         using range_type = range::type_t< std::vector< std::string>>;
         using size_type = platform::size::type;


         namespace reserved
         {
            namespace name
            {
               constexpr auto completion() { return "casual-bash-completion";}
               constexpr auto help() { return "--help";}

               namespace suggestions
               {
                  constexpr auto value() { return "<value>";}
               } // suggestions

            } // name
         } // reserved

         namespace exception
         {  
            void correlation( const std::string& key);
         } // exception


         namespace detail
         {

            namespace validate
            {
               void cardinality( const std::string& key, const Cardinality& cardinality, size_type value);

               namespace value
               {
                  void cardinality( const std::string& key, const Cardinality& cardinality, range_type values);
               } // value

            } // validate




            struct Invoke
            {
               template< typename C>
               Invoke( C callable) : m_callable( std::make_unique< model< C>>( std::move( callable))) {}

               Invoke( const Invoke& other) : m_callable( other.m_callable->copy()) {}
               Invoke( Invoke&&) = default;

               Invoke& operator = ( Invoke other)
               {
                  m_callable = std::move( other.m_callable);
                  return *this;
               }
               
               void invoke( const std::string& key, range_type values) { m_callable->invoke( key, values);}
               std::vector< std::string> complete( range_type values, bool help) const { return m_callable->complete( values, help);}
               Cardinality cardinality() const { return m_callable->cardinality();}

            private:
               struct base 
               {
                  virtual ~base() = default;
                  virtual void invoke( const std::string& key, range_type values) = 0;
                  virtual std::vector< std::string> complete( range_type values, bool help) const = 0;
                  virtual Cardinality cardinality() const = 0;
                  virtual std::unique_ptr< base> copy() const = 0;
               };

               template< typename C>
               struct model : base
               {
                  model( C callable) : m_callable( std::move( callable)) {}

                  void invoke( const std::string& key, range_type values) override { m_callable.invoke( key, values);}
                  std::vector< std::string> complete( range_type values, bool help) const override { return m_callable.complete( values, help);}

                  Cardinality cardinality() const override { return m_callable.cardinality();}

                  std::unique_ptr< base> copy() const override { return std::make_unique< model>( *this); }
               private:
                  C m_callable;
               };
               std::unique_ptr< base> m_callable;
            };


            //!
            //! A representation of an "option" and possible it's nested structure
            //! used when producing help and auto-complete information (and possible other stuff)
            //!
            struct Representation
            {  
               Representation( Invoke invocable) : invocable( std::move( invocable)) {}
               std::vector< std::string> keys;
               std::string description;
               
               Cardinality cardinality;

               Invoke invocable;

               std::vector< Representation> nested;

               friend std::ostream& operator << ( std::ostream& out, const Representation& value);
            };
            
            struct Invoked
            {
               Invoked( Invoke invocable) : invocable( std::move( invocable)) {}
               Invoke invocable;
               std::string key;
               std::string parent;
               range_type values;
               size_type invoked;
               
               friend std::ostream& operator << ( std::ostream& out, const Invoked& value);
            };


 
            struct Handler
            {
               template< typename H>
               Handler( H handler) : m_handler( std::make_unique< model< H>>( std::move( handler))) {}

               Handler( const Handler& other) : m_handler( other.m_handler->copy()) {}
               Handler( Handler&&) = default;

               Handler& operator = ( Handler other)
               {
                  m_handler = std::move( other.m_handler);
                  return *this;
               }


               //!
               //! Semantics
               //! * first `has` is used to find a key to an option
               //! * then to determine which values the option should be invoked with
               //!    `next` is used to find the next option. This to provide a mean for 
               //!     the _handler_ to have different semantics for the two.
               //! 
               //! @{
               inline bool has( const std::string& key) const { return m_handler->has( key);}
               inline bool next( const std::string& key) const { return m_handler->next( key);}
               //! @}

               inline void invoke( const std::string& key, range_type values) { m_handler->invoke( key, values);}
               
               inline detail::Invoked completion( const std::string& key, range_type values) { return m_handler->completion( key, values);}


               inline const std::vector< std::string>& keys() const { return m_handler->keys();} 
               inline const std::string& description() const { return m_handler->description();}
               
               inline Representation representation() const { return m_handler->representation();}

               inline void validate() const { return m_handler->validate();}


            private:
               struct base 
               {
                  virtual ~base() = default;
                  virtual bool has( const std::string& key) const = 0;
                  virtual bool next( const std::string& key) const = 0;
                  virtual void invoke( const std::string& key, range_type values) = 0;
                  virtual detail::Invoked completion( const std::string& key, range_type values) = 0;

                  virtual const std::vector< std::string>& keys() const  = 0;
                  virtual const std::string& description() const = 0;

                  virtual Representation representation() const = 0;

                  virtual void validate() const = 0;

                  virtual std::unique_ptr< base> copy() const = 0;
               };

               template< typename H>
               struct model : base
               {
                  model( H handler) : m_handler( std::move( handler)) {}

                  bool has( const std::string& key) const override { return m_handler.has( key);}
                  bool next( const std::string& key) const override { return m_handler.next( key);}
                  void invoke( const std::string& key, range_type values) override { m_handler.invoke( key, values);}
                  detail::Invoked completion( const std::string& key, range_type values) override { return m_handler.completion( key, values);}

                  inline const std::vector< std::string>& keys() const override { return m_handler.keys();} 
                  inline const std::string& description() const override { return m_handler.description();} 

                  inline Representation representation() const override { return m_handler.representation();}

                  
                  inline void validate() const override { return selective_validate( m_handler);}

                  std::unique_ptr< base> copy() const override { return std::make_unique< model>( *this); }
               private:
                  template< typename T>
                  using has_validate = decltype( std::declval< T&>().validate());

                  template< typename T>
                  static auto selective_validate( T& handler) -> 
                     std::enable_if_t< common::traits::detect::is_detected< has_validate, T>::value>
                  {
                     handler.validate();
                  }
                  template< typename T>
                  static auto selective_validate( T& handler) -> 
                     std::enable_if_t< ! common::traits::detect::is_detected< has_validate, T>::value>
                  {
                  }

                  H m_handler;
               };
               std::unique_ptr< base> m_handler;
            };



            namespace invoke
            {

               namespace implementation 
               {

                  template< typename T> 
                  constexpr auto tuple_cardinality( T&& tuple);

                  template< typename T>
                  range_type tuple_assign( range_type values, T&& tuple);

                  template< typename T, typename... Ts> 
                  constexpr auto cardinality( T&& t, Ts&&... ts);


                  // default for a single argument
                  template< typename T, typename Enable = void> 
                  struct value_traits
                  {
                     constexpr static auto cardinality() { return cardinality::one();};

                     static range_type assign( range_type values, T& value) 
                     { 
                         value = from_string< T>( *values);
                         return ++values;
                     }

                  };

                  template< typename T> 
                  struct value_traits< T, std::enable_if_t< 
                     traits::container::is_sequence< T>::value
                     && ! traits::container::is_string< T>::value>>
                  {
                     using value_type = std::decay_t< decltype( *std::begin( std::declval< T&>()))>;
                     using value_cardinality = decltype( value_traits< value_type>::cardinality());

                     static_assert( value_cardinality::fixed(), "a container can't have value type of dynamic cardinality");
                     
                     constexpr static auto cardinality() 
                     {
                        return cardinality::any::steps< value_cardinality::min>();
                     };

                     static range_type assign( range_type values, T& result) 
                     { 
                        while( values)
                        {
                           value_type value;
                           values = value_traits< value_type>::assign( values, value);
                           result.push_back( std::move( value));
                        }
                        return values;
                     }
                  };


                  template< typename T> 
                  struct value_traits< T, std::enable_if_t< traits::is::tuple< T>::value>>
                  {
                     constexpr static auto cardinality() { return tuple_cardinality( T{});};

                     static range_type assign( range_type values, T& result) 
                     { 
                        return tuple_assign( values, result);
                     }
                  };

                  template< typename T> 
                  struct value_traits< T, std::enable_if_t< traits::is::optional_like< T>::value>>
                  {
                     
                     using value_type = std::decay_t< decltype( std::declval< T&>().value())>;
                     using nested_traits = value_traits< value_type>;
                     using value_cardinality = decltype( nested_traits::cardinality());

                     static_assert( value_cardinality::fixed(), "an optional like argument can't have value type of dynamic cardinality");

                     constexpr static auto cardinality() 
                     {
                        //using max = cardinality::max< value_cardinality::min>::steps< value_cardinality::min>();
                        //return max::steps< value_cardinality::min>();
                        return cardinality::range< 0, value_cardinality::min, value_cardinality::min>{};
                     };

                     static range_type assign( range_type values, T& result) 
                     { 
                        if( values)
                        {
                           value_type value;
                           values = value_traits< value_type>::assign( values, value);
                           result = std::move( value);
                        }
                        return values;
                     }
                  };

                  template< typename T> 
                  using value_traits_d = value_traits< std::decay_t< T>>;

                  constexpr auto cardinality() { return cardinality::zero();}

                  template< typename T> 
                  constexpr auto cardinality( T&& t) 
                  { 
                     return value_traits_d< T>::cardinality();
                  }

                  template< typename T, typename... Ts> 
                  constexpr auto cardinality( T&& t, Ts&&... ts) 
                  { 
                     static_assert( decltype( value_traits_d< T>::cardinality())::fixed(), "only the last parameter can have dynamic size");
                     return value_traits_d< T>::cardinality() + cardinality( std::forward< Ts>( ts)...);
                  }

                  template< typename T, std::size_t... I> 
                  constexpr auto tuple_cardinality_implementation( T&& tuple, std::index_sequence< I...>) 
                  {
                     return implementation::cardinality( std::get< I>( std::forward< T>( tuple))...);
                  }

                  template< typename T> 
                  constexpr auto tuple_cardinality( T&& tuple) 
                  { 
                     return tuple_cardinality_implementation( std::forward< T>( tuple), 
                        std::make_index_sequence< std::tuple_size< std::remove_reference_t< T>>::value>{});
                  }

                  inline range_type assign( range_type values) { return values;}

                  template< typename T, typename... Ts> 
                  range_type assign( range_type values, T& argument, Ts&&... arguments)
                  {
                     values = value_traits_d< T>::assign( values, argument);
                     return assign( values, std::forward< Ts>( arguments)...);
                  }

                  template< typename T, std::size_t... I>
                  range_type tuple_assign_implementation( range_type values, T&& tuple, std::index_sequence< I...>)
                  {
                     return implementation::assign( values, std::get< I>( std::forward< T>( tuple))...);
                  }

                  template< typename T>
                  range_type tuple_assign( range_type values, T&& tuple)
                  {
                     return tuple_assign_implementation( values, std::forward< T>( tuple), 
                        std::make_index_sequence< std::tuple_size< std::remove_reference_t< T>>::value>{});
                  }

                  template< typename Tuple> 
                  struct value_holder  
                  {
                     using tuple_type = Tuple;
                     value_holder() =  default;
                     value_holder( tuple_type arguments) : arguments( std::move( arguments)) {}
                     
                     auto cardinality() const 
                     {
                        return tuple_cardinality( arguments);
                     }

                     void assign( const std::string& key, range_type values) 
                     { 
                        validate::value::cardinality( key, cardinality(), values);
                        tuple_assign( values, arguments);
                     }
                     tuple_type arguments;
                  };
                  
                  template< typename C, typename Enable = void>
                  struct Invoke;

                  template< typename C>
                  struct Invoke< C, std::enable_if_t< traits::is::function< C>::value>> 
                     : value_holder< typename traits::function< C>::decayed>
                  {
                     using base_type =  value_holder< typename traits::function< C>::decayed>;
                     
                     Invoke( C callable) : m_callable( std::move( callable)) {}

                     void invoke( const std::string& key, range_type values) 
                     { 
                        base_type::assign( key, values); 
                        common::apply( m_callable, base_type::arguments);

                        // clear arguments for the possible next invocation
                        base_type::arguments = {};

                     }
                  private:
                     C m_callable;
                  };

                  template< typename T> 
                  struct Invoke< T, std::enable_if_t< traits::is::tuple< T>::value>> : value_holder< T>
                  {
                     using base_type = value_holder< T>;
                     using base_type::base_type;

                     void invoke( const std::string& key, range_type values) { base_type::assign( key, values); }
                  };

                  template< typename Call, typename Compl> 
                  struct Completion : Invoke< Call>
                  {
                     using base_type = Invoke< Call>;
                     Completion( Call callable, Compl completer) : base_type( std::move( callable)), m_completer( std::move( completer))
                     {}

                     std::vector< std::string> complete( range_type values, bool help) const
                     {  
                        return m_completer( values, help);
                     }

                  private:
                     Compl m_completer;
                  };

                  namespace completer
                  {
                     struct Default
                     {
                        std::vector< std::string> operator () ( range_type values, bool help) const 
                        {
                           if( help)
                           {
                              if( cardinality() == cardinality::zero{}) return {};
                              if( cardinality() == cardinality::zero_one{}) return { string::compose( '[', reserved::name::suggestions::value(), ']')};
                              if( cardinality() == cardinality::one{}) return { reserved::name::suggestions::value()};
                              return { string::compose( reserved::name::suggestions::value(), "...")};
                           }
                           return { reserved::name::suggestions::value()};
                        }
                     };
                  } // completer
               } // implementation 


               template< typename Call, typename Compl>
               Invoke create( Call&& callable, Compl&& completer)
               {
                  return Invoke{ implementation::Completion< std::decay_t< Call>, std::decay_t< Compl>>( 
                     std::forward< Call>( callable), std::forward< Compl>( completer))};                  
               }

               template< typename Call>
               Invoke create( Call&& callable)
               {
                  
                  return create( std::forward< Call>( callable), implementation::completer::Default());
               }

            } // invoke

            struct Holder
            {
               template< typename... Hs>
               static Holder construct( Hs&&... handlers)
               {
                  Holder result;
                  result.m_handlers.reserve( sizeof...( Hs));
                  initialize( result.m_handlers, std::forward< Hs>( handlers)...);
                  return result;
               } 
               

               inline bool has( const std::string& key) const
               {
                  return ! algorithm::find_if( m_handlers, [&key]( const auto& h){ return h.has( key);}).empty();
               }

               inline bool next( const std::string& key) const
               {
                  return ! algorithm::find_if( m_handlers, [&key]( const auto& h){ return h.next( key);}).empty();
               }

               detail::Invoked completion( const std::string& key, range_type values)
               {
                  return apply( [&key, values]( auto& found){ 
                     return found.completion( key, values);
                  }, key);
               }

               inline void invoke( const std::string& key, range_type values)
               {
                  apply( [&key, values]( auto& found){ 
                     found.invoke( key, values);
                  }, key);
               }
             
               std::vector< Representation> representation() const 
               {
                  return algorithm::transform( m_handlers, []( auto& h){
                     return h.representation();
                  });
               }

               inline void validate() const 
               { 
                  algorithm::for_each( m_handlers, std::mem_fn( &detail::Handler::validate));
               }

               template< typename O>
               inline friend Holder operator + ( Holder&& lhs,  O&& option)
               {
                  lhs.m_handlers.emplace_back( std::forward< O>( option));
                  return std::move( lhs);
               }

            private:
               Holder() = default;

               template< typename A>
               auto apply( A&& applier, const std::string& key) -> decltype( applier( std::declval< detail::Handler&>()))
               {
                  auto found = algorithm::find_if( m_handlers, [&key]( const auto& h){ return h.has( key);});

                  assert( found);

                  //
                  // make sure we prioritize the found option the next time, to give a more intuitive semantic
                  // if the same option is used in different "groups"
                  //
                  algorithm::rotate( m_handlers, found);

                  return applier( m_handlers.front());
               }

               template< typename H>
               static inline void assign( std::vector< Handler>& result, H&& handler) { result.emplace_back( std::forward< H>( handler));}

               static inline void assign( std::vector< Handler>& result, Holder&& holder)
               { 
                  algorithm::move( holder.m_handlers, result);
               }

               static inline void initialize( std::vector< Handler>& result) {}

               template< typename H, typename... Hs>
               static void initialize( std::vector< Handler>& result, H&& handler, Hs&&... handlers)
               {
                  Holder::assign( result, std::forward< H>( handler));
                  Holder::initialize( result, std::forward< Hs>( handlers)...);
               }

               std::vector< detail::Handler> m_handlers;
            };


            struct basic_keys
            {
               basic_keys( std::vector< std::string> keys, std::string description) 
                  : m_keys( std::move( keys)), m_description( std::move( description)) 
               {
                  if( m_keys.empty())
                     throw exception::invalid::Argument{ "at least one option 'key' has to be provided"};
               }

               inline bool has( const std::string& key) const
               {
                  return ! algorithm::find( keys(), key).empty();
               }

               inline const std::vector< std::string>& keys() const { return m_keys;} 
               inline const std::string& description() const { return m_description;} 
         
            private:
               std::vector< std::string> m_keys;
               std::string m_description;
            };


            
            struct basic_cardinality
            {

               template< typename I>
               inline void invoke( I&& invocable, const std::string& key, range_type values) 
               { 
                  validate::cardinality( key, m_cardinality, m_invoked + 1);
                  invocable.invoke( key, values);
                  ++m_invoked;
               }

               Cardinality cardinality() const { return m_cardinality;}

               template< size_type min, size_type max>
               auto create( cardinality::range< min, max> cardinality) const
               {
                  auto result = *this;
                  result.m_cardinality = cardinality;
                  result.m_invoked = 0;
                  return result;
               }

               inline void validate( const std::string& key) const 
               { 
                  validate::cardinality( key, m_cardinality, m_invoked);
               }

            private:
               Cardinality m_cardinality;
               size_type m_invoked = 0;
            };


            template< typename A>
            void traverse( Holder& holder, range_type arguments, A&& action)
            {               
               while( arguments)
               {
                  auto& key = *arguments;
                  ++arguments;

                  if( ! holder.has( key))
                     exception::correlation( key);

                  auto found = algorithm::find_if( arguments, [&]( auto& k){
                     return holder.next( k);
                  });

                  action( holder, key, range::make( std::begin( arguments), std::begin( found)));
                  arguments = found;
               }
            }

         } // detail
         

         struct Option : detail::basic_keys
         {
            using base_type = detail::basic_keys;
            
            template< typename I>
            Option( I&& invocable, std::vector< std::string> keys, std::string description)
               : base_type( std::move( keys), std::move( description)),
               m_invocable( detail::invoke::create( std::forward< I>( invocable))) 
            {}

            template< typename I, typename C>
            Option( I&& invocable, C&& completer, std::vector< std::string> keys, std::string description)
               : base_type( std::move( keys), std::move( description)),
               m_invocable( detail::invoke::create( std::forward< I>( invocable), std::forward< C>( completer))) 
            {}

            inline bool next( const std::string& key) const { return has( key);}

            inline void invoke( const std::string& key, range_type values) 
            { 
               assert( has( key));
               m_cardinality.invoke( m_invocable, key, values);
            }

            template< size_type min, size_type max>
            auto operator () ( cardinality::range< min, max> cardinality) const 
            { 
               static_assert( min + max != 0, "cardinality of [0..0] does not make sense");
               auto result = *this;
               result.m_cardinality = m_cardinality.create( cardinality);
               return result;
            }

            inline auto cardinality() const { return m_cardinality.cardinality();}


            inline auto operator + ( Option rhs) 
            {
               return detail::Holder::construct( *this, std::move( rhs));
            }

            detail::Invoked completion( const std::string& key, range_type values)
            {
               detail::Invoked result{ m_invocable};
               result.values = values;
               result.key = key;
               return result;
            }

            inline detail::Representation representation() const 
            { 
               detail::Representation result( m_invocable);
               result.keys = keys();
               result.description = description();
               result.cardinality = m_cardinality.cardinality();
               return result;
            }

            inline void validate() const 
            { 
               m_cardinality.validate( keys().back());
            }

         private:
            detail::Invoke m_invocable;
            detail::basic_cardinality m_cardinality;
         };


         struct Group : detail::basic_keys
         {

            template< typename C, typename... Os>
            Group( C&& invocable, std::vector< std::string> keys, std::string description, Os&&... options) 
               : detail::basic_keys( std::move( keys), std::move( description)),
                 m_invocable( detail::invoke::create( std::forward< C>( invocable))),
                 m_content( detail::Holder::construct( std::forward< Os>( options)...)) 
            {}


            inline bool has( const std::string& key) const
            {
               return m_invoke_content ? m_content.has( key) : basic_keys::has( key);
            }
            inline bool next( const std::string& key) const
            {
               return m_content.has( key) || basic_keys::has( key);
            }

            inline void invoke( const std::string& key, range_type values) 
            { 
               if( ! m_invoke_content)
               {
                  m_invocable.invoke( key, values);
                  m_invoke_content = true;
               }
               else 
               {
                  m_content.invoke( key, values);
               }
            }

            detail::Invoked completion( const std::string& key, range_type values)
            {
               if( ! m_invoke_content)
               {
                  m_invoke_content = true;

                  detail::Invoked result{ m_invocable};
                  result.key = key;
                  result.values = values;
                  return result;
               }
               else 
               {
                  auto result =  m_content.completion( key, values);
                  result.parent = common::coalesce( result.parent, detail::basic_keys::keys().back());
                  return result;
               }
            }
            inline detail::Representation representation() const 
            { 
               detail::Representation result( m_invocable);
               result.keys = keys();
               result.description = description();
               result.cardinality = m_cardinality.cardinality();
               result.nested = m_content.representation();
               return result;
            }



            template< size_type min, size_type max>
            auto operator () ( cardinality::range< min, max> cardinality) const 
            { 
               static_assert( min <= 1 && max == 1, "a group can only have cardinlaity [0..1] or [1]");

               auto result = *this;
               result.m_cardinality = m_cardinality.create( cardinality);
               return result;
            }

            inline void validate() const 
            { 
               m_cardinality.validate( keys().at( 0));
            }

         private:

            detail::Invoke m_invocable;
            detail::Holder m_content;
            detail::basic_cardinality m_cardinality;
            bool m_invoke_content = false;
         };


         namespace policy
         {
            struct Default
            {
               Default( std::string description);
               using callback_type = std::function< detail::Holder()>;
               void overrides( range_type arguments, callback_type callback);
               
               std::string description;
            };
         } // policy

         template< typename P>
         struct basic_parse
         {

            using policy_type = P;

            template< typename... Os>
            basic_parse( std::string description, Os&&... options) 
               :  m_policy( std::move( description)),
                  m_options( detail::Holder::construct( std::forward< Os>( options)...))
            {}

            void operator() ( int argc, char* argv[])
            {
               assert( argc > 0);
               basic_parse::operator()( algorithm::transform( range::make( argv + 1, argv + argc), []( const auto c){
                  return std::string( c);
               }));
            }

            void operator() ( std::vector< std::string> arguments)
            {
               basic_parse::operator()( range::make( arguments));
            }

            void operator() ( range_type arguments)
            {               
               m_policy.overrides( arguments, callback( *this));

               detail::traverse( m_options, arguments, []( auto& option, auto& key, auto argument){
                  option.invoke( key, argument);
               });

               m_options.validate();
            }

         private:

            static policy::Default::callback_type callback( const basic_parse& parse)
            {
               return [&parse](){ return parse.m_options;};
            }
            policy_type m_policy;
            detail::Holder m_options;
         };

         using Parse = basic_parse< policy::Default>;


         namespace option
         {
            //! return a functor that toggles the boolean
            //! usefull to set "flags", example: --verbose.
            inline auto toggle( bool& value)
            {
               return [&value](){
                  value = ! value;
               };
            }

         } // option
         
      } // argument
   } // common 
} // casual 
