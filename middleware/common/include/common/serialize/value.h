//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/value/customize.h"

#include "common/serialize/traits.h"
#include "common/optional.h"
#include "common/view/binary.h"

#include <system_error>

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
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 0>)
               {
                  return archive.read( std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 1>) -> 
                  decltype( traits::value_t<T, A>::read( archive, std::forward< T>( value), name))
               {
                  return traits::value_t<T, A>::read( archive, std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 2>) -> 
                  decltype( value.serialize( archive, name))
               {
                  return value.serialize( archive, name);
               }

               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 3>) -> 
                  decltype( customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name))
               {
                  return customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name);
               }  

               // highest priority
               template< typename A, typename T>
               auto read( A& archive, T&& value, const char* name, traits::priority::tag< 4>) -> 
                  decltype( customize::traits::value_t<T, A>::read( archive, std::forward< T>( value), name))
               {
                  return customize::traits::value_t<T, A>::read( archive, std::forward< T>( value), name);
               }


               // write

               // lowest priority
               template< typename A, typename T>
               void write( A& archive, T&& value, const char* name, traits::priority::tag< 0>)
               {
                  archive.write( std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 1>) -> 
                  decltype( traits::value_t<T, A>::write( archive, std::forward< T>( value), name))
               {
                  traits::value_t<T, A>::write( archive, std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 2>) -> 
                  decltype( value.serialize( archive, name))
               {
                  value.serialize( archive, name);
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 3>) -> 
                  decltype( customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name))
               {
                  customize::traits::value_t<T, A>::serialize( archive, std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name, traits::priority::tag< 4>) -> 
                  decltype( customize::traits::value_t<T, A>::write( archive, std::forward< T>( value), name))
               {
                  customize::traits::value_t<T, A>::write( archive, std::forward< T>( value), name);
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
            void write( A& archive, T&& value, const char* name)
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
                  decltype( detail::read( archive, value, name)), 
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
            auto serialize( A& archive, T&& value) -> decltype( indirection::serialize( archive, std::forward< T>( value), traits::priority::tag< 2>{}))
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


         //! Specialization for serializable
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< traits::has::value::serialize< A, T>::value>>
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
         struct Value< T, A, std::enable_if_t< std::is_enum< T>::value>>
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
            } // tuple
         } // detail

         //! Specialization for tuple
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< common::traits::is::tuple< T>::value>>
         {
            template< typename V> 
            static void write( A& archive, V&& value, const char* name)
            {
               archive.container_start( std::tuple_size< T>::value, name);
               detail::tuple::Write< std::tuple_size< T>::value>::serialize( archive, value);
               archive.container_end( name);
            }

            static bool read( A& archive, T& value, const char* name)
            {
               constexpr auto expected_size = std::tuple_size< T>::value;
               const auto context = archive.container_start( expected_size, name);

               if( std::get< 1>( context))
               {
                  auto size = std::get< 0>( context);

                  if( expected_size != size)
                     throw std::system_error{ std::make_error_code( std::errc::invalid_argument), "unexpected size"};

                  detail::tuple::Read< std::tuple_size< T>::value>::serialize( archive, value);

                  archive.container_end( name);
                  return true;
               }
               return false;
            }
         };

         namespace detail
         {
            namespace is
            {
               template< typename T> 
               using container = traits::bool_constant< 
                  common::traits::is::container::like< traits::remove_cvref_t< T>>::value
                  && ! serialize::traits::is::pod< traits::remove_cvref_t< T>>::value
               >; 
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
                  {
                     serialize::value::write( archive, element, nullptr);
                  }
                  archive.container_end( name);
               }

               template< typename A, typename C>
               auto read( A& archive, C& container, const char* name) ->
                  std::enable_if_t< common::traits::is::container::associative::like< traits::remove_cvref_t< C>>::value, bool>
               {
                  auto properties = archive.container_start( 0, name);

                  if( std::get< 1>( properties))
                  {
                     auto count = std::get< 0>( properties);

                     while( count-- > 0)
                     {
                        // we need to get rid of const key (if pair), so we can serialize
                        container::value_t< typename traits::remove_cvref_t< C>::value_type> element;
                        serialize::value::read( archive, element, nullptr);

                        container.insert( std::move( element));
                     }

                     archive.container_end( name);
                     return true;
                  }
                  return false;
               }

               template< typename A, typename C>
               auto read( A& archive, C& container, const char* name) ->
                  std::enable_if_t< common::traits::is::container::sequence::like< traits::remove_cvref_t< C>>::value, bool>
               {
                  auto properties = archive.container_start( 0, name);

                  if( std::get< 1>( properties))
                  {
                     container.resize( std::get< 0>( properties));

                     for( auto& element : container)
                        serialize::value::read( archive, element, nullptr);

                     archive.container_end( name);
                     return true;
                  }
                  return false;
               }

            } // container
            
         } // detail


         //! Specialization for containers
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< detail::is::container< T>::value>>
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
            common::traits::is::container::array::like< T>::value
            && common::traits::is::binary::like< T>::value
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
               std::enable_if_t< traits::need::named< A>::value>
               write( A& archive, V&& value, const char* name)
               {
                  if( value)
                     value::write( archive, value.value(), name);
               }
               
               template< typename A, typename V> 
               std::enable_if_t< ! traits::need::named< A>::value>
               write( A& archive, V&& value, const char*)
               {  
                  if( value)
                  {
                     archive.write( true);
                     value::write( archive, value.value(), nullptr);
                  }
                  else 
                  {
                     archive.write( false);
                  }
               }

               template< typename A, typename V>
               std::enable_if_t< traits::need::named< A>::value, bool>
               read( A& archive, V& value, const char* name)
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
               std::enable_if_t< ! traits::need::named< A>::value, bool>
               read( A& archive, V& value, const char*)
               {
                  bool not_empty = false;
                  archive.read( not_empty);

                  if( not_empty)
                  {
                     std::decay_t< decltype( value.value())> contained;
                     value::read( archive, contained, nullptr);
                     value = std::move( contained);
                  }

                  return not_empty;
               }
            } // optional
         } // detail

         //! Specialization for optional-like (that hasn't 'serialize')
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< 
            common::traits::is::optional_like< T>::value 
            && ! traits::has::serialize< T, A>::value
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
               value::write( archive, std::chrono::duration_cast< common::platform::time::serialization::unit>( value).count(), name);
            }

            static bool read( A& archive, value_type& value, const char* name)
            {
               common::platform::time::serialization::unit::rep representation;

               if( value::read( archive, representation, name))
               {
                  value = std::chrono::duration_cast< value_type>( common::platform::time::serialization::unit( representation));
                  return true;
               }
               return false;
            }
         };

         template< typename A>
         struct Value< common::platform::time::point::type, A>
         {
            static void write( A& archive, common::platform::time::point::type value, const char* name)
            {
               value::write( archive, value.time_since_epoch(), name);
            }

            static bool read( A& archive, common::platform::time::point::type& value, const char* name)
            {
               common::platform::time::serialization::unit duration;
               if( value::read( archive, duration, name))
               {
                  value = common::platform::time::point::type( std::chrono::duration_cast< common::platform::time::unit>( duration));
                  return true;
               }
               return false;
            }
         };
         //! @}

         //! Specialization for named value
         template< typename T, typename A>
         struct Value< T, A, std::enable_if_t< 
            traits::is::named::value< T>::value
         >>
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