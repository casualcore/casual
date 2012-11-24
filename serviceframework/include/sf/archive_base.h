/*
 * casual_archivewriter.h
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#ifndef CASUAL_SF_ARCHIVEBASE_H_
#define CASUAL_SF_ARCHIVEBASE_H_

#include "sf/namevaluepair.h"
#include "sf/archive_traits.h"

#include "common/types.h"


#include <utility>


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

            void handleStart( const char* name);

            void handleEnd( const char* name);

            std::size_t handleContainerStart( std::size_t size);

            void handleContainerEnd();

            void handleSerialtypeStart();

            void handleSerialtypeEnd();

         private:

            virtual void handle_start( const char* name) = 0;

            virtual void handle_end( const char* name) = 0;

            virtual std::size_t handle_container_start( std::size_t size) = 0;

            virtual void handle_container_end() = 0;

            virtual void handle_serialtype_start() = 0;

            virtual void handle_serialtype_end() = 0;

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

            virtual void readPOD( bool& value) = 0;

            virtual void readPOD( char& value) = 0;

            virtual void readPOD( short& value) = 0;

            virtual void readPOD( long& value) = 0;

            virtual void readPOD( float& value) = 0;

            virtual void readPOD( double& value) = 0;

            virtual void readPOD( std::string& value) = 0;

            virtual void readPOD( common::binary_type& value) = 0;

         public:
            void read( bool& value);

            //void write (const signed char& value);

            void read( char& value);

            //void write (const unsigned char& value);

            void read( short& value);

            //void write (const unsigned short& value);

            void read( int& value);

            //void write (const unsigned int& value);

            void read( long& value);

            void read( unsigned long& value);

            void read( float& value);

            void read( double& value);

            void read( std::string& value);

            void read( common::binary_type& value);

         };


         template< typename T, typename RV>
         Reader& operator >>( Reader& archive, const NameValuePair< T, RV>&& nameValuePair);


         template< typename T>
         typename std::enable_if< traits::is_pod< T>::value, void>::type
         serialize( Reader& archive, T& value)
         {
            archive.read( value);
         }


         template< typename T>
         typename std::enable_if< traits::is_serializible< T>::value, void>::type
         serialize( Reader& archive, T& value)
         {
            archive.handleSerialtypeStart();

            value.serialize( archive);

            archive.handleSerialtypeEnd();
         }


         template< typename K, typename V>
         void serialize( Reader& archive, std::pair< K, V>& value)
         {
            typedef typename std::remove_const< K>::type key_type;

            archive.handleSerialtypeStart();

            archive >> makeNameValuePair( "key", const_cast< key_type&>( value.first));
            archive >> makeNameValuePair( "value", value.second);

            archive.handleSerialtypeEnd();
         }



         template< typename T>
         typename std::enable_if< traits::is_sequence_container< T >::value, void>::type
         serialize( Reader& archive, T& container)
         {
            container.resize( archive.handleContainerStart( 0));

            for( auto& element : container)
            {
               archive >> CASUAL_MAKE_NVP( element);
            }

            archive.handleContainerEnd();
         }



         template< typename T>
         typename std::enable_if< traits::is_associative_container< T >::value, void>::type
         serialize( Reader& archive, T& container)
         {
            std::size_t size = archive.handleContainerStart( 0);

            for( std::size_t index = 0; index < size; ++index)
            {
               typename T::value_type element;
               archive >> CASUAL_MAKE_NVP( element);

               container.insert( std::move( element));
            }

            archive.handleContainerEnd();
         }



         template< typename T>
         inline void serialize( Reader& archive, const char* name, T& value)
         {
            archive.handleStart( name);

            serialize( archive, value);

            archive.handleEnd( name);
         }

         template< typename T>
         Reader& operator &( Reader& archive, T&& nameValuePair)
         {
            return operator >> ( archive, std::forward< T>( nameValuePair));
         }


         template< typename T, typename RV>
         Reader& operator >>( Reader& archive, const NameValuePair< T, RV>&& nameValuePair)
         {
            serialize( archive, nameValuePair.getName(), nameValuePair.getValue());

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

            virtual void writePOD( const bool value) = 0;

            virtual void writePOD( const char value) = 0;

            virtual void writePOD( const short value) = 0;

            virtual void writePOD( const long value) = 0;

            virtual void writePOD( const float value) = 0;

            virtual void writePOD( const double value) = 0;

            virtual void writePOD( const std::string& value) = 0;

            virtual void writePOD( const common::binary_type& value) = 0;

         public:
            void write( const bool value);

            //void write (const signed char& value);

            void write( const char value);

            //void write (const unsigned char& value);

            void write( const short value);

            //void write (const unsigned short& value);

            void write( const int value);

            //void write (const unsigned int& value);

            void write( const long value);

            void write (const unsigned long value);

            void write( const float value);

            void write( const double value);

            void write( const std::string& value);

            void write( const common::binary_type& value);

         };

         template< typename T, typename RV>
         Writer& operator <<( Writer& archive, const NameValuePair< T, RV>&& nameValuePair);


         template< typename T>
         typename std::enable_if< traits::is_pod< T>::value, void>::type
         serialize( Writer& archive, const T& value)
         {
            archive.write( value);
         }


         template< typename T>
         typename std::enable_if< traits::is_serializible< T>::value, void>::type
         serialize( Writer& archive, const T& value)
         {
            archive.handleSerialtypeStart();

            // TODO: Can we get rid of const-cast?
            const_cast< T&>( value).serialize( archive);

            archive.handleSerialtypeEnd();
         }

         template< typename K, typename V>
         void serialize( Writer& archive, const std::pair< K, V>& value)
         {
            archive.handleSerialtypeStart();

            archive << makeNameValuePair( "key", value.first);
            archive << makeNameValuePair( "value", value.second);

            archive.handleSerialtypeEnd();
         }


         template< typename T>
         typename std::enable_if< traits::is_container< T >::value, void>::type
         serialize( Writer& archive, const T& container)
         {
            archive.handleContainerStart( container.size());

            for( auto element : container)
            {
               archive << CASUAL_MAKE_NVP( element);
            }

            archive.handleContainerEnd();
         }



         template< typename T>
         inline void serialize( Writer& archive, const char* name, const T& value)
         {
            archive.handleStart( name);

            serialize( archive, value);

            archive.handleEnd( name);
         }

         template< typename T>
         Writer& operator &( Writer& archive, T&& nameValuePair)
         {
            return operator << ( archive, std::forward< T>( nameValuePair));
         }


         template< typename T, typename RV>
         Writer& operator <<( Writer& archive, const NameValuePair< T, RV>&& nameValuePair)
         {
            serialize( archive, nameValuePair.getName(), nameValuePair.getConstValue());

            return archive;
         }

      }
   }

}

#endif /* CASUAL_ARCHIVEWRITER_H_ */
