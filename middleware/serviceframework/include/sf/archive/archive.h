/*
 * casual_archivewriter.h
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#ifndef CASUAL_SF_ARCHIVE_H_
#define CASUAL_SF_ARCHIVE_H_

#include "sf/namevaluepair.h"
#include "sf/archive/traits.h"
#include "sf/exception.h"

#include "sf/platform.h"


#include <utility>

// temp
//#include <iostream>


namespace casual
{
   namespace sf
   {
      namespace archive
      {



         class Base
         {

         public:

            Base();
            virtual ~Base();

         //protected:

            std::size_t containerStart( std::size_t size, const char* name);
            void containerEnd( const char* name);

            bool serialtypeStart( const char* name);
            void serialtypeEnd( const char* name);

         private:

            virtual std::size_t container_start( std::size_t size, const char* name) = 0;
            virtual void container_end( const char* name) = 0;

            virtual bool serialtype_start( const char* name) = 0;
            virtual void serialtype_end( const char* name) = 0;

         };



         //!
         //! TODO:
         //!
         class Reader: public Base
         {

         public:

            Reader();
            virtual ~Reader();

         private:

            virtual void pod( bool& value, const char* name) = 0;
            virtual void pod( char& value, const char* name) = 0;
            virtual void pod( short& value, const char* name) = 0;
            virtual void pod( long& value, const char* name) = 0;
            virtual void pod( long long& value, const char* name) = 0;
            virtual void pod( float& value, const char* name) = 0;
            virtual void pod( double& value, const char* name) = 0;
            virtual void pod( std::string& value, const char* name) = 0;
            virtual void pod( platform::binary_type& value, const char* name) = 0;

         public:
            void read( bool& value, const char* name);
            void read( char& value, const char* name);
            void read( short& value, const char* name);
            void read( int& value, const char* name);
            void read( long& value, const char* name);
            void read( unsigned long& value, const char* name);
            void read( long long& value, const char* name);
            void read( float& value, const char* name);
            void read( double& value, const char* name);
            void read( std::string& value, const char* name);
            void read( platform::binary_type& value, const char* name);

         };


         template< typename T, typename RV>
         Reader& operator >>( Reader& archive, const NameValuePair< T, RV>&& nameValuePair);


         template< typename T>
         typename std::enable_if< traits::is_pod< T>::value, void>::type
         serialize( Reader& archive, T& value, const char* name)
         {
            archive.read( value, name);
         }


         template< typename T>
         typename std::enable_if< traits::is_serializible< T>::value, void>::type
         serialize( Reader& archive, T& value, const char* name)
         {
            if( archive.serialtypeStart( name))
            {
               value.serialize( archive);
            }
            archive.serialtypeEnd( name);
         }

         template< typename T>
         typename std::enable_if< std::is_enum< T >::value, void>::type
         serialize( Reader& archive, T& value, const char* name)
         {
            typename std::underlying_type< T>::type enum_value;

            serialize( archive, enum_value, name);

            value = static_cast< T>( enum_value);
         }

         namespace detail
         {
            template< std::size_t index>
            struct tuple_read
            {
               template< typename T>
               static void serialize( Reader& archive, T& value)
               {
                  archive >> makeNameValuePair( nullptr, std::get< std::tuple_size< T>::value - index>( value));
                  tuple_read< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct tuple_read< 0>
            {
               template< typename T>
               static void serialize( Reader& archive, T& value) {}
            };

            template< typename T>
            void serialize_tuple( Reader& archive, T& value, const char* name)
            {
               const auto expected_size = std::tuple_size< T>::value;
               const auto size = archive.containerStart( expected_size, name);
               if( expected_size != size)
               {
                  throw exception::archive::invalid::Node{ "expected tuple size: " + std::to_string( expected_size) + " got: " + std::to_string( size)};
               }

               tuple_read< std::tuple_size< T>::value>::serialize( archive, value);

               archive.containerEnd( name);
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
         typename std::enable_if< traits::is_sequence_container< T >::value, void>::type
         serialize( Reader& archive, T& container, const char* name)
         {
            container.resize( archive.containerStart( 0, name));

            for( auto& element : container)
            {
               archive >> makeNameValuePair( nullptr, element);
            }

            archive.containerEnd( name);
         }


         template< typename T>
         typename std::enable_if< traits::is_associative_map_container< T >::value, void>::type
         serialize( Reader& archive, T& container, const char* name)
         {
            auto count = archive.containerStart( 0, name);

            while( count-- > 0)
            {
               std::pair< typename T::key_type, typename T::mapped_type> element;
               archive >> makeNameValuePair( nullptr, element);

               container.insert( std::move( element));
            }

            archive.containerEnd( name);
         }


         template< typename T>
         typename std::enable_if< traits::is_associative_set_container< T >::value, void>::type
         serialize( Reader& archive, T& container, const char* name)
         {
            auto count = archive.containerStart( 0, name);

            while( count-- > 0)
            {
               typename T::value_type element;
               archive >> makeNameValuePair( nullptr, element);

               container.insert( std::move( element));
            }

            archive.containerEnd( name);
         }


         template< typename T>
         Reader& operator &( Reader& archive, T&& nameValuePair)
         {
            return operator >> ( archive, std::forward< T>( nameValuePair));
         }


         template< typename T, typename RV>
         Reader& operator >>( Reader& archive, const NameValuePair< T, RV>& nameValuePair)
         {
            serialize( archive, nameValuePair.getValue(), nameValuePair.getName());

            return archive;
         }

         template< typename T, typename RV>
         Reader& operator >>( Reader& archive, NameValuePair< T, RV>&& nameValuePair)
         {
            serialize( archive, nameValuePair.getValue(), nameValuePair.getName());

            return archive;
         }


         //!
         //! TODO:
         //!
         class Writer: public Base
         {

         public:


            Writer();
            virtual ~Writer();

         private:

            virtual void pod( const bool value, const char* name) = 0;
            virtual void pod( const char value, const char* name) = 0;
            virtual void pod( const short value, const char* name) = 0;
            virtual void pod( const long value, const char* name) = 0;
            virtual void pod( const long long value, const char* name) = 0;
            virtual void pod( const float value, const char* name) = 0;
            virtual void pod( const double value, const char* name) = 0;
            virtual void pod( const std::string& value, const char* name) = 0;
            virtual void pod( const platform::binary_type& value, const char* name) = 0;

         public:
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
            void write( const platform::binary_type& value, const char* name);

         };

         template< typename T, typename RV>
         Writer& operator <<( Writer& archive, const NameValuePair< T, RV>&& nameValuePair);


         template< typename T>
         typename std::enable_if< traits::is_pod< T>::value, void>::type
         serialize( Writer& archive, const T& value, const char* name)
         {
            archive.write( value, name);
         }


         template< typename T>
         typename std::enable_if< traits::is_serializible< T>::value, void>::type
         serialize( Writer& archive, const T& value, const char* name)
         {
            archive.serialtypeStart( name);

            // TODO: Can we get rid of const-cast?
            const_cast< T&>( value).serialize( archive);

            archive.serialtypeEnd( name);
         }


         namespace detail
         {
            template< std::size_t index>
            struct tuple_write
            {
               template< typename T>
               static void serialize( Writer& archive, const T& value)
               {
                  archive << makeNameValuePair( nullptr, std::get< std::tuple_size< T>::value - index>( value));
                  tuple_write< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct tuple_write< 0>
            {
               template< typename T>
               static void serialize( Writer& archive, const T& value) {}
            };

            template< typename T>
            void serialize_tuple( Writer& archive, const T& value, const char* name)
            {
               archive.containerStart( std::tuple_size< T>::value, name);
               tuple_write< std::tuple_size< T>::value>::serialize( archive, value);
               archive.containerEnd( name);
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
         typename std::enable_if< std::is_enum< T >::value, void>::type
         serialize( Writer& archive, const T& value, const char* name)
         {
            auto enum_value = static_cast< typename std::underlying_type< T>::type>( value);

            serialize( archive, enum_value, name);
         }


         template< typename T>
         typename std::enable_if< traits::is_container< T >::value, void>::type
         serialize( Writer& archive, const T& container, const char* name)
         {
            archive.containerStart( container.size(), name);

            for( auto& element : container)
            {
               archive << makeNameValuePair( nullptr, element);
            }

            archive.containerEnd( name);
         }



         template< typename T>
         Writer& operator & ( Writer& archive, T&& nameValuePair)
         {
            return operator << ( archive, std::forward< T>( nameValuePair));
         }


         template< typename T, typename RV>
         Writer& operator << ( Writer& archive, const NameValuePair< T, RV>& nameValuePair)
         {
            serialize( archive, nameValuePair.getConstValue(), nameValuePair.getName());

            return archive;
         }

         template< typename T, typename RV>
         Writer& operator << ( Writer& archive, NameValuePair< T, RV>&& nameValuePair)
         {
            serialize( archive, nameValuePair.getConstValue(), nameValuePair.getName());

            return archive;
         }

      } // archive
   } // sf
} // casual

#endif /* CASUAL_ARCHIVEWRITER_H_ */
