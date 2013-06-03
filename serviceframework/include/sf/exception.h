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
            const char* what() const noexcept
            {
               return m_information.c_str();
            }
         protected:
            Base( const std::string& information) : m_information( information) {}

         private:
            std::string m_information;

         };

         struct Validation : public Base
         {
            Validation( const std::string& information) : Base( information) {}

         };

         struct NotReallySureWhatToCallThisExcepion : public Base
         {
            NotReallySureWhatToCallThisExcepion() : Base( "NotRealllySureWhatToCallThisExcepion") {}

         };

      }

   }


}



#endif /* CASUAL_SF_EXCEPTION_H_ */
