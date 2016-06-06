//!
//! casual 
//!

#include "config/gateway.h"
#include "config/file.h"

#include "common/algorithm.h"

#include "sf/log.h"
#include "sf/archive/maker.h"


namespace casual
{
   using namespace common;
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

               void validate( const gateway::Gateway& value)
               {

               }


               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = range::find( lhs, value);

                     if( found)
                     {
                        *found = std::move( value);
                     }
                     else
                     {
                        lhs.push_back( std::move( value));
                     }
                  }
               }

               template< typename G>
               Gateway& append( Gateway& lhs, G&& rhs)
               {
                  local::replace_or_add( lhs.listeners, std::move( rhs.listeners));
                  local::replace_or_add( lhs.connections, std::move( rhs.connections));

                  return lhs;
               }


            } // <unnamed>
         } // local

         bool operator == ( const Listener& lhs, const Listener& rhs)
         {
            return lhs.address == rhs.address;
         }

         bool operator == ( const Connection& lhs, const Connection& rhs)
         {
            return lhs.address == rhs.address;
         }


         Gateway& Gateway::operator += ( const Gateway& rhs)
         {
            return local::append( *this, rhs);
         }

         Gateway& Gateway::operator += ( Gateway&& rhs)
         {
            return local::append( *this, std::move( rhs));
         }

         Gateway operator + ( const Gateway& lhs, const Gateway& rhs)
         {
            auto result = lhs;
            result += rhs;
            return result;
         }

         void Gateway::finalize()
         {
            //
            // Complement with default values
            //
            local::complement::default_values( *this);

            //
            // Make sure we've got valid configuration
            //
            local::validate( *this);
         }

         Gateway get( const std::string& file)
         {
            //
            // Create the reader and deserialize configuration
            //
            auto reader = sf::archive::reader::from::file( file);

            Gateway gateway;
            reader >> CASUAL_MAKE_NVP( gateway);

            gateway.finalize();

            common::log::internal::gateway << CASUAL_MAKE_NVP( gateway);

            return gateway;

         }

         Gateway get()
         {
            return get( config::file::gateway());
         }

         namespace transform
         {
            common::message::domain::configuration::gateway::Reply gateway( const Gateway& gateway)
            {
               common::message::domain::configuration::gateway::Reply result;


               range::transform( gateway.listeners, result.listeners, []( const config::gateway::Listener& l){
                  common::message::domain::configuration::gateway::Listener result;

                  result.address = l.address;

                  return result;
               });


               range::transform( gateway.connections, result.connections, []( const config::gateway::Connection& c){
                  using result_type = common::message::domain::configuration::gateway::Connection;
                  result_type result;

                  result.name = c.name;
                  result.address = c.address;
                  result.type = c.type == "ipc" ?  result_type::Type::ipc : result_type::Type::tcp;
                  result.restart = c.restart == "true" ? true : false;
                  result.services = c.services;

                  return result;
               });

               return result;

            }

         } // transform

      } // gateway
   } // config
} // casual
