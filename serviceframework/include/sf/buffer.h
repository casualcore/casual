//!
//! buffer.h
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
//!

#ifndef BUFFER_H_
#define BUFFER_H_

#include <memory>

#include <string>

namespace casual
{
   namespace sf
   {
      namespace buffer
      {
         struct Type
         {
            Type( const std::string& type_name, const std::string& subtype_name)
             : name( type_name), subname( subtype_name) {}

            Type() = default;
            Type( Type&&) = default;


            bool operator < ( const Type& rhs) const
            {
               if( name == rhs.name)
                  return subname < rhs.subname;

               return name < rhs.name;
            }

            std::string name;
            std::string subname;

         };

         Type type( const char* buffer);

         //!
         //! Holds the buffer and its size together. Has no resource responsibility
         //!
         struct Raw
         {
            Raw( char* p_buffer, std::size_t p_size);

            char* buffer;
            std::size_t size;
         };

         class Base
         {
         public:


            Base( Base&&) = default;
            Base& operator = ( Base&&) = default;

            virtual ~Base();

            Base( const Base&) = delete;

            //!
            //! @return the 'raw buffer'
            //!
            Raw raw();
            std::size_t size() const;

            //!
            //! Releases the responsibility for the resource.
            //!
            //! @attention No other member function is callable after.
            //!
            Raw release();

            Type type() const;

         private:

            struct xatmi_deleter
            {
               xatmi_deleter() = default;
               xatmi_deleter( xatmi_deleter&&) = default;

               void operator () ( char* xatmiBuffer) const;
            };
         protected:

            Base( Raw buffer);
            Base( Type&& type, std::size_t size);

            void expand( std::size_t expansion);

            std::unique_ptr< char, xatmi_deleter> m_buffer;
            std::size_t m_size;
         };


         namespace source
         {

            //!
            //! todo should be google-proto-buffer, or something similar later on.
            //!
            class Binary : public Base
            {
            public:
               //Binary();
               Binary( Base&&);

               Binary( Binary&&) = default;
               Binary& operator = ( Binary&&) = default;

               template< typename T>
               void read( T& value)
               {
                  Raw raw = Base::raw();
                  if( m_offset + sizeof( T) > raw.size)
                  {
                     //TODO: throw
                  }
                  memcpy( &value, raw.buffer + m_offset, sizeof( T));
                  m_offset += sizeof( T);
               }

            private:
               std::size_t m_offset = 0;

            };
         }

         namespace target
         {
            class Binary : public Base
            {
            public:
               Binary();

               Binary( Binary&&) = default;
               Binary& operator = ( Binary&&) = default;

               template< typename T>
               void write( T&& value)
               {
                  if( m_offset + sizeof( T) > size())
                  {
                     expand( sizeof( T));
                  }

                  Raw raw = Base::raw();

                  memcpy( raw.buffer + m_offset, &value, sizeof( T));
                  m_offset += sizeof( T);
               }
            private:
               std::size_t m_offset;
            };
         }


         class X_Octet : public Base
         {
         public:
            X_Octet( const std::string& subtype);
            X_Octet( const std::string& subtype, std::size_t size);

            X_Octet( Raw buffer);

            std::string str() const;
            void str( const std::string& new_string);

         };


      } // buffer
   } // sf
} // casual




#endif /* BUFFER_H_ */
