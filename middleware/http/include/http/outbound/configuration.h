//!
//! casual
//!

#ifndef MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_CONFIGURATION_H_
#define MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_CONFIGURATION_H_

#include "sf/namevaluepair.h"

#include <string>
#include <vector>
#include <functional>
#include <iosfwd>

namespace casual
{
   namespace http
   {
      namespace outbound
      {


         namespace configuration
         {
            struct Header
            {
               std::string name;
               std::string value;


               CASUAL_CONST_CORRECT_SERIALIZE(
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( value);
               )
            };

            struct Service
            {
               Service() = default;
               inline Service( std::function< void(Service&)> foriegn) { foriegn( *this);}

               std::string name;
               std::string url;

               std::vector< Header> headers;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( url);
                  archive & CASUAL_MAKE_NVP( headers);
               )
            };

            struct Default
            {
               std::vector< Header> headers;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  archive & CASUAL_MAKE_NVP( headers);
               )

               friend Default operator + ( Default lhs, Default rhs);
            };

            struct Model
            {
               Model() = default;
               inline Model( std::function< void(Model&)> foriegn) { foriegn( *this);}

               Default casual_default;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  archive & sf::name::value::pair::make( "default", casual_default);
                  archive & CASUAL_MAKE_NVP( services);
               )

               friend Model operator + ( Model lhs, Model rhs);
               friend std::ostream& operator << ( std::ostream& out, const Model& value);

            };


            Model get( const std::string& file);
            Model get( const std::vector< std::string>& files);

         } // configuration

      } // outbound

   } // http
} // casual



#endif /* MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_CONFIGURATION_H_ */
