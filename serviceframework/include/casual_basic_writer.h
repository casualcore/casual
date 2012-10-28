//!
//! casual_basic_writer.h
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BASIC_WRITER_H_
#define CASUAL_BASIC_WRITER_H_

#include "casual_archivewriter.h"

#include <utility>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         template< typename T>
         class basic_writer : public Writer
         {

         public:

            template< typename... Arguments>
            basic_writer( Arguments&&... arguments)
             : m_writerArcive( std::forward< Arguments>( arguments)...)
               {

               }


         private:


            void handle_start( const char* name)
            {
               m_writerArcive.handle_start( name);
            }

            void handle_end( const char* name)
            {
               m_writerArcive.handle_end( name);
            }

            void handle_container_start()
            {
               m_writerArcive.handle_container_start();
            }

            void handle_container_end()
            {
               m_writerArcive.handle_container_end();
            }

            void handle_serialtype_start()
            {
               m_writerArcive.handle_serialtype_start();
            }

            void handle_serialtype_end()
            {
               m_writerArcive.handle_serialtype_end();
            }



            void writePOD (const bool value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const char value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const short value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const long value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const float value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const double value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const std::string& value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const std::wstring& value)
            {
               m_writerArcive.write( value);
            }

            void writePOD (const std::vector< char>& value)
            {
               m_writerArcive.write( value);
            }


            T m_writerArcive;

         };


      }
   }
}



#endif /* CASUAL_BASIC_WRITER_H_ */
