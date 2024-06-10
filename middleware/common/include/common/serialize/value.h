//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/value/customize.h"
#include "common/serialize/archive/type.h"
#include "casual/concepts/serialize.h"

#include "common/traits.h"
#include "common/view/binary.h"
#include "common/code/serialize.h"
#include "common/flag.h"

#include "casual/concepts/serialize.h"

#include <optional>
#include <system_error>
#include <filesystem>

namespace casual
{
   namespace common::serialize
   {
   
      //! "customization point" for casual known stuff
      //! @{
      template< typename T, typename A> 
      struct Value;

      namespace traits
      {
         using namespace common::traits;

         template< typename T, typename A>
         using value_t = serialize::Value< std::remove_cvref_t< T>, std::remove_cvref_t< A>>;
         

      } // traits

      namespace composite
      {
         //! customization point for serialization
         template< typename T, typename A, typename Enable = void> 
         struct Value;

         namespace traits
         {
            template< typename T, typename A>
            using value_t = composite::Value< std::remove_cvref_t< T>, std::remove_cvref_t< A>>;
         } // traits
         
      } // composite
      //! @}

      namespace value
      {
         namespace indirection
         {
            // read
            // highest - the archive can natively handle the value
            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name, traits::priority::tag< 4>)
               -> decltype( archive.read( std::forward< T>( value), name))
            {
               return archive.read( std::forward< T>( value), name);
            }
            
            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name, traits::priority::tag< 3>) -> 
               decltype( customize::traits::value_t<T, A>::read( archive, std::forward< T>( value), name))
            {
               return customize::traits::value_t<T, A>::read( archive, std::forward< T>( value), name);
            }

            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name, traits::priority::tag< 2>) -> 
               decltype( customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name))
            {
               return customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name);
            }

            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name, traits::priority::tag< 1>) -> 
               decltype( value.serialize( archive, name))
            {
               return value.serialize( archive, name);
            }

            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name, traits::priority::tag< 0>) -> 
               decltype( traits::value_t<T, A>::read( archive, std::forward< T>( value), name))
            {
               return traits::value_t<T, A>::read( archive, std::forward< T>( value), name);
            }

            // write
            // highest - the archive can natively handle the value
            template< typename A, typename T>
            auto write( A& archive, const T& value, const char* name, traits::priority::tag< 4>)
               -> decltype( void( archive.write( value, name)))
            {
               archive.write( value, name);
            }

            template< typename A, typename T>
            auto write( A& archive, const T& value, const char* name, traits::priority::tag< 3>) -> 
               decltype( customize::traits::value_t<T, A>::write( archive, value, name))
            {
               customize::traits::value_t<T, A>::write( archive, value, name);
            }

            template< typename A, typename T>
            auto write( A& archive, const T& value, const char* name, traits::priority::tag< 2>) -> 
               decltype( customize::traits::value_t<T, A>::serialize( archive, value, name))
            {
               customize::traits::value_t<T, A>::serialize( archive, value, name);
            }

            template< typename A, typename T>
            auto write( A& archive, const T& value, const char* name, traits::priority::tag< 1>) -> 
               decltype( value.serialize( archive, name))
            {
               value.serialize( archive, name);
            }

            template< typename A, typename T>
            auto write( A& archive, const T& value, const char* name, traits::priority::tag< 0>) -> 
               decltype( traits::value_t<T, A>::write( archive, value, name))
            {
               traits::value_t<T, A>::write( archive, value, name);
            }

            // serialize
            // highest priority
            template< typename A, typename T>
            auto serialize( A& archive, T&& value, traits::priority::tag< 2>) -> 
               decltype( customize::composite::traits::value_t<T, A>::serialize( archive, std::forward< T>( value)))
            {
               customize::composite::traits::value_t<T, A>::serialize( archive, std::forward< T>( value));
            }
            
            template< typename A, typename T>
            auto serialize( A& archive, T&& value, traits::priority::tag< 1>) -> 
               decltype( composite::traits::value_t<T, A>::serialize( archive, std::forward< T>( value)))
            {
               composite::traits::value_t<T, A>::serialize( archive, std::forward< T>( value));
            }

            template< typename A, typename T>
            auto serialize( A& archive, T&& value, traits::priority::tag< 0>) -> 
               decltype( value.serialize( archive))
            {
               value.serialize( archive);
            }

         } // indirection


         template< typename A, typename T>
         auto write( A& archive, const T& value, const char* name)
            -> decltype( indirection::write( archive, value, name, traits::priority::tag< 4>{}))
         {
            // invoke the most prioritized implementation
            indirection::write( archive, value, name, traits::priority::tag< 4>{});
         }

         namespace detail
         {
            // invoke the most prioritized implementation
            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name)
            {
               return indirection::read( archive, std::forward< T>( value), name, traits::priority::tag< 4>{});
            }
         } // detail

         //! take care of read. if returns void -> and always return true
         template< typename A, typename T>
         auto read( A& archive, T&& value, const char* name)
         {
            if constexpr( std::same_as< bool, decltype( detail::read( archive, std::forward< T>( value), name))>)
               return detail::read( archive, std::forward< T>( value), name);
            
            // otherwise always return true.
            detail::read( archive, std::forward< T>( value), name);
            return true;
         }

         template< typename A, typename T>
         auto serialize( A& archive, T&& value) 
            -> decltype( indirection::serialize( archive, std::forward< T>( value), traits::priority::tag< 2>{}))
         {
            // invoke the most prioritized implementation
            indirection::serialize( archive, std::forward< T>( value), traits::priority::tag< 2>{});
         }


      } // value

      namespace detail::has::value
      {         
         template< typename A, typename V>
         concept serialize = requires( A a, V v)
         {
            { common::serialize::value::serialize( a, v) };
         };

      } // detail::has::value


      //! Specialization for serializable
      template< typename T, typename A>
      requires detail::has::value::serialize< A, T>
      struct Value< T, A>
      {
         template< typename V> 
         static void write( A& archive, const V& value, const char* name)
         {
            archive.composite_start( name);
            value::serialize( archive, value);
            archive.composite_end( name);
         }

         static bool read( A& archive, T& value, const char* name)
         {
            if( archive.composite_start( name))
            {
               value::serialize( archive, value);
               archive.composite_end( name);
               return true;
            }
            return false;
         }
      };

      namespace detail::integral::archive::conformant
      {
         //! @returns the promoted/converted `value`. 
         //!    Assumes that compiler will optimize this call away if integral types
         //!    can be "casted" directly.
         template< typename R, typename T>
         R convert( T value) noexcept { return value;}

         template< typename T>
         auto convert( T value) noexcept
         {
            if constexpr( concepts::any_of< T, std::int8_t, std::uint8_t>)
               return convert< char>( value);
            if constexpr( concepts::any_of< T, std::uint16_t>)
               return convert< short>( value);
            if constexpr( concepts::any_of< T, int, unsigned int, std::int32_t, std::uint32_t>)
               return convert< long>( value);
            if constexpr( concepts::any_of< T, std::uint64_t, unsigned long>)
               return convert< long>( value);
         };
      } // detail::integral::archive::conformant

      namespace detail
      {
         template< typename T>
         concept odd_integral_types = concepts::any_of< T, 
         std::int8_t, std::uint8_t, std::uint16_t, int, unsigned int, std::int32_t, std::uint32_t, unsigned long, std::uint64_t>;
         
      } // detail

      //! Specialization for "odd" integral types
      template< detail::odd_integral_types T, typename A>
      struct Value< T, A>
      {
         static void write( A& archive, const T& value, const char* name)
         {
            using conformant_type = decltype( detail::integral::archive::conformant::convert( value));
            static_assert( concepts::serialize::archive::native::type< conformant_type>);

            archive.write( detail::integral::archive::conformant::convert( value), name);
         }

         static bool read( A& archive, T& value, const char* name)
         {
            using conformant_type = decltype( detail::integral::archive::conformant::convert( value));
            static_assert( concepts::serialize::archive::native::type< conformant_type>);

            conformant_type conformant;

            if( ! archive.read( conformant, name))
               return false;

            value = detail::integral::archive::conformant::convert< T>( conformant);
            return true;
         }
      };

      //! Specialization for enum
      template< concepts::enumerator T, typename A>
      struct Value< T, A>
      {
         template< typename V> 
         static void write( A& archive, V&& value, const char* name)
         {
            value::write( archive, static_cast< std::underlying_type_t< T>>( value), name);
         }

         static bool read( A& archive, T& value, const char* name)
         {
            std::underlying_type_t< T> underlying; 
            if( value::read( archive, underlying, name))
            {
               value = static_cast< T>( underlying);
               return true;
            }
            return false;
         }
      };

      namespace detail
      {
         namespace tuple
         {
            template< platform::size::type index>
            struct Write
            {
               template< typename A, typename T>
               static void serialize( A& archive, const T& value)
               {
                  value::write( archive, std::get< std::tuple_size< T>::value - index>( value), nullptr);
                  Write< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct Write< 0>
            {
               template< typename A, typename T>
               static void serialize( A&, const T&) {}
            };

            template< platform::size::type index>
            struct Read
            {
               template< typename A, typename T>
               static void serialize( A& archive, T& value)
               {
                  value::read( archive, std::get< std::tuple_size< T>::value - index>( value), nullptr);
                  Read< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct Read< 0>
            {
               template< typename A, typename T>
               static void serialize( A&, T&) {}
            };

            namespace has
            {
               template< typename T>
               concept tuple_start = requires( T a)
               {
                  a.tuple_start( platform::size::type{}, "");
               };

               template< typename T>
               concept tuple_end = requires( T a)
               {
                  a.tuple_end( "");
               };
            } // has

            template< typename A>
            auto start( A& archive, platform::size::type size, const char* name)
            {
               if constexpr( has::tuple_start< A>)
                  return archive.tuple_start( size, name);
               else
                  return archive.container_start( size, name);
            }

            template< typename A>
            auto end( A& archive, const char* name)
            {
               if constexpr( has::tuple_end< A>)
                  return archive.tuple_end( name);
               else
                  return archive.container_end( name);
            }

         } // tuple
      } // detail

      //! Specialization for tuple
      template< concepts::tuple::like T, typename A>
      struct Value< T, A>
      {
         template< typename V> 
         static void write( A& archive, V&& value, const char* name)
         {
            detail::tuple::start( archive, std::tuple_size< T>::value, name);
            detail::tuple::Write< std::tuple_size< T>::value>::serialize( archive, value);
            detail::tuple::end( archive, name);
         }

         static bool read( A& archive, T& value, const char* name)
         {
            constexpr auto expected_size = std::tuple_size< T>::value;
            const auto [ size, exists] = detail::tuple::start( archive, expected_size, name);

            if( exists)
            {
               if( expected_size != size)
                  throw std::system_error{ std::make_error_code( std::errc::invalid_argument), "unexpected tuple size"};

               detail::tuple::Read< std::tuple_size< T>::value>::serialize( archive, value);

               detail::tuple::end( archive, name);
               return true;
            }
            return false;
         }
      };

      namespace detail
      {
         template< typename T>
         concept container_like = concepts::container::like< std::remove_cvref_t< T>>
               && ! concepts::string::like< std::remove_cvref_t< T>>
               && ! concepts::serialize::archive::native::type< std::remove_cvref_t< T>>;

         namespace container
         {
            template< typename T>
            struct value { using type = T;};

            template< typename K, typename V>
            struct value< std::pair< K, V>> { using type = std::pair< std::remove_cvref_t< K>, V>;};

            template< typename T> 
            using value_t = typename value< std::remove_cvref_t< T>>::type;

            template< typename A, typename C> 
            void write( A& archive, C&& container, const char* name)
            {
               archive.container_start( container.size(), name);

               for( auto& element : container)
                  serialize::value::write( archive, element, nullptr);

               archive.container_end( name);
            }

            template< typename A, typename C>
            auto read( A& archive, C& container, const char* name)
            {
               auto [ size, exists] = archive.container_start( container.size(), name);

               if( exists)
               {
                  if constexpr( concepts::container::value::insert< C>)
                  {
                     auto count = size;

                     while( count-- > 0)
                     {
                        // we need to get rid of const key (if pair), so we can serialize
                        container::value_t< typename std::remove_cvref_t< C>::value_type> element;
                        serialize::value::read( archive, element, nullptr);

                        container.insert( std::move( element));
                     }
                  }
                  else
                  {
                     static_assert( concepts::container::sequence< C>);
                     container.resize( size);

                     for( auto& element : container)
                        serialize::value::read( archive, element, nullptr);
                  }

                  archive.container_end( name);
                  return true;
               }
               return false;
            }

         } // container
         
      } // detail


      //! Specialization for containers
      template< detail::container_like T, typename A>
      struct Value< T, A>
      {
         template< typename C> 
         static void write( A& archive, C&& container, const char* name)
         {
            detail::container::write( archive, std::forward< C>( container), name);
         }

         static bool read( A& archive, T& value, const char* name)
         {
            return detail::container::read( archive, value, name);
         }
      };

      //! Specialization for binary array likes
      template< typename T, typename A>
      requires concepts::container::array< T> && concepts::binary::like< T>
      struct Value< T, A>
      {
         template< typename V> 
         static void write( A& archive, V&& value, const char* name)
         {
            value::write( archive, view::binary::make( value), name);
         }

         static bool read( A& archive, T& value, const char* name)
         {
            return value::read( archive, view::binary::make( value), name);
         }
      };

      namespace detail
      {
         namespace optional
         {
            template< typename A, typename V> 
            void write_named( A& archive, V&& value, const char* name)
            {
               if( value)
                  value::write( archive, value.value(), name);
            }

            template< typename A, typename V> 
            void write_order_type( A& archive, V&& value)
            {
               if( value)
               {
                  archive.write( true, nullptr);
                  value::write( archive, value.value(), nullptr);
               }
               else 
                  archive.write( false, nullptr);
            }

            template< typename A, typename V> 
            void write( A& archive, V&& value, [[maybe_unused]] const char* name)
            {
               if constexpr( A::archive_type() == archive::Type::static_need_named)
                  write_named( archive, value, name);
               else if constexpr( A::archive_type() == archive::Type::static_order_type)
                  write_order_type( archive, value);
               else if constexpr( A::archive_type() == archive::Type::dynamic_type)
               {
                  if( archive.type() == archive::dynamic::Type::named)
                     write_named( archive, value, name);
                  else
                     write_order_type( archive, value);
               }
            }

            template< typename A, typename V>
            auto read_named( A& archive, V& value, const char* name)
            {
               std::decay_t< decltype( value.value())> contained;

               if( value::read( archive, contained, name))
               {
                  value = std::move( contained);
                  return true;
               }
               return false;
            }

            template< typename A, typename V>
            auto read_order_type( A& archive, V& value)
            {
               bool has_value = false;
               archive.read( has_value, nullptr);

               if( has_value)
               {
                  std::remove_cvref_t< decltype( value.value())> contained{};
                  value::read( archive, contained, nullptr);
                  value = std::move( contained);
               }

               return has_value;
            }

            template< typename A, typename V>
            auto read( A& archive, V& value, [[maybe_unused]] const char* name)
            {
               if constexpr( A::archive_type() == archive::Type::static_need_named)
                  return read_named( archive, value, name);
               else if constexpr( A::archive_type() == archive::Type::static_order_type)
                  return read_order_type( archive, value);
               else if constexpr( A::archive_type() == archive::Type::dynamic_type)
               {
                  if( archive.type() == archive::dynamic::Type::named)
                     return read_named( archive, value, name);
                  else
                     return read_order_type( archive, value);
               }
            }
         } // optional
      } // detail

      //! Specialization for optional-like (that hasn't 'serialize')
      template< typename T, typename A>
      requires ( concepts::optional::like< T> && ! concepts::serialize::has::serialize< T, A>)
      struct Value< T, A>
      {
         template< typename V> 
         static void write( A& archive, V&& value, const char* name)
         {  
            // dispatch depending on archive since 'binary archive' need to serialize with different semantics
            detail::optional::write( archive, std::forward< V>( value), name);
         }

         template< typename V>
         static bool read( A& archive, V& value, const char* name)
         {
            // dispatch depending on archive since 'binary archive' need to serialize with different semantics
            return detail::optional::read( archive, value, name);
         }
      };

      //! Specialization for time
      //! @{

      template< typename R, typename P, typename A>
      struct Value< std::chrono::duration< R, P>, A>
      {
         using value_type = std::chrono::duration< R, P>;

         template< typename V> 
         static void write( A& archive, V&& value, const char* name)
         {
            value::write( archive, std::chrono::duration_cast< platform::time::serialization::unit>( value).count(), name);
         }

         static bool read( A& archive, value_type& value, const char* name)
         {
            platform::time::serialization::unit::rep representation;

            if( value::read( archive, representation, name))
            {
               value = std::chrono::duration_cast< value_type>( platform::time::serialization::unit{ representation});
               return true;
            }
            return false;
         }
      };

      template< typename A>
      struct Value< platform::time::point::type, A>
      {
         static void write( A& archive, platform::time::point::type value, const char* name)
         {
            value::write(
               archive, 
               std::chrono::time_point_cast< platform::time::serialization::unit>( value).time_since_epoch(), 
               name);
         }

         static bool read( A& archive, platform::time::point::type& value, const char* name)
         {
            platform::time::serialization::unit duration;
            if( value::read( archive, duration, name))
            {
               value = platform::time::point::type{ std::chrono::duration_cast< platform::time::unit>( duration)};
               return true;
            }
            return false;
         }
      };
      //! @}

      //! Specialization for std::error_code
      template< typename A>
      struct Value< std::error_code, A>
      {
         static auto write( A& archive, const std::error_code& code, const char* name)
         {
            archive.composite_start( name);
            value::write( archive, code::serialize::lookup::id( code.category()), "id");
            value::write( archive, code.value(), "value");
            archive.composite_end( name);
         }
         static auto read( A& archive, std::error_code& code, const char* name)
         {
            if( ! archive.composite_start( name))
               return false;

            Uuid id;
            value::read( archive, id, "id");
            decltype( code.value()) value{};
            value::read( archive, value, "value");

            code = code::serialize::create( id, value);

            archive.composite_end( name);
            return true;
         }
      };  

      //! Specialization for std::filesystem::path
      template< typename A>
      struct Value< std::filesystem::path, A>
      {
         static auto write( A& archive, const std::filesystem::path& path, const char* name)
         {
            value::write( archive, path.u8string(), name);
         }

         static auto read( A& archive, std::filesystem::path& path, const char* name)
         {
            std::u8string data;
            if( value::read( archive, data, name))
            {
               path = std::move( data);
               return true;
            }
            return false;
         }
      };  

      //! Specialization for named value
      template< concepts::serialize::named::value T, typename A>
      struct Value< T, A>
      {
         template< typename V>
         static auto write( A& archive, V&& value, const char*)
         {
            value::write( archive, value.value(), value.name());
         }

         template< typename V>
         static auto read( A& archive, V&& value, const char*)
         {
            return value::read( archive, value.value(), value.name());
         }
      };

   } // common::serialize
} // casual
