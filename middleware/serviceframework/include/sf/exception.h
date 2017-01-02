//!
//! casual
//!

#ifndef CASUAL_SF_EXCEPTION_H_
#define CASUAL_SF_EXCEPTION_H_


#include "common/exception.h"

#include <string>
#include <stdexcept>

#include <ostream>

namespace casual
{
   namespace sf
   {
      namespace exception
      {
         class Base : public common::exception::base
         {
         public:
            using common::exception::base::base;

         protected:
            ~Base() = default;
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

         namespace invalid
         {
            struct File : Base
            {
               using Base::Base;
            };


         } // invalid

         namespace memory
         {
            struct Allocation : public Base
            {
               using Base::Base;
            };

         } // memory

         namespace archive
         {
            namespace invalid
            {
               struct Document : Base
               {
                  using Base::Base;
               };

               struct Node : Base
               {
                  using Base::Base;
               };

            } // invalid
         } // archive

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
