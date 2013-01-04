//!
//! buffer.h
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
//!

#ifndef BUFFER_H_
#define BUFFER_H_

#include <memory>


namespace casual
{
   namespace sf
   {
      namespace buffer
      {
         class Base
         {
         public:


            Base( Base&&) = default;
            Base& operator = ( Base&&) = default;

            virtual ~Base();

            Base( const Base&) = delete;

            char* raw();
            std::size_t size() const;



         private:

            struct xatmi_deleter
            {
               xatmi_deleter() = default;
               xatmi_deleter( xatmi_deleter&&) = default;

               void operator () ( char* xatmiBuffer) const;
            };
         protected:

            Base( char* buffer, std::size_t size);

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
                  memcpy( &value, raw() + m_offset, sizeof( T));
                  m_offset += sizeof( T);
               }

            private:
               std::size_t m_offset;

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

                  memcpy( raw() + m_offset, &value, sizeof( T));
                  m_offset += sizeof( T);
               }
            private:
               std::size_t m_offset;
            };

         }



      }


   }

}




#endif /* BUFFER_H_ */
