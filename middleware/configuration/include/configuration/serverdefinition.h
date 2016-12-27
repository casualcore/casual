//!
//! casual
//!

#ifndef SERVICEDEFINITION_H_
#define SERVICEDEFINITION_H_

#include "sf/namevaluepair.h"

#include <algorithm>
#include <string>
#include <vector>

namespace casual
{

   namespace configuration
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
                  //! Can be:
                  //! - 'auto' Join current transaction, or start a new one if there is no current.
                  //! - 'join' Join current transaction if there is one.
                  //! - 'atomic' Always start a new transaction.
                  //! - 'none' Don't start or join any transaction
                  //!
                  //! default is 'auto'
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
               std::string transaction = "auto";

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
