//!
//! casual_broker_configuration.h
//!
//! Created on: Nov 4, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_CONFIGURATION_H_
#define CASUAL_BROKER_CONFIGURATION_H_

#include "casual_sf_archive_base.h"

#include <limits>


namespace casual
{
   namespace broker
   {

      namespace configuration
      {
         enum
         {
            cUnset = -1
         };

         struct Limit
         {

            int min = cUnset;
            int max = cUnset;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( min);
               archive & CASUAL_MAKE_NVP( max);
            }
         };

         struct Server
         {
            std::string path;
            int instances = cUnset;
            Limit limits;
            std::string arguments;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( limits);
               archive & CASUAL_MAKE_NVP( arguments);
            }

         };


         struct Service
         {
            std::string name;
            int timeout = cUnset;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
            }
         };

      }


   }
}


#endif /* CASUAL_BROKER_CONFIGURATION_H_ */
