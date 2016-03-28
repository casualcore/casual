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

         namespace local
         {
            namespace
            {

               namespace complement
               {
                  struct Default
                  {
                     Default( const gateway::Default& casual_default)
                           : m_default( casual_default)
                     {
                     }

                     void operator ()( gateway::Connection& connection) const
                     {
                        assign_if_empty( connection.address, m_default.connection.address);
                        assign_if_empty( connection.name, m_default.connection.name);
                        assign_if_empty( connection.restart, m_default.connection.restart);
                        assign_if_empty( connection.type, m_default.connection.type);
                     }

                     void operator ()( gateway::Listener& listener) const
                     {
                        assign_if_empty( listener.address, m_default.listener.address);
                     }

                     void operator ()( gateway::Gateway& gateway) const
                     {
                        common::range::for_each( gateway.listeners, *this);
                        common::range::for_each( gateway.connections, *this);
                     }

                  private:

                     inline void assign_if_empty( std::string& value, const std::string& def) const
                     {
                        if( value.empty() || value == "~")
                           value = def;
                     }

                     const gateway::Default& m_default;
                  };

                  inline void default_values( gateway::Gateway& gateway)
                  {
                     Default defaults( gateway.casual_default);
                     defaults( gateway);
                  }

               } // complement

               void validate( const gateway::Gateway& settings)
               {

               }

            } //
         } // local


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
            local::complement::default_values( gateway);

            //
            // Make sure we've got valid configuration
            //
            local::validate( gateway);


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
