//!
//! buffer.h
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
//!

#ifndef BUFFER_H_
#define BUFFER_H_

#include "sf/exception.h"
#include "common/types.h"

//
// std
//
#include <memory>
#include <string>

#include <cstring>


// TODO: temp
#include <iostream>


#include <xatmi.h>



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
            long size;
         };

         inline Raw raw( TPSVCINFO* serviceInfo)
         {
            return Raw( serviceInfo->data, serviceInfo->len);
         }

         class Base
         {
         public:


            Base( Base&&) = default;
            Base& operator = ( Base&&) = default;

            virtual ~Base();

            //Base( const Base&) = delete;

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

            void reset( Raw buffer);

            Type type() const;

         private:

            struct xatmi_deleter
            {
               void operator () ( char* xatmiBuffer) const;
            };

            virtual void doReset( Raw buffer);

         protected:

            Base( Raw buffer);
            Base( Type&& type, std::size_t size);



            void expand( std::size_t expansion);

            std::unique_ptr< char, xatmi_deleter> m_buffer;
            std::size_t m_size;
         };



         class Binary : public Base
         {
         public:
            Binary();
            Binary( Raw buffer) : Base( buffer) {}

            //Binary( Base&&);
            Binary( Binary&&);
            Binary& operator = ( Binary&&);

            Binary( const Binary&) = delete;

            template< typename T>
            void write( T&& value)
            {
               write( &value, sizeof( T));
            }

            void write( const common::binary_type& value)
            {
               write( value.size());
               write( value.data(), value.size());
            }

            void write( const std::string& value)
            {
               write( value.size());
               write( value.data(), value.size());
            }

            template< typename T>
            void read( T& value)
            {
               Raw raw = Base::raw();
               if( m_read_offset + static_cast< long>( sizeof( T)) > raw.size)
               {
                  throw exception::NotReallySureWhatToCallThisExcepion();
               }
               memcpy( &value, raw.buffer + m_read_offset, sizeof( T));
               m_read_offset += sizeof( T);
            }

            void read( std::string& value)
            {
               read_assign( value);
            }

            void read( common::binary_type& value)
            {
               read_assign( value);
            }

         private:

            template< typename T>
            void write( T* value, std::size_t lenght)
            {
               if( m_write_offset + lenght > size())
               {
                  expand( lenght);
               }

               Raw raw = Base::raw();

               memcpy( raw.buffer + m_write_offset, value, lenght);
               m_write_offset += lenght;
            }

            template< typename T>
            void read_assign( T& value)
            {
               decltype( value.size()) size;
               read( size);

               Raw raw = Base::raw();
               if( m_read_offset + static_cast< long>( size) > raw.size)
               {
                  throw exception::NotReallySureWhatToCallThisExcepion();
               }
               value.assign( raw.buffer + m_read_offset, raw.buffer + m_read_offset + size);
               m_read_offset += size;
            }


            long m_write_offset = 0;
            long m_read_offset = 0;
         };



         class X_Octet : public Base
         {
         public:
            X_Octet( const std::string& subtype);
            X_Octet( const std::string& subtype, std::size_t size);

            X_Octet( Raw buffer);

            std::string str() const;
            void str( const std::string& new_string);

         };

         /*
         template< typename T>
         class allocator;

         template<>
         class allocator< char>
         {
         public:
            typedef char value_type;
            typedef char* pointer;
            typedef const char* const_pointer;
            typedef char& reference;
            typedef const char& const_reference;
            typedef std::size_t size_type;
            typedef std::ptrdiff_t difference_type;

         };
         */

         //typedef std::vector< char, buffer::allocator< char>> test_buffer;



      } // buffer
   } // sf
} // casual




#endif /* BUFFER_H_ */
