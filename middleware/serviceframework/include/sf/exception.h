//!
//! casual
//!

#ifndef CASUAL_SF_EXCEPTION_H_
#define CASUAL_SF_EXCEPTION_H_


#include "common/exception/casual.h"

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

         using Validation =  common::exception::casual::base< common::code::casual::validation>;

         namespace archive
         {
            namespace invalid
            {
               using Document = common::exception::casual::base< common::code::casual::invalid_document>;

               using Node = common::exception::casual::base< common::code::casual::invalid_node>;

            } // invalid
         } // archive

      } // exception
   } // sf
} // casual



#endif /* CASUAL_SF_EXCEPTION_H_ */
