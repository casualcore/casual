//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/invoke.h"


namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace invoke
         {
            std::ostream& operator << ( std::ostream& out, Result::Transaction value)
            {
               switch( value)
               {
                  case Result::Transaction::commit: return out << "commit";
                  case Result::Transaction::rollback: return out << "rollback";
                  default: return out << "unknown";
               }
            }

            std::ostream& operator << ( std::ostream& out, const Result& value)
            {
               return out << "{ payload: " << value.payload
                     << ", transaction: " << value.transaction
                     << ", code: " << value.code
                     << '}';
            }


         } // invoke
      } // service
   } // common
} // casual
