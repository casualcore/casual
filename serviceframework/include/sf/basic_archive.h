//!
//! casual_basic_writer.h
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BASIC_WRITER_H_
#define CASUAL_BASIC_WRITER_H_

#include "sf/archive_base.h"

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
             : m_readerArchive( std::forward< Arguments>( arguments)...)
               {

               }


            const T& policy() const
            {
               return m_readerArchive;
            }


         private:


            void handle_start( const char* name) { m_readerArchive.handle_start( name);}

            void handle_end( const char* name) { m_readerArchive.handle_end( name); }

            std::size_t handle_container_start( std::size_t size) { return m_readerArchive.handle_container_start( size); }

            void handle_container_end() {m_readerArchive.handle_container_end();}

            void handle_serialtype_start() { m_readerArchive.handle_serialtype_start();}

            void handle_serialtype_end() {m_readerArchive.handle_serialtype_end();}



            void readPOD( bool& value) { m_readerArchive.read( value);}

            void readPOD( char& value) { m_readerArchive.read( value);}

            void readPOD( short& value) { m_readerArchive.read( value);}

            void readPOD( long& value) { m_readerArchive.read( value);}

            void readPOD( float& value) { m_readerArchive.read( value);}

            void readPOD ( double& value) { m_readerArchive.read( value);}

            void readPOD ( std::string& value) { m_readerArchive.read( value);}

            void readPOD( common::binary_type& value) { m_readerArchive.read( value);}

            T m_readerArchive;

         };

         template< typename T>
         class basic_writer : public Writer
         {

         public:

            template< typename... Arguments>
            basic_writer( Arguments&&... arguments)
             : m_writerArchive( std::forward< Arguments>( arguments)...)
               {

               }

            const T& policy() const
            {
               return m_writerArchive;
            }

         private:

            void handle_start( const char* name) { m_writerArchive.handle_start( name); }

            void handle_end( const char* name) { m_writerArchive.handle_end( name); }

            std::size_t handle_container_start( std::size_t size) { return m_writerArchive.handle_container_start( size); }

            void handle_container_end() { m_writerArchive.handle_container_end();}

            void handle_serialtype_start() { m_writerArchive.handle_serialtype_start();}

            void handle_serialtype_end() { m_writerArchive.handle_serialtype_end();}



            void writePOD (const bool value) { m_writerArchive.write( value);}

            void writePOD (const char value) { m_writerArchive.write( value);}

            void writePOD (const short value) { m_writerArchive.write( value);}

            void writePOD (const long value) { m_writerArchive.write( value);}

            void writePOD (const float value) { m_writerArchive.write( value);}

            void writePOD (const double value) { m_writerArchive.write( value);}

            void writePOD (const std::string& value) { m_writerArchive.write( value);}

            void writePOD (const common::binary_type& value) { m_writerArchive.write( value);}

            T m_writerArchive;

         };


      }
   }
}



#endif /* CASUAL_BASIC_WRITER_H_ */
