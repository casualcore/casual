//!
//! casual
//!

#ifndef CASUAL_GATEWAY_MANAGER_ADMIN_VO_H
#define CASUAL_GATEWAY_MANAGER_ADMIN_VO_H


#include "sf/namevaluepair.h"
#include "sf/platform.h"

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
                  struct base_connection
                  {
                     enum class Type : char
                     {
                        unknown,
                        ipc,
                        tcp
                     };

                     enum class Runlevel : short
                     {
                        absent,
                        booting,
                        online,
                        shutdown,
                        error
                     };

                     common::process::Handle process;
                     common::domain::Identity remote;
                     std::vector< std::string> address;
                     Type type = Type::unknown;
                     Runlevel runlevel = Runlevel::absent;


                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( process);
                        archive & CASUAL_MAKE_NVP( remote);
                        archive & CASUAL_MAKE_NVP( address);
                        archive & CASUAL_MAKE_NVP( type);
                        archive & CASUAL_MAKE_NVP( runlevel);
                     })

                  };

                  namespace outbound
                  {
                     struct Connection : base_connection
                     {

                     };

                  } // outbound

                  namespace inbound
                  {
                     struct Connection : base_connection
                     {

                     };

                  } // outbound

                  struct State
                  {
                     struct
                     {
                        std::vector< outbound::Connection> outbound;
                        std::vector< inbound::Connection> inbound;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           archive & CASUAL_MAKE_NVP( outbound);
                           archive & CASUAL_MAKE_NVP( inbound);
                        })

                     } connections;


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

#endif // BROKERVO_H_
