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

            const char* what() const noexcept
            {
               return m_information.c_str();
            }

         protected:
            ~Base() = default;

         private:
            const std::string m_information;

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

      }

   }


}



#endif /* CASUAL_SF_EXCEPTION_H_ */
