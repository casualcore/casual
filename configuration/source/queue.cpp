//!
//! queue.cpp
//!
//! Created on: Jun 30, 2014
//!     Author: Lazan
//!

#include "config/queue.h"
#include "config/file.h"

#include "sf/archive/maker.h"


namespace casual
{
   namespace config
   {
      namespace queue
      {

         Queues get( const std::string& file)
         {
            Queues queues;

            //
            // Create the reader and deserialize configuration
            //
            auto reader = sf::archive::reader::makeFromFile( file);

            reader >> CASUAL_MAKE_NVP( queues);

            //
            // Complement with default values
            //
            //local::complement::defaultValues( domain);

            //
            // Make sure we've got valid configuration
            //
            //local::validate( domain);

            return queues;

         }

         Queues get()
         {
            return get( config::file::queue());

         }

      } // queue

   } // config

} // casual
