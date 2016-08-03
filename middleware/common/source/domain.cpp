//!
//! casual 
//!

#include "common/domain.h"

#include "common/environment.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace domain
      {
         Identity::Identity() = default;

         Identity::Identity( const Uuid& id, std::string name)
            : id{ id}, name{ std::move( name)}
         {

         }

         Identity::Identity( std::string name)
            : Identity{ uuid::make(), std::move( name)}
         {

         }


         std::ostream& operator << ( std::ostream& out, const Identity& value)
         {
            return out << "{ id: " << value.id << ", name: " << value.name << "}";
         }

         bool operator < ( const Identity& lhs, const Identity& rhs)
         {
            return lhs.id < rhs.id;
         }



         namespace local
         {
            namespace
            {

               Identity& identity()
               {
                  static Identity id{
                     environment::variable::get( environment::variable::name::domain::id(), "00000000000000000000000000000000"),
                     environment::variable::get( environment::variable::name::domain::name(), "")};
                  return id;

               }
            }
         }

         const Identity& identity()
         {
            return local::identity();
         }

         void identity( Identity value)
         {
            local::identity() = value;
            if( ! local::identity().id)
            {
               local::identity().id = uuid::make();
            }
            environment::variable::set( environment::variable::name::domain::name(), local::identity().name);
            environment::variable::set( environment::variable::name::domain::id(), uuid::string( local::identity().id));

         }

      } // domain
   } // common
} // casual
