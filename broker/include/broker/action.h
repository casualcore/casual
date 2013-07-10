//!
//! action.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef ACTION_H_
#define ACTION_H_

#include "broker/configuration.h"

namespace casual
{
   namespace broker
   {
      namespace action
      {
         namespace server
         {
            void start( const configuration::Server& server);

            struct Start
            {
               void operator() ( const configuration::Server& server)
               {
                  start( server);
               }
            };
         }


         namespace server
         {
            struct Instances
            {
               std::string pathRegexp;
               std::size_t instances;

               template< typename A>
               void serialize( A& archive)
               {
                  archive & CASUAL_MAKE_NVP( pathRegexp);
                  archive & CASUAL_MAKE_NVP( instances);
               }

            };
         }



      } // action
   } // broker
} // casual



#endif /* ACTION_H_ */
