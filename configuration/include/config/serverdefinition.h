//!
//! servicedefinition.h
//!
//! Created on: May 10, 2014
//!     Author: Lazan
//!

#ifndef SERVICEDEFINITION_H_
#define SERVICEDEFINITION_H_

#include "sf/namevaluepair.h"

#include <algorithm>
#include <string>
#include <vector>

namespace casual
{

   namespace config
   {
      namespace build
      {
         namespace server
         {
            struct Server
            {
               struct Service
               {
                  std::string name;
                  std::string function;
                  std::string type;

                  //!
                  //! Can be 'adopt' or 'auto' - default is 'auto'
                  //!
                  std::string transaction;

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( function);
                     archive & CASUAL_MAKE_NVP( type);
                     archive & CASUAL_MAKE_NVP( transaction);
                  }

               };

               //!
               //! Default for all services
               //!
               std::string type;
               std::string transaction;

               std::vector< Service> services;

               template< typename A>
               void serialize( A& archive)
               {
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( transaction);
                  archive & CASUAL_MAKE_NVP( services);
               }


            };

            Server get( const std::string& file);

         } // server
      } // build

   } // config


} // casual

#endif // SERVICEDEFINITION_H_
