//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

#include "common/domain.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace admin
         {

            namespace vo
            {
               inline namespace v1_0
               {
                  struct Connection
                  {
                     enum class Bound : char
                     {
                        out,
                        in,
                        unknown,
                     };

                     enum class Type : char
                     {
                        unknown,
                        ipc,
                        tcp
                     };

                     enum class Runlevel : short
                     {
                        absent,
                        connecting,
                        online,
                        shutdown,
                        error
                     };


                     Bound bound = Bound::unknown;
                     Type type = Type::unknown;
                     Runlevel runlevel = Runlevel::absent;


                     common::process::Handle process;
                     common::domain::Identity remote;
                     std::vector< std::string> address;



                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( bound);
                        archive & CASUAL_MAKE_NVP( type);
                        archive & CASUAL_MAKE_NVP( runlevel);
                        archive & CASUAL_MAKE_NVP( process);
                        archive & CASUAL_MAKE_NVP( remote);
                        archive & CASUAL_MAKE_NVP( address);
                     })

                     friend bool operator < ( const Connection& lhs, const Connection& rhs)
                     {
                        return std::make_tuple( lhs.remote, lhs.bound, lhs.type, lhs.process)
                             < std::make_tuple( rhs.remote, rhs.bound, rhs.type, rhs.process);
                     }

                  };


                  struct State
                  {
                     std::vector< Connection> connections;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( connections);
                     })

                  };

               } // v1_0
            } // vo
         } // admin
      } // manager
   } // gateway
} // casual


