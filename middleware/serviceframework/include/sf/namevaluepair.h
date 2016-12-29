//!
//! casual
//!


#ifndef CASUAL_NAMEVALUEPAIR_H
#define CASUAL_NAMEVALUEPAIR_H

#include "common/traits.h"

#include <utility>
#include <type_traits>
#include <tuple>

namespace casual
{
   namespace sf
   {
      namespace name
      {
         namespace value
         {
            template <typename T, typename R>
            class Pair;


            //!
            //! Holds non const lvalue types
            //!
            template <typename T>
            class Pair< T, std::true_type> : public std::tuple< const char*, T*>
            {
            public:

               explicit Pair (const char* name, T& value)
               :  std::tuple< const char*, T*>( name, &value) {}

               const char* name() const
               {
                  return std::get< 0>( *this);
               }

               T& value() const
               {
                  return  *( std::get< 1>( *this));
               }
            };

            //!
            //! Holds const lvalue types
            //!
            template <typename T>
            class Pair< const T, std::true_type> : public std::tuple< const char*, const T*>
            {
            public:

               explicit Pair (const char* name, const T& value)
               :  std::tuple< const char*, const T*>( name, &value) {}

               const char* name() const
               {
                  return std::get< 0>( *this);
               }

               const T& value() const
               {
                  return *( std::get< 1>( *this));
               }
            };


            //!
            //! Holds rvalue types
            //!
            template <typename T>
            class Pair< T, std::false_type> : public std::tuple< const char*, T>
            {
            public:

               explicit Pair (const char* name, T&& value)
               :  std::tuple< const char*, T>( name, std::move( value)) {}

               const char* name() const
               {
                  return std::get< 0>( *this);
               }

               const T& value() const
               {
                  return std::get< 1>( *this);
               }
            };

            namespace pair
            {
               namespace internal
               {
                  template< typename T>
                  using nvp_traits_t = Pair< typename std::remove_reference< T>::type, typename std::is_lvalue_reference<T>::type>;
               }

               template< typename T>
               auto make( const char* name, T&& value) -> decltype( internal::nvp_traits_t< T>( name, std::forward< T>( value)))
               {
                  return internal::nvp_traits_t< T>( name, std::forward< T>( value));
               }
            } // pair

         } // value
      } // name

      namespace traits
      {
         namespace detail
         {
            template< typename T>
            struct is_nvp : std::false_type {};

            template< typename T, typename R>
            struct is_nvp< name::value::Pair< T, R>> : std::true_type {};

         } // detail
         template< typename T>
         struct is_nvp : detail::is_nvp< common::traits::decay_t< T>> {};

      } // traits
   } // sf
} // casual


#define CASUAL_MAKE_NVP( member) \
      casual::sf::name::value::pair::make( #member, member)


#define CASUAL_CONST_CORRECT_SERIALIZE( statement) \
      template< typename A>  \
      void serialize( A& archive) \
      {  \
   statement  \
      } \
      template< typename A>  \
      void serialize( A& archive) const\
      {  \
         statement  \
      } \


//#define CASUAL_READ_WRITE( )



#endif
