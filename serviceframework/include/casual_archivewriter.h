/*
 * casual_archivewriter.h
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#ifndef CASUAL_ARCHIVEWRITER_H_
#define CASUAL_ARCHIVEWRITER_H_

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

            virtual void writePOD( const std::wstring& value) = 0;

            virtual void writePOD( const std::vector< char>& value) = 0;

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

            void write( const std::wstring& value);

            void write( const std::vector< char>& value);

         };


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
            archive << makeNameValuePair( "key", value.first);
            archive << makeNameValuePair( "value", value.second);
         }


         template< typename T>
         typename std::enable_if< traits::is_container< T >::value, void>::type
         serialize( Writer& archive, const T& container)
         {
            archive.handleContainerStart();

            archive << makeNameValuePair( "size", container.size());

            for( auto element : container)
            {
               archive << makeNameValuePair( "element", element);
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
