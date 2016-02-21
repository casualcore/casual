//!
//! casual 
//!

#include "config/gateway.h"
#include "config/file.h"

#include "sf/log.h"
#include "sf/archive/maker.h"


namespace casual
{
   namespace config
   {
      namespace gateway
      {

         Gateway get( const std::string& file)
         {


            //
            // Create the reader and deserialize configuration
            //
            auto reader = sf::archive::reader::from::file( file);

            Gateway gateway;
            reader >> CASUAL_MAKE_NVP( gateway);


            //
            // Complement with default values
            //
            //local::default_values( domain);

            //
            // Make sure we've got valid configuration
            //
            //local::validate( domain);


            common::log::internal::gateway << CASUAL_MAKE_NVP( gateway);

            return gateway;

         }

         Gateway get()
         {
            return get( config::file::gateway());
         }

      } // gateway
   } // config
} // casual
