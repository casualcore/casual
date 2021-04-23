//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/value/customize.h"

#include "common/serialize/traits.h"
#include "common/view/binary.h"
#include "common/code/serialize.h"
#include "common/string/utf8.h"

#include <optional>
#include <system_error>
#include <filesystem>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         //! "customization point" for casual known stuff
         //! @{
         template< typename T, typename A, typename Enable = void> 
         struct Value;

         namespace traits
         {
            template< typename T, typename A>
            using value_t = serialize::Value< common::traits::remove_cvref_t< T>, common::traits::remove_cvref_t< A>>;
            

         } // traits

         namespace composit
         {
            //! customization point for serialization
            template< typename T, typename A, typename Enable = void> 
            struct Value;

            namespace traits
            {
               template< typename T, typename A>
               using value_t = composit::Value< common::traits::remove_cvref_t< T>, common::traits::remove_cvref_t< A>>;
            } // traits
            
         } // composit
         //! @}

         namespace value
         {
            namespace indirection
            {
               // lowest priority


               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 0>) -> 
                  decltype( traits::value_t<T, A>::read( archive, std::forward< T>( value), name))
               {
                  return traits::value_t<T, A>::read( archive, std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 1>) -> 
                  decltype( value.serialize( archive, name))
               {
                  return value.serialize( archive, name);
               }

               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 2>) -> 
                  decltype( customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name))
               {
                  return customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name);
               }  

               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 3>) -> 
                  decltype( customize::traits::value_t<T, A>::read( archive, std::forward< T>( value), name))
               {
                  return customize::traits::value_t<T, A>::read( archive, std::forward< T>( value), name);
               }
               
               // highest - the archive can natively handle the value
               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 4>)
                  -> decltype( archive.read( std::forward< T>( value), name))
               {
                  return archive.read( std::forward< T>( value), name);
               }


               // write

               // lowest priority

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 0>) -> 
                  decltype( traits::value_t<T, A>::write( archive, std::forward< T>( value), name))
               {
                  traits::value_t<T, A>::write( archive, std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 1>) -> 
                  decltype( value.serialize( archive, name))
               {
                  value.serialize( archive, name);
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 2>) -> 
                  decltype( customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name))
               {
                  customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 3>) -> 
                  decltype( customize::traits::value_t<T, A>::write( archive, std::forward< T>( value), name))
               {
                  customize::traits::value_t<T, A>::write( archive, std::forward< T>( value), name);
               }

               // highest - the archive can natively handle the value
               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 4>)
                  -> decltype( archive.write( std::forward< T>( value), name))
               {
                  archive.write( std::forward< T>( value), name);
               }


               // lowest priority
               template< typename A, typename T>
               auto serialize( A& archive, T&& value, traits::priority::tag< 0>) -> 
                  decltype( value.serialize( archive))
               {
                  value.serialize( archive);
               }

               template< typename A, typename T>
               auto serialize( A& archive, T&& value, traits::priority::tag< 1>) -> 
                  decltype( composit::traits::value_t<T, A>::serialize( archive, std::forward< T>( value)))
               {
                  composit::traits::value_t<T, A>::serialize( archive, std::forward< T>( value));
               }

               template< typename A, typename T>
               auto serialize( A& archive, T&& value, traits::priority::tag< 2>) -> 
                  decltype( customize::composit::traits::value_t<T, A>::serialize( archive, std::forward< T>( value)))
               {
                  customize::composit::traits::value_t<T, A>::serialize( archive, std::forward< T>( value));
               }
               
            } // indirection


            template< typename A, typename T>
            auto write( A& archive, T&& value, const char* name)
               -> decltype( indirection::write( archive, std::forward< T>( value), name, traits::priority::tag< 4>{}))
            {
               // invoke the most prioritized implementation
               indirection::write( archive, std::forward< T>( value), name, traits::priority::tag< 4>{});
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

            //! take care of read that returns bool
            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name) -> std::enable_if_t<
               std::is_same< 
                  decltype( detail::read( archive, std::forward< T>( value), name)), 
                  bool
               >::value, bool>
            {
               return detail::read( archive, std::forward< T>( value), name);
            }

            //! take care of read that returns void -> and allways return true
            template< typename A, typename T>
            auto read( A& archive, T&& value, const char* name) -> std::enable_if_t<
               ! std::is_same< 
                  decltype( detail::read( archive, std::forward< T>( value), name)), 
                  bool
               >::value, bool>
            {
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

         namespace traits
         {
            namespace has
            {
               namespace value
               {
                  namespace detail
                  {
                     template< typename A, typename V>
                     using serialize = decltype( common::serialize::value::serialize( std::declval< A&>(), std::declval< traits::remove_cvref_t< V>&>()));
                  } // detail
                  
                  template< typename A, typename V>
                  using serialize = common::traits::detect::is_detected< detail::serialize, A, V>;
               } // value
            } // has
         } // traits

         namespace detail::has::value
         {
            template< typename A, typename V>
            using serialize = decltype( common::serialize::value::serialize( std::declval< A&>(), std::declval< traits::remove_cvref_t< V>&>()));

            template< typename A, typename V>
            inline constexpr bool serialize_v = common::traits::detect::is_detected_v< serialize, A, V>;
            
         } // detail::has::value


         //! Specialization for serializable
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< detail::has::value::serialize_v< A, T>>>
         {
            template< typename V> 
            static void write( A& archive, V&& value, const char* name)
            {
               archive.composite_start( name);
               value::serialize( archive, std::forward< V>( value));
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

         //! Specialization for enum
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< std::is_enum_v< T>>>
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

               template< typename A>
               using has_tuple_start = decltype( std::declval< A&>().tuple_start( platform::size::type{}, ""));

               template< typename A>
               using has_tuple_end = decltype( std::declval< A&>().tuple_end( ""));

               template< typename A>
               auto start( A& archive, platform::size::type size, const char* name)
               {
                  if constexpr( common::traits::detect::is_detected_v< has_tuple_start, A>)
                     return archive.tuple_start( size, name);
                  else
                     return archive.container_start( size, name);
               }

               template< typename A>
               auto end( A& archive, const char* name)
               {
                  if constexpr( common::traits::detect::is_detected_v< has_tuple_end, A>)
                     return archive.tuple_end( name);
                  else
                     return archive.container_end( name);
               }

            } // tuple
         } // detail

         //! Specialization for tuple
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< common::traits::is::tuple_v< T>>>
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
            namespace is
            {
               template< typename T, typename C = common::traits::remove_cvref_t< T>> 
               inline constexpr auto container_v = common::traits::is::container::like_v< C>
                  && ! common::traits::is::string::like_v< C>
                  && ! serialize::traits::is::pod_v< C>;
            } // is
   
            namespace container
            {
               template< typename T>
               struct value { using type = T;};

               template< typename K, typename V>
               struct value< std::pair< K, V>> { using type = std::pair< traits::remove_cvref_t< K>, V>;};

               template< typename T> 
               using value_t = typename value< traits::remove_cvref_t< T>>::type;

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
                     if constexpr( common::traits::is::container::associative::like_v< traits::remove_cvref_t< C>>)
                     {
                        auto count = size;

                        while( count-- > 0)
                        {
                           // we need to get rid of const key (if pair), so we can serialize
                           container::value_t< typename traits::remove_cvref_t< C>::value_type> element;
                           serialize::value::read( archive, element, nullptr);

                           container.insert( std::move( element));
                        }
                     }
                     else
                     {
                        static_assert( common::traits::is::container::sequence::like_v< traits::remove_cvref_t< C>>);
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
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< detail::is::container_v< T>>>
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
         struct Value< T, A, std::enable_if_t< 
            common::traits::is::container::array::like_v< T>
            && common::traits::is::binary::like_v< T>
         >>
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
               void write( A& archive, V&& value, const char* name)
               {
                  if constexpr( traits::archive::type_v< A> == archive::Type::static_need_named)
                     write_named( archive, value, name);
                  else if constexpr( traits::archive::type_v< A> == archive::Type::static_order_type)
                     write_order_type( archive, value);
                  else if constexpr( traits::archive::type_v< A> == archive::Type::dynamic_type)
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
                  bool not_empty = false;
                  archive.read( not_empty, nullptr);

                  if( not_empty)
                  {
                     std::decay_t< decltype( value.value())> contained;
                     value::read( archive, contained, nullptr);
                     value = std::move( contained);
                  }

                  return not_empty;
               }

               template< typename A, typename V>
               auto read( A& archive, V& value, const char* name)
               {
                  if constexpr( traits::archive::type_v< A> == archive::Type::static_need_named)
                     return read_named( archive, value, name);
                  else if constexpr( traits::archive::type_v< A> == archive::Type::static_order_type)
                     return read_order_type( archive, value);
                  else if constexpr( traits::archive::type_v< A> == archive::Type::dynamic_type)
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
         struct Value< T, A, std::enable_if_t< 
            common::traits::is::optional_like_v< T>
            && ! traits::has::serialize_v< T, A>
         >>
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
            //! @todo: Remove usage of string::utf8 when using C++20
            static auto write( A& archive, const std::filesystem::path& path, const char* name)
            {
               const auto data = path.u8string();
               const string::immutable::utf8 wrapper{ data};
               value::write( archive, wrapper, name);
            }

            //! @todo: Remove usage of string::utf8 when using C++20
            static auto read( A& archive, std::filesystem::path& path, const char* name)
            {
               std::string data;
               string::utf8 wrapper{ data};
               if( value::read( archive, wrapper, name))
               {
                  path = std::filesystem::u8path( std::move( data));
                  return true;
               }
               return false;
            }
         };  

         //! Specialization for named value
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< traits::is::named::value_v< T>>>
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

         namespace traits
         {
            namespace has
            {
               namespace value
               {
                  namespace detail
                  {
                     template< typename A, typename V>
                     using write = decltype(  common::serialize::value::write( std::declval< A&>(), std::declval< traits::remove_cvref_t< const V>&>(), nullptr));
                  } // detail
                  
                  template< typename A, typename V>
                  using write = common::traits::detect::is_detected< detail::write, A, V>;
               } // value
            } // has
         } // traits

      } // serialize
   } // common
} // casual