//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/type.h"


#include <map>
#include <ostream>

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace transaction
         {
            std::ostream& operator << ( std::ostream& out, Type value)
            {
               switch( value)
               {
                  case Type::atomic: return out << "atomic";
                  case Type::join: return out << "join";
                  case Type::automatic: return out << "automatic";
                  case Type::none: return out << "none";
                  case Type::branch: return out << "branch";
               }
               return out << "unknown";
            }

            Type mode( const std::string& mode)
            {
               const static std::map< std::string, Type> mapping{
                  { "automatic", Type::automatic},
                  { "auto", Type::automatic},
                  { "join", Type::join},
                  { "atomic", Type::atomic},
                  { "none", Type::none},
                  { "branch", Type::branch},
               };
               return mapping.at( mode);
            }

            Type mode( std::uint16_t mode)
            {
               return static_cast< Type>( mode);
            }

         } // transaction

      } // service

   } // common


} // casual
