//!
//! casual 
//!

#include "configuration/transaction.h"


namespace casual
{
   namespace configuration
   {
      namespace transaction
      {

         namespace local
         {
            namespace
            {
               namespace complement
               {
                  template< typename R, typename V>
                  void default_values( R& range, V&& value)
                  {
                     for( auto& element : range) { element += value;}
                  }


                  void default_values( Manager& value)
                  {
                     default_values( value.resources, value.manager_default.resource);
                  }
               } // complement

               void validate( Manager& value)
               {

               }

            } // <unnamed>
         } // local

         Resource::Resource() = default;
         Resource::Resource( std::function< void(Resource&)> foreign) { foreign( *this);}

         bool operator == ( const Resource& lhs, const Resource& rhs)
         {
            return lhs.name == rhs.name;
         }

         Resource& operator += ( Resource& lhs, const resource::Default& rhs)
         {
            lhs.key = common::coalesce( lhs.key, rhs.key);
            lhs.instances = common::coalesce( lhs.instances, rhs.instances);

            return lhs;
         }

         Manager::Manager() : log{ "${CASUAL_DOMAIN_HOME}/transaction/log.db"}
         {
            manager_default.resource.instances.emplace( 1);
         }


         void Manager::finalize()
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
      } // transaction
   } // configuration



} // casual
