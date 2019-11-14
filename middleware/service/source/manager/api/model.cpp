//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/service/manager/api/model.h"

#include <ostream>
#include <cassert>

namespace casual
{
   namespace service
   {
      namespace manager
      {
         namespace api
         {
            inline namespace v1 
            {
               namespace model
               {
                  std::ostream& operator << ( std::ostream& out, Service::Transaction value)
                  {
                     using Enum = Service::Transaction;
                     switch( value)
                     {
                        case Enum::automatic: return out << "automatic";
                        case Enum::join: return out << "join";
                        case Enum::atomic: return out << "atomic";
                        case Enum::none: return out << "none";
                        case Enum::branch: return out << "branch";
                     }
                     assert( ! "unknown transaction mode");
                  }
               } // model

            } // v1
         } // api
      } // manager
   } // service
} // casual