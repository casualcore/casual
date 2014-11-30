//!
//! casual_sf_exception.h
//!
//! Created on: Nov 17, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SF_EXCEPTION_H_
#define CASUAL_SF_EXCEPTION_H_


#include <string>
#include <stdexcept>

#include <ostream>

namespace casual
{
   namespace sf
   {
      namespace exception
      {
         class Base : public std::exception
         {
         public:

            Base( std::string information) : m_information( std::move( information)) {}
            Base( std::string information, const char* file, decltype( __LINE__) line) : Base( std::move( information))
            {
               m_information.push_back( '\0');
               m_information.append( file);
               m_information.append( ':' + std::to_string( line));

            }

            const char* what() const noexcept
            {
               return m_information.c_str();
            }

            friend std::ostream& operator << ( std::ostream& out, const Base& exception)
            {
               return out << exception.what();
            }

         protected:
            ~Base() = default;

         private:
            std::string m_information;

         };

         struct Validation : public Base
         {
            using Base::Base;
         };




         struct NotReallySureWhatToCallThisExcepion : public Base
         {
            using Base::Base;
            NotReallySureWhatToCallThisExcepion() : Base( "NotRealllySureWhatToCallThisExcepion") {}

         };

         namespace memory
         {
            struct Allocation : public Base
            {
               using Base::Base;
            };

         } // memory

         namespace xatmi
         {
            struct Timeout : Base
            {
               using Base::Base;
            };

            struct System : Base
            {
               using Base::Base;
            };

         } // xatmi

      }

   }


}



#endif /* CASUAL_SF_EXCEPTION_H_ */
