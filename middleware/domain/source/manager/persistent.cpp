//!
//! casual 
//!

#include "domain/manager/persistent.h"

#include "domain/common.h"

#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"

#include "common/environment.h"


#define CASUAL_CUSTOMIZATION_POINT_SERIALIZE( type, statement) \
void serialize( sf::archive::Writer& archive, const type& value, const char* name) \
{  \
   archive.serialtype_start( name); \
statement  \
   archive.serialtype_end( name); \
} \
void serialize( sf::archive::Reader& archive, type& value, const char* name)\
{  \
   if( archive.serialtype_start( name)) \
   {   \
   statement  \
   archive.serialtype_end( name); \
   } \
} \

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace domain
         {
            namespace configuration
            {
               namespace gateway
               {
                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Listener,
                     archive & sf::name::value::pair::make( "address", value.address);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Connection,
                     archive & sf::name::value::pair::make( "type", value.type);
                     archive & sf::name::value::pair::make( "restart", value.restart);
                     archive & sf::name::value::pair::make( "address", value.address);
                     archive & sf::name::value::pair::make( "note", value.note);
                     archive & sf::name::value::pair::make( "services", value.services);
                     archive & sf::name::value::pair::make( "queues", value.queues);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                     archive & sf::name::value::pair::make( "listeners", value.listeners);
                     archive & sf::name::value::pair::make( "connections", value.connections);

                  )

               } // gateway

               namespace transaction
               {
                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Resource,
                     archive & sf::name::value::pair::make( "name", value.name);
                     archive & sf::name::value::pair::make( "key", value.key);
                     archive & sf::name::value::pair::make( "instances", value.instances);
                     archive & sf::name::value::pair::make( "note", value.note);
                     archive & sf::name::value::pair::make( "openinfo", value.openinfo);
                     archive & sf::name::value::pair::make( "closeinfo", value.closeinfo);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                     archive & sf::name::value::pair::make( "log", value.log);
                     archive & sf::name::value::pair::make( "resources", value.resources);
                  )

               } // transaction

               namespace queue
               {

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Queue,
                        archive & sf::name::value::pair::make( "name", value.name);
                        archive & sf::name::value::pair::make( "note", value.note);
                        archive & sf::name::value::pair::make( "retries", value.retries);
                   )


                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Group,
                        archive & sf::name::value::pair::make( "name", value.name);
                        archive & sf::name::value::pair::make( "note", value.note);
                        archive & sf::name::value::pair::make( "queuebase", value.queuebase);
                        archive & sf::name::value::pair::make( "queues", value.queues);
                  )


                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                        archive & sf::name::value::pair::make( "groups", value.groups);
                  )

               } // queue

               namespace service
               {
                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Service,
                     archive & sf::name::value::pair::make( "name", value.name);
                     archive & sf::name::value::pair::make( "timeout", value.timeout);
                     archive & sf::name::value::pair::make( "routes", value.routes);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                     archive & sf::name::value::pair::make( "default_timeout", value.default_timeout);
                     archive & sf::name::value::pair::make( "services", value.services);
                  )

               } // service

               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Domain,
                  archive & sf::name::value::pair::make( "name", value.name);
                  archive & sf::name::value::pair::make( "queue", value.queue);
                  archive & sf::name::value::pair::make( "transaction", value.transaction);
                  archive & sf::name::value::pair::make( "gateway", value.gateway);
                  archive & sf::name::value::pair::make( "service", value.service);
               )

            } //configuration
         } // domain
      } // message
   } // common


   namespace domain
   {
      namespace manager
      {
         namespace persistent
         {
            namespace state
            {
               namespace local
               {
                  namespace
                  {
                     struct Version
                     {
                        int major = 0;
                        int minor = 1;

                        CASUAL_CONST_CORRECT_SERIALIZE
                        (
                           archive & CASUAL_MAKE_NVP( major);
                           archive & CASUAL_MAKE_NVP( minor);
                        )

                     };

                     template< typename S>
                     struct Persistent
                     {
                        Persistent( S& state) : state( state) {}

                        Version version;
                        std::reference_wrapper< S> state;

                        CASUAL_CONST_CORRECT_SERIALIZE
                        (
                           archive & CASUAL_MAKE_NVP( version);
                           archive & sf::name::value::pair::make( "state", state.get());
                        )
                     };

                     template< typename S>
                     auto persistent( S& s) -> Persistent< S>
                     {
                        return { s};
                     }

                  } // <unnamed>
               } // local

               //
               void save( const manager::State& state, const std::string& file)
               {
                  Trace trace{ "domain::manager::persistent::state::save"};

                  auto persistent = local::persistent( state);

                  auto archive = sf::archive::writer::from::file( file);
                  archive << CASUAL_MAKE_NVP( persistent);
               }

               State load( const std::string& file)
               {
                  Trace trace{ "domain::manager::persistent::state::load"};

                  State state;
                  auto persistent = local::persistent( state);

                  auto archive = sf::archive::reader::from::file( file);
                  archive >> CASUAL_MAKE_NVP( persistent);

                  return state;
               }

               namespace local
               {
                  namespace
                  {
                     std::string file()
                     {
                        auto file = common::coalesce( common::environment::variable::get( "CASUAL_DOMAIN_PERSISTENT_STATE", ""),
                            common::environment::directory::domain() + "/.persistent/state.json");

                        common::directory::create( common::directory::name::base( file));

                        return file;
                     }

                  } // <unnamed>
               } // local

               State load()
               {
                  Trace trace{ "domain::manager::persistent::state::load"};

                  try
                  {
                     return load( local::file());
                  }
                  catch( ...)
                  {
                     common::exception::handle();
                     common::log::category::information << "failed to locate persistent file - using default state\n";
                  }
                  return {};
               }

               void save( const manager::State& state)
               {
                  Trace trace{ "domain::manager::persistent::state::save"};

                  try
                  {
                     save( state, local::file());
                  }
                  catch( ...)
                  {
                     common::exception::handle();
                     common::log::category::information << "failed to locate persistent file during save - ignore\n";
                  }
               }


            } // state


         } // persistent

      } // manager
   } // domain


} // casual
