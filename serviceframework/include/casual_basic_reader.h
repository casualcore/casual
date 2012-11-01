//!
//! casual_basic_writer.h
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BASIC_READER_H_
#define CASUAL_BASIC_READER_H_

#include "casual_archivereader.h"

#include <utility>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         template< typename T>
         class basic_reader : public Reader
         {

         public:

            template< typename... Arguments>
            basic_reader( Arguments&&... arguments)
             : m_readerArcive( std::forward< Arguments>( arguments)...)
               {

               }


            const T& policy() const
            {
               return m_readerArcive;
            }


         private:


            void handle_start( const char* name)
            {
               m_readerArcive.handle_start( name);
            }

            void handle_end( const char* name)
            {
               m_readerArcive.handle_end( name);
            }

            void handle_container_start()
            {
               m_readerArcive.handle_container_start();
            }

            void handle_container_end()
            {
               m_readerArcive.handle_container_end();
            }

            void handle_serialtype_start()
            {
               m_readerArcive.handle_serialtype_start();
            }

            void handle_serialtype_end()
            {
               m_readerArcive.handle_serialtype_end();
            }



            void readPOD( bool& value)
            {
               m_readerArcive.read( value);
            }

            void readPOD( char& value)
            {
               m_readerArcive.read( value);
            }

            void readPOD( short& value)
            {
               m_readerArcive.read( value);
            }

            void readPOD( long& value)
            {
               m_readerArcive.read( value);
            }

            void readPOD( float& value)
            {
               m_readerArcive.read( value);
            }

            void readPOD ( double& value)
            {
               m_readerArcive.read( value);
            }

            void readPOD ( std::string& value)
            {
               m_readerArcive.read( value);
            }

            void readPOD( std::vector< char>& value)
            {
               m_readerArcive.read( value);
            }


            T m_readerArcive;

         };


      }
   }
}



#endif /* CASUAL_BASIC_WRITER_H_ */
