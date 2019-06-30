//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/persistent.h"

#include "domain/common.h"

#include "common/serialize/macro.h"
#include "common/serialize/create.h"

#include "common/environment.h"
#include "common/exception/handle.h"



namespace casual
{
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
                           CASUAL_SERIALIZE( major);
                           CASUAL_SERIALIZE( minor);
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
                           CASUAL_SERIALIZE( version);
                           CASUAL_SERIALIZE_NAME( state.get(), "state");
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

                  //auto persistent = local::persistent( state);

                  common::file::Output file{ name};
                  auto archive = common::serialize::create::writer::from( file.extension(), file);
                  
                  archive << CASUAL_NAMED_VALUE( state);
               }

               State load( const std::string& name)
               {
                  Trace trace{ "domain::manager::persistent::state::load"};

                  State state;
                  //auto persistent = local::persistent( state);

                  common::file::Input file{ name};
                  auto archive = common::serialize::create::reader::relaxed::from( file.extension(), file);
                  
                  archive >> CASUAL_NAMED_VALUE( state);

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
