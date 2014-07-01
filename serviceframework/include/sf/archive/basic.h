//!
//! casual_basic_writer.h
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BASIC_WRITER_H_
#define CASUAL_BASIC_WRITER_H_

#include "sf/archive/archive.h"

#include <utility>

namespace casual
{
   namespace sf
   {
      namespace archive
      {

         template< typename I>
         class basic_reader : public Reader
         {

         public:

            typedef I implementation_type;



            template< typename... Arguments>
            basic_reader( Arguments&&... arguments)
             : m_readerImplementation( std::forward< Arguments>( arguments)...)
               {

               }

            basic_reader( basic_reader&&) = default;

            const implementation_type& implemenation() const
            {
               return m_readerImplementation;
            }


         private:


            void handle_start( const char* name) { m_readerImplementation.handle_start( name);}

            void handle_end( const char* name) { m_readerImplementation.handle_end( name); }

            std::size_t handle_container_start( std::size_t size) { return m_readerImplementation.handle_container_start( size); }

            void handle_container_end() {m_readerImplementation.handle_container_end();}

            void handle_serialtype_start() { m_readerImplementation.handle_serialtype_start();}

            void handle_serialtype_end() {m_readerImplementation.handle_serialtype_end();}



            void readPOD( bool& value) { m_readerImplementation.read( value);}

            void readPOD( char& value) { m_readerImplementation.read( value);}

            void readPOD( short& value) { m_readerImplementation.read( value);}

            void readPOD( long& value) { m_readerImplementation.read( value);}

            void readPOD( long long& value) { m_readerImplementation.read( value);}

            void readPOD( float& value) { m_readerImplementation.read( value);}

            void readPOD ( double& value) { m_readerImplementation.read( value);}

            void readPOD ( std::string& value) { m_readerImplementation.read( value);}

            void readPOD( platform::binary_type& value) { m_readerImplementation.read( value);}

            implementation_type m_readerImplementation;
         };


         template< typename I, typename RP>
         class basic_reader_node : public Reader
         {

         public:

            typedef I implementation_type;
            typedef RP relaxed_policy;

            template< typename... Arguments>
            basic_reader_node( Arguments&&... arguments)
             : m_readerImplementation( std::forward< Arguments>( arguments)...)
               {

               }

            basic_reader_node( basic_reader_node&&) = default;

         private:


            void handle_start( const char* name) { m_name = name; m_readerImplementation.handle_start( name);}

            void handle_end( const char* name) { m_readerImplementation.handle_end( name); }

            std::size_t handle_container_start( std::size_t size)
            {
               if( m_ignoreScope == 0)
               {
                  if( m_readerImplementation.has_node( m_name))
                  {
                     return m_readerImplementation.handle_container_start( size);
                  }

                  //
                  // Throws or not. name is to provide better messages
                  //
                  m_policy.apply( m_name);
               }
               ++m_ignoreScope;
               return 0;
            }

            void handle_container_end()
            {
               if( m_ignoreScope == 0)
                  m_readerImplementation.handle_container_end();
               else
                  --m_ignoreScope;
            }

            void handle_serialtype_start()
            {
               if( m_ignoreScope == 0)
               {
                  if( m_readerImplementation.has_node( m_name))
                  {
                     m_readerImplementation.handle_serialtype_start();
                  }
                  else
                  {

                     //
                     // Throws or not. name is to provide better messages
                     //
                     m_policy.apply( m_name);
                     ++m_ignoreScope;
                  }
               }
               else
               {
                  ++m_ignoreScope;
               }
            }

            void handle_serialtype_end()
            {
               if( m_ignoreScope == 0)
                  m_readerImplementation.handle_serialtype_end();
               else
                  --m_ignoreScope;
            }

            void readPOD( bool& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD( char& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD( short& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD( long& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD( long long& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD( float& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD ( double& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD ( std::string& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            void readPOD( platform::binary_type& value) { if( m_ignoreScope == 0) if( ! m_readerImplementation.read( value)) m_policy.apply( m_name);}

            implementation_type m_readerImplementation;
            relaxed_policy m_policy;
            const char* m_name = nullptr;
            std::size_t m_ignoreScope = 0;

         };




         template< typename I>
         class basic_writer : public Writer
         {

         public:

            typedef I implementation_type;

            template< typename... Arguments>
            basic_writer( Arguments&&... arguments)
             : m_writerImplementation( std::forward< Arguments>( arguments)...)
               {

               }

            basic_writer( basic_writer&&) = default;

            const implementation_type& implementation() const
            {
               return m_writerImplementation;
            }

         private:

            void handle_start( const char* name) { m_writerImplementation.handle_start( name); }

            void handle_end( const char* name) { m_writerImplementation.handle_end( name); }

            std::size_t handle_container_start( std::size_t size) { return m_writerImplementation.handle_container_start( size); }

            void handle_container_end() { m_writerImplementation.handle_container_end();}

            void handle_serialtype_start() { m_writerImplementation.handle_serialtype_start();}

            void handle_serialtype_end() { m_writerImplementation.handle_serialtype_end();}



            void writePOD (const bool value) { m_writerImplementation.write( value);}

            void writePOD (const char value) { m_writerImplementation.write( value);}

            void writePOD (const short value) { m_writerImplementation.write( value);}

            void writePOD (const long value) { m_writerImplementation.write( value);}

            void writePOD( const long long value) { m_writerImplementation.write( value);}

            void writePOD (const float value) { m_writerImplementation.write( value);}

            void writePOD (const double value) { m_writerImplementation.write( value);}

            void writePOD (const std::string& value) { m_writerImplementation.write( value);}

            void writePOD (const platform::binary_type& value) { m_writerImplementation.write( value);}

            implementation_type m_writerImplementation;

         };


      }
   }
}



#endif /* CASUAL_BASIC_WRITER_H_ */
