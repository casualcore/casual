//!
//! casual_sf_archivebuffer.h
//!
//! Created on: Nov 19, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SF_ARCHIVEBUFFER_H_
#define CASUAL_SF_ARCHIVEBUFFER_H_

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         template< typename A, typename S>
         class Holder
         {
         public:

            typedef A archive_type;
            typedef S source_type;


            template< typename... Arguments>
            Holder( Arguments&&... arguments)
               : m_source( std::forward< Arguments>( arguments)...),
                 m_archive( m_source.archiveBuffer())
            {
            }

            /*auto buffer() const -> decltype( m_source.buffer())
            {
               return m_source.buffer();
            }
            */


            template< typename T>
            Holder& operator & ( T&& value)
            {
               m_archive & std::forward< T>( value);
            }

            template< typename T>
            Holder& operator << ( T&& value)
            {
               m_archive << std::forward< T>( value);
            }

            template< typename T>
            Holder& operator >> ( T&& value)
            {
               m_archive >> std::forward< T>( value);
            }

            //template< typename T>
            //friend Holder& operator & ( Holder& holder, T&& value);

         private:
            source_type m_source;
            archive_type m_archive;

         };


      }

   }

}



#endif /* CASUAL_SF_ARCHIVEBUFFER_H_ */
