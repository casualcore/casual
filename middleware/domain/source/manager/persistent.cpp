//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/persistent.h"

#include "domain/common.h"

#include "serviceframework/namevaluepair.h"
#include "serviceframework/archive/create.h"

#include "common/environment.h"
#include "common/exception/handle.h"


#define CASUAL_CUSTOMIZATION_POINT_SERIALIZE( type, statement) \
void serialize( serviceframework::archive::Writer& archive, const type& value, const char* name) \
{  \
   archive.serialtype_start( name); \
statement  \
   archive.serialtype_end( name); \
} \
void serialize( serviceframework::archive::Reader& archive, type& value, const char* name)\
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
                     archive & serviceframework::name::value::pair::make( "address", value.address);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Connection,
                     archive & serviceframework::name::value::pair::make( "restart", value.restart);
                     archive & serviceframework::name::value::pair::make( "address", value.address);
                     archive & serviceframework::name::value::pair::make( "note", value.note);
                     archive & serviceframework::name::value::pair::make( "services", value.services);
                     archive & serviceframework::name::value::pair::make( "queues", value.queues);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                     archive & serviceframework::name::value::pair::make( "listeners", value.listeners);
                     archive & serviceframework::name::value::pair::make( "connections", value.connections);

                  )

               } // gateway

               namespace transaction
               {
                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Resource,
                     archive & serviceframework::name::value::pair::make( "name", value.name);
                     archive & serviceframework::name::value::pair::make( "key", value.key);
                     archive & serviceframework::name::value::pair::make( "instances", value.instances);
                     archive & serviceframework::name::value::pair::make( "note", value.note);
                     archive & serviceframework::name::value::pair::make( "openinfo", value.openinfo);
                     archive & serviceframework::name::value::pair::make( "closeinfo", value.closeinfo);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                     archive & serviceframework::name::value::pair::make( "log", value.log);
                     archive & serviceframework::name::value::pair::make( "resources", value.resources);
                  )

               } // transaction

               namespace queue
               {
                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Queue,
                        archive & serviceframework::name::value::pair::make( "name", value.name);
                        archive & serviceframework::name::value::pair::make( "note", value.note);
                        archive & serviceframework::name::value::pair::make( "retries", value.retries);
                   )


                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Group,
                        archive & serviceframework::name::value::pair::make( "name", value.name);
                        archive & serviceframework::name::value::pair::make( "note", value.note);
                        archive & serviceframework::name::value::pair::make( "queuebase", value.queuebase);
                        archive & serviceframework::name::value::pair::make( "queues", value.queues);
                  )


                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                        archive & serviceframework::name::value::pair::make( "groups", value.groups);
                  )

               } // queue

               namespace service
               {
                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Service,
                     archive & serviceframework::name::value::pair::make( "name", value.name);
                     archive & serviceframework::name::value::pair::make( "timeout", value.timeout);
                     archive & serviceframework::name::value::pair::make( "routes", value.routes);
                  )

                  CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Manager,
                     archive & serviceframework::name::value::pair::make( "default_timeout", value.default_timeout);
                     archive & serviceframework::name::value::pair::make( "services", value.services);
                  )

               } // service

               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( Domain,
                  archive & serviceframework::name::value::pair::make( "name", value.name);
                  archive & serviceframework::name::value::pair::make( "queue", value.queue);
                  archive & serviceframework::name::value::pair::make( "transaction", value.transaction);
                  archive & serviceframework::name::value::pair::make( "gateway", value.gateway);
                  archive & serviceframework::name::value::pair::make( "service", value.service);
               )

            } //configuration
         } // domain
      } // message
   } // common

   using namespace common;

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
                           archive & serviceframework::name::value::pair::make( "state", state.get());
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
               void save( const manager::State& state, const std::string& name)
               {
                  Trace trace{ "domain::manager::persistent::state::save"};

                  auto persistent = local::persistent( state);

                  common::file::Output file{ name};
                  auto archive = serviceframework::archive::create::writer::from( file.extension(), file);
                  
                  archive << CASUAL_MAKE_NVP( persistent);
               }

               State load( const std::string& name)
               {
                  Trace trace{ "domain::manager::persistent::state::load"};

                  State state;
                  auto persistent = local::persistent( state);

                  common::file::Input file{ name};
                  auto archive = serviceframework::archive::create::reader::relaxed::from( file.extension(), file);
                  
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
                     exception::handle();
                     log::line( log::category::information, "failed to locate persistent file - using default state");
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
                     exception::handle();
                     log::line( log::category::information, "failed to locate persistent file during save - ignore");
                  }
               }


            } // state


         } // persistent

      } // manager
   } // domain


} // casual
