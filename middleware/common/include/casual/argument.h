//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/argument/cardinality.h"

#include "casual/platform.h"
#include "casual/concepts.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/algorithm.h"
#include "common/string.h"

#include <algorithm>

namespace casual
{
   namespace argument
   {
      
      using range_type = std::span< std::string_view>;
      using size_type = platform::size::type;

      namespace reserved
      {
         namespace name
         {
            constexpr std::string_view completion = "casual-bash-completion";
            constexpr std::string_view help = "--help";
            constexpr std::string_view suggestions = "<value>";
         } // name
      } // reserved

      struct Option;

      namespace option
      {
         namespace invoke
         {
            struct preemptive{};
            
         } // invoke
      } // option

      namespace detail
      {
         namespace validate
         {
            void cardinality( const Cardinality& cardinality, std::string_view key, size_type value);

            namespace value
            {
               void cardinality( const Cardinality& cardinality, std::string_view key, range_type values);
            } // value

         } // validate

         namespace concepts
         {
            template< typename T>
            concept tuple_like = requires( T value) 
            { 
               std::tuple_size< T>::value; 
               //std::get< 0>( value); 
            };

            template< typename T>
            concept container_like = casual::concepts::container::sequence< T> && ! casual::concepts::string::like< T>;

            template< typename T>
            concept optional_like = casual::concepts::optional::like< T>;

            template< typename T>
            concept callable_like = common::traits::is::function< T>;

            template< typename T>
            concept invocable = callable_like< T> || tuple_like< T>;

            template< typename T>
            concept completable = requires( T completer, bool test, range_type argument) 
            { 
               { completer( test, argument)} -> casual::concepts::any_of< std::string, std::vector< std::string>>;
            };

            
         } // concepts

         namespace value
         {
            template< typename T> 
            struct traits;

            //! helper to decay the `T`. 
            template< typename T>
            using traits_d = traits< std::remove_cvref_t< T>>;

            // default for a single argument
            template< typename T> 
            struct traits
            {
               static_assert( ! concepts::invocable< T>);

               constexpr static auto cardinality() { return cardinality::one();}

               static range_type assign( range_type values, T& value) 
               { 
                  if constexpr( std::same_as< std::string_view, T>)
                     value = values.front();
                  else
                     value = common::string::from< T>( values.front());

                  return values.subspan( 1);
               }
            };

            // container sequences
            template< concepts::container_like T>
            struct traits< T>
            {
               using value_type = std::ranges::range_value_t< T>;
               constexpr static auto value_cardinality = traits< value_type>::cardinality();

               static_assert( value_cardinality.fixed(), "a container can't have value type of dynamic cardinality");
               
               constexpr static auto cardinality() {return cardinality::any().steps( value_cardinality.min());}

               static range_type assign( range_type values, T& result) 
               { 
                  while( ! values.empty())
                  {
                     value_type value;
                     values = traits< value_type>::assign( values, value);
                     result.push_back( std::move( value));
                  }
                  return values;
               }
            };
            

            // tuple
            template< concepts::tuple_like T> 
            struct traits< T>
            {
               constexpr static auto cardinality() 
               {
                  return element_cardinality<>();
               };

               static range_type assign( range_type values, T& result) 
               {
                  return std::apply( [ &values]( auto&&... args)
                  {
                     // fold on args
                     ( ( values = traits_d< decltype( args)>::assign( values, args)) , ... );

                     return values;

                  }, result);
               }

            private:

               template< std::size_t Index = 0>
               constexpr static argument::Cardinality element_cardinality() 
               {
                  if constexpr( std::tuple_size_v< T> == 0)
                     return argument::cardinality::zero();
                  else if constexpr( Index + 1 == std::tuple_size_v< T>)
                     return traits_d< std::tuple_element_t< Index, T>>::cardinality();
                  else 
                     return traits_d< std::tuple_element_t< Index, T>>::cardinality()
                        + element_cardinality< Index + 1>();
               }
            };

            // optional like
            template< concepts::optional_like T> 
            struct traits< T>
            {
               using value_type = std::decay_t< decltype( std::declval< T&>().value())>;
               constexpr static auto value_cardinality = traits< value_type>::cardinality();

               static_assert( value_cardinality.fixed(), "an optional like argument can't have value type of dynamic cardinality");

               constexpr static auto cardinality() 
               {
                  return cardinality::range( 0, value_cardinality.min(), value_cardinality.min());
               };

               static range_type assign( range_type values, T& result) 
               { 
                  if( ! values.empty())
                  {
                     value_type value;
                     values = traits_d< value_type>::assign( values, value);
                     result = std::move( value);
                  }
                  return values;
               }
            };
               
         } // value

         namespace option
         {
            namespace invoke
            {
               enum struct Phase : short
               {
                  regular,
                  preemptive,
               };
               
            } // invoke


            struct Assigned 
            {
               template< concepts::callable_like C, concepts::tuple_like T>
               Assigned( C callable, T arguments)
                  : m_invocable{ std::make_unique< Model< C, T>>( std::move( callable), std::move( arguments))}
               {}

               inline void invoke() { m_invocable->invoke();}
               inline invoke::Phase phase() const { return m_invocable->phase();}

            private:

               struct Interface
               {
                  virtual ~Interface() = default;
                  virtual void invoke() = 0;
                  virtual invoke::Phase phase() const = 0;
               };

               template< typename C, typename T>
               struct Model : Interface 
               {
                  Model( C callable, T arguments) 
                     : m_callable{ std::move( callable)}, m_arguments{ std::move( arguments)}
                  {}

                  void invoke() override
                  { 
                     std::apply( m_callable, m_arguments);
                  }

                  invoke::Phase phase() const override
                  {
                     if constexpr( std::same_as< argument::option::invoke::preemptive, decltype( std::apply( m_callable, m_arguments))>)
                        return invoke::Phase::preemptive;
                     return invoke::Phase::regular;
                  }

                  C m_callable;
                  T m_arguments;
               };

               std::unique_ptr< Interface> m_invocable;
            };

            template< typename C>
            struct basic_invoke;

            template< concepts::callable_like C>
            struct basic_invoke< C> 
            {
               using callable_type = C;
               using tuple_type = typename common::traits::function< callable_type>::decayed;
               
               basic_invoke( callable_type callable) : m_callable( std::move( callable)) {}

               Assigned assign( std::string_view key, range_type values)
               {
                  validate::value::cardinality( cardinality(), key, values);

                  tuple_type arguments;
                  value::traits_d< tuple_type>::assign( values, arguments);

                  return Assigned{ m_callable, std::move( arguments)};
               };

               constexpr auto cardinality() const 
               {
                  return value::traits< tuple_type>::cardinality();
               }

            private:
               callable_type m_callable;
            };

            template< concepts::tuple_like T> 
            struct basic_invoke< T> 
            {
               using tuple_type = T;

               basic_invoke( tuple_type tuple) : m_tuple( std::move( tuple)) {}

               void assign( std::string_view key, range_type values)
               {
                  validate::value::cardinality( cardinality(), key, values);
                  value::traits_d< tuple_type>::assign( values, m_tuple);
               };

               constexpr auto cardinality() const 
               {
                  return value::traits_d< tuple_type>::cardinality();
               }

            private:
               tuple_type m_tuple;
            };

            template< detail::concepts::invocable I> 
            struct default_completer 
            {
               std::vector< std::string> operator() ( bool help, range_type values) const
               {
                  // if the invocable is callable, we need to check the argument cardinality
                  if constexpr( concepts::callable_like< I>)
                  {
                     using argument_type = typename common::traits::function< I>::decayed;
                     if constexpr( value::traits_d< argument_type>::cardinality() == cardinality::zero())
                        return {};
                  }
                  else if constexpr( value::traits_d< I>::cardinality() == cardinality::zero())
                     return {};

                  return { std::string{ reserved::name::suggestions}};
               }
            };

            
         } // option

        
         //! exposed for unittests
         std::vector< detail::option::Assigned> assign( std::span< Option> options, range_type arguments);

         void parse( std::span< Option> options, range_type arguments);

         void complete( std::span< Option> options, range_type arguments);
         

         struct Policy
         {

            static void help( std::string_view description, std::span< const Option> options, range_type arguments);

            static std::vector< std::string> help_names();

            static Option help_option( std::vector< std::string> names);
         };

      } // detail

      namespace option
      {
         struct Names
         {
            inline Names( std::vector< std::string> active, std::vector< std::string> deprecated)
               : m_active( std::move( active)), m_deprecated( std::move( deprecated)) 
            {
               if( m_active.empty() && m_deprecated.empty())
                  common::code::raise::error( common::code::casual::invalid_argument, "at least one option 'key' has to be provided");
            }

            inline Names( std::vector< std::string> active) : Names{ std::move( active), {}} 
            {}

            inline friend bool operator == ( const Names& lhs, std::string_view key) 
            { 
               return common::algorithm::find( lhs.m_active, key) || common::algorithm::find( lhs.m_deprecated, key);
            }

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_active);
               CASUAL_SERIALIZE( m_deprecated);
            )

            inline const auto& active() const { return m_active;}
            inline const auto& deprecated() const { return m_deprecated;}
            
            inline const std::string& canonical() const 
            {
               if( ! m_active.empty())
                  return m_active.back();
               return m_deprecated.back();
            }
            
         private:
            std::vector< std::string> m_active;
            std::vector< std::string> m_deprecated;

         };
      
      } // option 

      struct Option
      {
         template< detail::concepts::invocable I>
         Option( I invocable, option::Names names, std::string description)
            : m_names{ std::move( names)}, 
               m_invocable{ Option::create( std::move( invocable))},
               m_description{ std::move( description)}
         {};

         template< detail::concepts::invocable I, detail::concepts::completable C>
         Option( I invocable, C completer, option::Names names, std::string description)
            : m_names{ std::move( names)}, 
               m_invocable{ Option::create( std::move( invocable), std::move( completer))},
               m_description{ std::move( description)}
         {};

         Option( detail::concepts::invocable auto invocable, std::vector< std::string> names, std::string description)
            : Option{ std::move( invocable), option::Names{ names}, std::move( description)} {}

         Option( detail::concepts::invocable auto invocable, detail::concepts::completable auto completer, std::vector< std::string> names, std::string description)
            : Option{ std::move( invocable), std::move( completer), option::Names{ names}, std::move( description)} {}
         
         //! 'construction continuation'. 
         //! @returns this object with cardinality set to the provided `cardinality`.
         inline Option operator () ( Cardinality cardinality) &&
         {
            auto result = std::move( *this);
            result.m_cardinality = std::move( cardinality);
            return result;
         }

         //! 'construction continuation'. 
         //! @returns this object with suboptions set to the provided `suboptions`.
         inline Option operator () ( std::vector< Option> suboptions) &&
         {
            auto result = std::move( *this);
            result.m_suboptions = std::move( suboptions);
            return result;
         }

         //! explict "usage" of the option, used when "consuming" options during complete
         void use()
         {
            ++m_usage;
         }
         
         inline friend bool operator == ( const Option& lhs, std::string_view rhs) { return lhs.m_names == rhs;}

         //! assigns `values` to the option. @returns `Assigned` if the option can be invoked, otherwise std::nullopt. 
         inline std::optional< detail::option::Assigned> assign( std::string_view key, range_type values)
         {
            ++m_usage;
            return m_invocable->assign( key, values);
         }

         //! @returns number of "usage" for the option. used to verify option cardinality
         inline auto usage() const { return m_usage;}

         //! @returns completion/help information for the option
         inline std::vector< std::string> complete( bool test, range_type values) const 
         { 
            return m_invocable->complete( test, values);
         }

         //! @returns the cardinality of the values this option takes
         inline Cardinality value_cardinality() const { return m_invocable->value_cardinality();}

         inline const auto& names() const { return m_names;}
         inline auto& suboptions() { return m_suboptions;}
         inline auto& suboptions() const { return m_suboptions;}
         //! @returns the cardinality of this option 
         inline auto cardinality() const { return m_cardinality;}
         inline auto& description() const { return m_description;}

         //! @returns true if another "usage" would invalidate option cardinality
         inline bool exhausted() const
         {
            return ! m_cardinality.valid( m_usage + 1);
         }

      private:


         struct Interface
         {
            virtual ~Interface() = default;
            virtual std::optional< detail::option::Assigned> assign( std::string_view key, range_type values) = 0;
            virtual std::vector< std::string> complete( bool test, range_type values) const = 0;
            virtual Cardinality value_cardinality() const = 0;
         };

         template< typename I, typename C>
         struct Model : Interface 
         {
            Model( I invocable, C completable) : m_invocable{ std::move( invocable)}, m_completable{ std::move( completable)}
            {}

            std::optional< detail::option::Assigned> assign( std::string_view key, range_type values) override
            {
               if constexpr( std::same_as< void, decltype( m_invocable.assign( key, values))>)
               {
                  m_invocable.assign( key, values);
                  return std::nullopt;
               }
               else
                  return m_invocable.assign( key, values);
            }

            std::vector< std::string> complete( bool test, range_type values) const override
            {
               if constexpr( std::same_as< std::string, decltype( m_completable( test, values))>)
                  return { m_completable( test, values)};
               else 
                  return m_completable( test, values);
            }

            Cardinality value_cardinality() const override
            {
               return m_invocable.cardinality();
            }
         
         private:
            I m_invocable;
            C m_completable;
         };

         template< detail::concepts::invocable I>
         static std::shared_ptr< Interface> create( I invocable)
         {
            using model_type = Model< detail::option::basic_invoke< I>, detail::option::default_completer< I>>;
            return std::make_shared< model_type>( std::move( invocable), detail::option::default_completer< I>{});
         }

         template< detail::concepts::invocable I,  detail::concepts::completable C>
         static std::shared_ptr< Interface> create( I invocable, C completer)
         {
            using model_type = Model< detail::option::basic_invoke< I>, C>;
            return std::make_shared< model_type>( std::move( invocable), std::move( completer));
         }

         option::Names m_names;
         std::shared_ptr< Interface> m_invocable;
         // the option cardinality
         Cardinality m_cardinality = cardinality::zero_one();
         std::vector< Option> m_suboptions;
         platform::size::type m_usage{};
         std::string m_description;

      };

      template< typename P>
      struct basic_parse
      {
         using policy_type = P;

         template< detail::concepts::container_like A>
         static void operator () ( std::string_view description, std::vector< Option> options, A arguments) 
         {
            if constexpr( std::same_as< A, std::vector< std::string_view>>)
               parse( description, std::move( options), std::move( arguments));
            else
               parse( description, std::move( options), std::vector< std::string_view>{ std::begin( arguments), std::end( arguments)});
         }

         static void operator ()( std::string_view description, std::vector< Option> options, int argc, char const* const* argv)
         {
            assert( argc > 0);
            parse( description, std::move( options), std::vector< std::string_view>{ argv + 1, argv + argc});
         }

         static void operator () ( std::string_view description, std::vector< Option> options,  std::initializer_list< std::string_view> arguments)
         {
            parse( description, std::move( options), std::vector< std::string_view>{ std::begin( arguments), std::end( arguments)});
         }

      private:

         static void parse( std::string_view description, std::vector< Option> options, std::vector< std::string_view> arguments)
         {
            // special treatment for completion
            if( auto found = common::algorithm::find( arguments, reserved::name::completion))
            {
               common::algorithm::rotate( arguments, found);
               detail::complete( options, range_type{ arguments}.subspan( 1));
               return;
            }

            options.push_back( policy_type::help_option( policy_type::help_names()));
            auto& help = options.back();

            // special treatment for _help_
            if( auto found = common::algorithm::find( arguments, help))
            {
               common::algorithm::rotate( arguments, found);
               policy_type::help( description, options, range_type{ arguments}.subspan( 1));
               return;
            }

            // the regular parse
            detail::parse( options, arguments);
         }

      };


      constexpr auto parse = basic_parse< detail::Policy>{};

      namespace option
      {
         //! return a functor that sets the provided `value`
         //! to `true` when invoked
         inline auto flag( bool& value)
         {
            return [&value]()
            {
               value = ! value;
               return option::invoke::preemptive{};
            };
         }

         namespace one
         {
            namespace detail
            {
               using namespace common;

               template< argument::detail::concepts::callable_like T>
               requires ( traits::function< T>::arguments() == 1 && std::same_as< typename traits::function< T>::result_type, void>)
               auto many( T&& callable)
               {
                  using rest_type = std::remove_cvref_t< typename traits::function< T>::template argument< 0>::type>;
                  using first_type = std::ranges::range_value_t< rest_type>;

                  return [ callable = std::forward< T>( callable)]( first_type first, rest_type rest)
                  {
                     rest.insert( std::begin( rest), std::move( first));
                     callable( std::move( rest));
                  };
               }

               template< typename T>
               auto many( std::vector< T>& values)
               {
                  return [&values]( T first, std::vector< T> rest)
                  {
                     values.reserve( values.size() + rest.size() + 1);
                     values.push_back( std::move( first));
                     algorithm::move( std::move( rest), values);
                  };
               }

            } // detail

            template< typename T> 
            auto many( T&& dispatch) -> decltype( detail::many( std::forward< T>( dispatch)))
            {
               return detail::many( std::forward< T>( dispatch));
            }

         } // one

      } // option

   } // argument
} // casual
