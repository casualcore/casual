/*
 * casual_archivewriter.h
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#ifndef CASUAL_ARCHIVEREADER_H_
#define CASUAL_ARCHIVEREADER_H_

#include "casual_archivebase.h"
#include "casual_namevaluepair.h"
#include "casual_archive_traits.h"


#include <utility>


namespace casual
{
   namespace sf
   {
      namespace archive
      {

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

            virtual void readPOD( std::vector< char>& value) = 0;

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

            void read( std::vector< char>& value);

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

            archive >> makeNameValuePair( "key", const_cast< key_type&>( value.first));
            archive >> makeNameValuePair( "value", value.second);
         }



         template< typename T>
         typename std::enable_if< traits::is_sequence_container< T >::value, void>::type
         serialize( Reader& archive, T& container)
         {
            archive.handleContainerStart();

            std::size_t size = 0;
            archive >> CASUAL_MAKE_NVP( size);

            container.resize( size);

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
            archive.handleContainerStart();

            std::size_t size = 0;
            archive >> CASUAL_MAKE_NVP( size);

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

      }
   }

}

#endif /* CASUAL_ARCHIVEWRITER_H_ */
