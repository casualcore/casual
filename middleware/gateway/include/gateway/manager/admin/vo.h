//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
#include "common/platform.h"

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
                     enum class Bound : short
                     {
                        out,
                        in,
                        unknown,
                     };

                     enum class Runlevel : short
                     {
                        absent,
                        connecting,
                        online,
                        shutdown,
                        error
                     };

                     struct Address
                     {
                        std::string local;
                        std::string peer;
                        
                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( local);
                           CASUAL_SERIALIZE( peer);
                        })
                     };


                     Bound bound = Bound::unknown;
                     Runlevel runlevel = Runlevel::absent;

                     common::process::Handle process;
                     common::domain::Identity remote;
                     Address address;



                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( bound);
                        CASUAL_SERIALIZE( runlevel);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( remote);
                        CASUAL_SERIALIZE( address);
                     })

                     friend bool operator < ( const Connection& lhs, const Connection& rhs)
                     {
                        return std::make_tuple( lhs.remote, lhs.bound, lhs.process)
                             < std::make_tuple( rhs.remote, rhs.bound, rhs.process);
                     }

                  };

                  struct Listener
                  {
                     struct Limit 
                     {
                        common::platform::binary::size::type size = 0;
                        common::platform::binary::size::type messages = 0;
                        
                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( size);
                           CASUAL_SERIALIZE( messages);
                        })
                     };

                     struct Address
                     {
                        std::string host;
                        std::string port;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( host);
                           CASUAL_SERIALIZE( port);
                        })
                     };

                     Limit limit;
                     Address address;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( limit);
                        CASUAL_SERIALIZE( address);
                     })

                  };


                  struct State
                  {
                     std::vector< Connection> connections;
                     std::vector< Listener> listeners;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( connections);
                        CASUAL_SERIALIZE( listeners);
                     })

                  };

               } // v1_0
            } // vo
         } // admin
      } // manager
   } // gateway
} // casual


