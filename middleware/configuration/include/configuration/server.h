//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_SERVER_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_SERVER_H_

#include "configuration/environment.h"

#include "sf/namevaluepair.h"
#include "sf/platform.h"

namespace casual
{
   namespace configuration
   {
      namespace server
      {
         namespace executable
         {
            struct Default
            {
               sf::optional< std::size_t> instances;
               sf::optional< bool> restart;
               sf::optional< std::vector< std::string>> memberships;

               sf::optional< Environment> environment;


               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( instances);
                  archive & CASUAL_MAKE_NVP( restart);
                  archive & CASUAL_MAKE_NVP( memberships);
                  archive & CASUAL_MAKE_NVP( environment);
               )
            };

         } // executable

         struct Executable : executable::Default
         {
            Executable();
            Executable( std::function< void(Executable&)> foreign);

            std::string path;
            sf::optional< std::string> alias;
            sf::optional< std::string> note;


            sf::optional< std::vector< std::string>> arguments;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( alias);
               archive & CASUAL_MAKE_NVP( arguments);

               executable::Default::serialize( archive);
               archive & CASUAL_MAKE_NVP( note);
            )

            friend bool operator == ( const Executable& lhs, const Executable& rhs);

            //!
            //! Will assign any unassigned values in lhs
            //!
            friend Executable& operator += ( Executable& lhs, const executable::Default& rhs);

            friend bool operator < ( const Executable& lhs, const Executable& rhs);

         };

         struct Server : public Executable
         {

            Server();
            Server( std::function< void(Server&)> foreign);

            sf::optional< std::vector< std::string>> restrictions;
            sf::optional< std::vector< std::string>> resources;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               Executable::serialize( archive);
               archive & CASUAL_MAKE_NVP( restrictions);
               archive & CASUAL_MAKE_NVP( resources);
            )

            friend bool operator == ( const Server& lhs, const Server& rhs);
         };


      } // server

   } // configuration


} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_SERVER_H_
