//!
//! casual
//!

#ifndef CASUAL_SF_ARCHIVE_MAKER_H_
#define CASUAL_SF_ARCHIVE_MAKER_H_


#include <memory>
#include <string>
#include <iosfwd>

#include "sf/archive/archive.h"


namespace casual
{
   namespace sf
   {
      namespace archive
      {

         namespace maker
         {

            template<typename A>
            class Interface
            {
            public:
               typedef A archive_type;

               virtual ~Interface() = default;
               virtual archive_type& archive() = 0;
               virtual void serialize() = 0;
            };


            template<typename T>
            class Holder
            {
            public:
               typedef std::unique_ptr<T> base_type;

               Holder( base_type&& base) : m_base( std::move( base)) {}
               Holder( Holder&& rhs) = default;
               ~Holder() = default;

            protected:
               base_type m_base;
            };

         } // maker


         namespace reader
         {
            using Interface = maker::Interface<Reader>;

            using Base = maker::Holder<Interface>;

            class Holder : public Base
            {
            public:

               using Base::Base;

               template< typename T>
               Holder& operator >> ( T&& value)
               {
                  m_base->serialize();
                  m_base->archive() >> std::forward< T>( value);
                  return *this;
               }

               template< typename T>
               Holder& operator & ( T&& value)
               {
                  return Holder::operator >> ( std::forward< T>( value));
               }
            };

            namespace from
            {
               //! @{
               Holder data();
               Holder data( std::istream& stream);
               //! @}


               //! @{
               Holder file( std::string name);
               //! @}

               //! @{
               Holder name( std::string name);
               Holder name( std::istream& stream, std::string name);
               //! @}

               Holder buffer( const platform::binary::type& data, std::string type);



            } // from

         } // reader


         namespace writer
         {
            using Interface = maker::Interface<sf::archive::Writer>;
            using Base = maker::Holder<Interface>;

            class Holder : public Base
            {
            public:

               using Base::Base;



               template< typename T>
               Holder& operator << ( T&& value)
               {
                  m_base->archive() << std::forward< T>( value);
                  m_base->serialize();
                  return *this;
               }

               template< typename T>
               Holder& operator & ( T&& value)
               {
                  return Holder::operator << ( std::forward< T>( value));
               }
            };

            namespace from
            {
               //! @{
               Holder file( std::string name);
               //! @}

               //! @{
               Holder name( std::string name);
               Holder name( std::ostream& stream, std::string name);
               //! @}

               Holder buffer( platform::binary::type& data, std::string type);

            } // from

         } // writer

      } // archive

   } // sf

} // casual



#endif /* CASUAL_SF_ARCHIVE_MAKER_H_ */
