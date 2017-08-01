//!
//! casual
//!

#ifndef CASUAL_SF_ARCHIVE_H_
#define CASUAL_SF_ARCHIVE_H_

#include "sf/namevaluepair.h"
#include "sf/archive/traits.h"
#include "sf/exception.h"

#include "sf/platform.h"


#include <utility>


namespace casual
{
   namespace sf
   {
      namespace archive
      {


         //!
         //! Read from archvie
         //!
         class Reader
         {

         public:

            Reader();
            virtual ~Reader();

            std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name);
            void container_end( const char* name);

            bool serialtype_start( const char* name);
            void serialtype_end( const char* name);

            bool read( bool& value, const char* name);
            bool read( char& value, const char* name);
            bool read( short& value, const char* name);
            bool read( int& value, const char* name);
            bool read( long& value, const char* name);
            bool read( unsigned long& value, const char* name);
            bool read( long long& value, const char* name);
            bool read( float& value, const char* name);
            bool read( double& value, const char* name);
            bool read( std::string& value, const char* name);
            bool read( platform::binary::type& value, const char* name);

         private:

            virtual std::tuple< platform::size::type, bool> dispatch_container_start( platform::size::type size, const char* name) = 0;
            virtual void dispatch_container_end( const char* name) = 0;

            virtual bool dispatch_serialtype_start( const char* name) = 0;
            virtual void dispatch_serialtype_end( const char* name) = 0;

            virtual bool pod( bool& value, const char* name) = 0;
            virtual bool pod( char& value, const char* name) = 0;
            virtual bool pod( short& value, const char* name) = 0;
            virtual bool pod( long& value, const char* name) = 0;
            virtual bool pod( long long& value, const char* name) = 0;
            virtual bool pod( float& value, const char* name) = 0;
            virtual bool pod( double& value, const char* name) = 0;
            virtual bool pod( std::string& value, const char* name) = 0;
            virtual bool pod( platform::binary::type& value, const char* name) = 0;

         };

         template< typename NVP>
         Reader& operator >> ( Reader& archive, NVP&& nvp);


         template< typename T>
         traits::enable_if_t< traits::is_pod< T>::value, bool>
         serialize( Reader& archive, T& value, const char* name)
         {
            return archive.read( value, name);
         }


         template< typename T>
         traits::enable_if_t< traits::has_serialize< T, Reader&>::value, bool>
         serialize( Reader& archive, T& value, const char* name)
         {
            if( archive.serialtype_start( name))
            {
               value.serialize( archive);
               archive.serialtype_end( name);
               return true;
            }
            return false;
         }

         template< typename T>
         traits::enable_if_t< std::is_enum< T >::value, bool>
         serialize( Reader& archive, T& value, const char* name)
         {
            typename std::underlying_type< T>::type enum_value;

            if( serialize( archive, enum_value, name))
            {
               value = static_cast< T>( enum_value);
               return true;
            }
            return false;
         }

         namespace detail
         {
            template< platform::size::type index>
            struct tuple_read
            {
               template< typename T>
               static void serialize( Reader& archive, T& value)
               {
                  archive >> sf::name::value::pair::make( nullptr, std::get< std::tuple_size< T>::value - index>( value));
                  tuple_read< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct tuple_read< 0>
            {
               template< typename T>
               static void serialize( Reader&, T&) {}
            };

            template< typename T>
            void serialize_tuple( Reader& archive, T& value, const char* name)
            {
               const auto expected_size = std::tuple_size< T>::value;
               const auto context = archive.container_start( expected_size, name);

               if( std::get< 1>( context))
               {
                  auto size = std::get< 0>( context);

                  if( expected_size != size)
                  {
                     throw exception::archive::invalid::Node{ string::compose( "got unexpected size: ", size, " - expected: ", expected_size)};
                  }
                  tuple_read< std::tuple_size< T>::value>::serialize( archive, value);

                  archive.container_end( name);
               }
            }
         } // detail

         template< typename... T>
         void serialize( Reader& archive, std::tuple< T...>& value, const char* name)
         {
            detail::serialize_tuple( archive, value, name);
         }

         template< typename K, typename V>
         void serialize( Reader& archive, std::pair< K, V>& value, const char* name)
         {
            detail::serialize_tuple( archive, value, name);
         }


         template< typename T>
         traits::enable_if_t< traits::container::is_sequence< T>::value && ! std::is_same< platform::binary::type, T>::value, bool>
         serialize( Reader& archive, T& container, const char* name)
         {
            auto properties = archive.container_start( 0, name);

            if( std::get< 1>( properties))
            {
               container.resize( std::get< 0>( properties));

               for( auto& element : container)
               {
                  archive >> sf::name::value::pair::make( nullptr, element);
               }
               archive.container_end( name);

               return true;
            }
            return false;
         }

         namespace detail
         {
            template< typename T>
            struct value { using type = T;};

            template< typename K, typename V>
            struct value< std::pair< K, V>> { using type = std::pair< typename std::remove_cv< K>::type, V>;};

         } // detail

         template< typename T>
         traits::enable_if_t< traits::container::is_associative< T >::value, bool>
         serialize( Reader& archive, T& container, const char* name)
         {
            auto properties = archive.container_start( 0, name);

            if( std::get< 1>( properties))
            {
               auto count = std::get< 0>( properties);

               while( count-- > 0)
               {
                  typename detail::value< typename T::value_type>::type element;
                  archive >> sf::name::value::pair::make( nullptr, element);

                  container.insert( std::move( element));
               }

               archive.container_end( name);
               return true;
            }
            return false;
         }

         template< typename T>
         bool serialize( Reader& archive, optional< T>& value, const char* name)
         {
            typename optional< T>::value_type contained;

            if( serialize( archive, contained, name))
            {
               value = contained;
               return true;
            }
            return false;
         }


         template< typename NVP>
         Reader& operator &( Reader& archive, NVP&& nvp)
         {
            return archive >> std::forward< NVP>( nvp);
         }

         template< typename NVP>
         Reader& operator >> ( Reader& archive, NVP&& nvp)
         {
            serialize( archive, nvp.value(), nvp.name());
            return archive;
         }


         //!
         //! Write to archive
         //!
         class Writer
         {

         public:

            Writer();
            virtual ~Writer();

            void container_start( platform::size::type size, const char* name);
            void container_end( const char* name);

            void serialtype_start( const char* name);
            void serialtype_end( const char* name);

            void write( const bool value, const char* name);
            void write( const char value, const char* name);
            void write( const short value, const char* name);
            void write( const int value, const char* name);
            void write( const long value, const char* name);
            void write( const unsigned long value, const char* name);
            void write( const long long value, const char* name);
            void write( const float value, const char* name);
            void write( const double value, const char* name);
            void write( const std::string& value, const char* name);
            void write( const platform::binary::type& value, const char* name);

         private:

            virtual void dispatch_container_start( platform::size::type size, const char* name) = 0;
            virtual void dispatch_container_end( const char* name) = 0;

            virtual void dispatch_serialtype_start( const char* name) = 0;
            virtual void dispatch_serialtype_end( const char* name) = 0;

            virtual void pod( const bool value, const char* name) = 0;
            virtual void pod( const char value, const char* name) = 0;
            virtual void pod( const short value, const char* name) = 0;
            virtual void pod( const long value, const char* name) = 0;
            virtual void pod( const long long value, const char* name) = 0;
            virtual void pod( const float value, const char* name) = 0;
            virtual void pod( const double value, const char* name) = 0;
            virtual void pod( const std::string& value, const char* name) = 0;
            virtual void pod( const platform::binary::type& value, const char* name) = 0;

         };

         template< typename NVP>
         Writer& operator << ( Writer& archive, NVP&& nvp);


         template< typename T>
         traits::enable_if_t< traits::is_pod< T>::value>
         serialize( Writer& archive, const T& value, const char* name)
         {
            archive.write( value, name);
         }


         template< typename T>
         traits::enable_if_t< traits::has_serialize< T, Writer&>::value || traits::has_serialize< const T, Writer&>::value>
         serialize( Writer& archive, const T& value, const char* name)
         {
            archive.serialtype_start( name);

            // TODO: Can we get rid of const-cast?
            const_cast< T&>( value).serialize( archive);

            archive.serialtype_end( name);
         }


         namespace detail
         {
            template< platform::size::type index>
            struct tuple_write
            {
               template< typename T>
               static void serialize( Writer& archive, const T& value)
               {
                  archive << sf::name::value::pair::make( nullptr, std::get< std::tuple_size< T>::value - index>( value));
                  tuple_write< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct tuple_write< 0>
            {
               template< typename T>
               static void serialize( Writer&, const T&) {}
            };

            template< typename T>
            void serialize_tuple( Writer& archive, const T& value, const char* name)
            {
               archive.container_start( std::tuple_size< T>::value, name);
               tuple_write< std::tuple_size< T>::value>::serialize( archive, value);
               archive.container_end( name);
            }
         } // detail

         template< typename... T>
         void serialize( Writer& archive, const std::tuple< T...>& value, const char* name)
         {
            detail::serialize_tuple( archive, value, name);
         }

         template< typename K, typename V>
         void serialize( Writer& archive, const std::pair< K, V>& value, const char* name)
         {
            detail::serialize_tuple( archive, value, name);
         }


         template< typename T>
         traits::enable_if_t< std::is_enum< T >::value>
         serialize( Writer& archive, const T& value, const char* name)
         {
            auto enum_value = static_cast< typename std::underlying_type< T>::type>( value);

            serialize( archive, enum_value, name);
         }


         template< typename T>
         traits::enable_if_t< traits::container::is_container< T>::value && ! std::is_same< platform::binary::type, T>::value>
         serialize( Writer& archive, const T& container, const char* name)
         {
            archive.container_start( container.size(), name);

            for( auto& element : container)
            {
               archive << sf::name::value::pair::make( nullptr, element);
            }

            archive.container_end( name);
         }


         template< typename T>
         void serialize( Writer& archive, const optional< T>& value, const char* name)
         {
            if( value)
            {
               archive << name::value::pair::make( name, value.value());
            }
         }

         template< typename NVP>
         Writer& operator & ( Writer& archive, NVP&& nvp)
         {
            return operator << ( archive, std::forward< NVP>( nvp));
         }


         template< typename NVP>
         Writer& operator << ( Writer& archive, NVP&& nvp)
         {
            serialize( archive, nvp.value(), nvp.name());
            return archive;
         }


      } // archive
   } // sf
} // casual

#endif /* CASUAL_ARCHIVEWRITER_H_ */
