//!
//! casual
//!

#ifndef CONFIGURATION_INCLUDE_CONFIG_ENVIRONMENT_H_
#define CONFIGURATION_INCLUDE_CONFIG_ENVIRONMENT_H_


#include "sf/namevaluepair.h"

#include <string>
#include <vector>


namespace casual
{
   namespace configuration
   {

      struct Environment
      {
         std::vector< std::string> files;

         struct Variable
         {
            std::string key;
            std::string value;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( value);
            }
         };

         std::vector< Variable> variables;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( files);
            archive & CASUAL_MAKE_NVP( variables);
         }

      };

      namespace environment
      {

         configuration::Environment get( const std::string& file);

         std::vector< Environment::Variable> fetch( configuration::Environment environment);

      } // environment


   } // config
} // casual



#endif /* CONFIGURATION_INCLUDE_CONFIG_ENVIRONMENT_H_ */
