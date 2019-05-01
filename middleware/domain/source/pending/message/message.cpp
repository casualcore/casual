//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/pending/message/message.h"


namespace casual
{
   namespace domain
   {
      namespace pending
      {
         namespace message
         {

            std::ostream& operator << ( std::ostream& out, const Connect& rhs)
            {
               return out << "{ process: " << rhs.process
                  << '}';
            }

            std::ostream& operator << ( std::ostream& out, const Request& rhs)
            {
               return out << "{ message: " << rhs.message
                  << '}';
            }

         } // message
      } // pending
   } // domain
} // casual