//!
//! casual
//!

#ifndef CONFIGURATION_INCLUDE_CONFIG_ENVIRONMENT_H_
#define CONFIGURATION_INCLUDE_CONFIG_ENVIRONMENT_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include <string>
#include <vector>


namespace casual
{
   namespace configuration
   {
      namespace environment
      {
         struct Variable
         {
            Variable();
            Variable( std::function< void(Variable&)> foreign);

            std::string key;
            std::string value;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( value);
            }

            friend bool operator == ( const Variable& lhs, const Variable& rhs);

         };

      } // environment

      struct Environment
      {
         Environment();
         Environment( std::function< void(Environment&)> foreign);

         std::vector< std::string> files;
         std::vector< environment::Variable> variables;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( files);
            archive & CASUAL_MAKE_NVP( variables);
         }

         friend bool operator == ( const Environment& lhs, const Environment& rhs);
      };

      namespace environment
      {

         configuration::Environment get( const std::string& file);

         std::vector< Variable> fetch( configuration::Environment environment);

      } // environment


   } // config
} // casual



#endif /* CONFIGURATION_INCLUDE_CONFIG_ENVIRONMENT_H_ */
