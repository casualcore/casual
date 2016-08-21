//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "domain/manager/state.h"
#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"
#include "domain/manager/admin/vo.h"


#include "common/string.h"
#include "common/environment.h"
#include "common/mockup/file.h"
#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/service/lookup.h"
#include "sf/xatmi_call.h"
#include "sf/log.h"


#include <fstream>

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               namespace configuration
               {

                  std::vector< file::scoped::Path> files( const std::vector< std::string>& config)
                  {
                     return range::transform( config, []( const std::string& c){
                        return file::scoped::Path{ mockup::file::temporary( ".yaml", c)};
                     });
                  }

                  std::string names( const std::vector< file::scoped::Path>& files)
                  {
                     return string::join( files, " ");
                  }

               } // configuration

               struct Manager
               {
                  Manager( const std::vector< std::string>& config)
                   : files( configuration::files( config)),
                     process{ "./bin/casual-domain-manager", {
                        "--configuration-files", configuration::names( files),
                        "--bare",
                     }}
                  {

                  }

                  struct remove_singleton_file_t
                  {
                     remove_singleton_file_t()
                     {
                        if( file::exists( environment::domain::singleton::file()))
                        {
                           file::remove( environment::domain::singleton::file());
                        }
                     }
                  } remove_singleton_file;

                  std::vector< file::scoped::Path> files;
                  mockup::Process process;
               };

               namespace manager
               {

                  platform::ipc::id::type ipc()
                  {
                     std::ifstream file{ environment::domain::singleton::file()};

                     while( ! file.is_open())
                     {
                        process::sleep( std::chrono::milliseconds{ 10});
                        file.open( environment::domain::singleton::file());
                     }

                     platform::ipc::id::type result = 0;

                     file >> result;

                     log::debug << "manager ipc: " << result << '\n';

                     return result;
                  }

               } // manager


               namespace configuration
               {

                  std::string empty()
                  {
                     return R"(
domain: {}

)";

                  }

                  std::string echo()
                  {
                     return R"(
domain:
  executables:
    - path: echo
      instances: 4    

)";
                  }

                  std::string echo_restart()
                  {
                     return R"(
domain:
  executables:
    - path: echo
      instances: 4
      arguments: [poop]
      restart: true    

)";
                  }


                  std::string sleep()
                  {
                     return R"(
domain:
  executables:
    - path: sleep
      arguments: [100]
      instances: 4    

)";
                  }



               } // configuration

            } // <unnamed>
         } // local

         TEST( casual_domain_manager, empty_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::empty()}};
               process::ping( local::manager::ipc());

            });
         }

         TEST( casual_domain_manager, echo_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::echo()}};
               process::ping( local::manager::ipc());

            });
         }


         TEST( casual_domain_manager, echo_restart_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::echo_restart()}};
               process::ping( local::manager::ipc());

            });
         }


         TEST( casual_domain_manager, sleep_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::sleep()}};
               process::ping( local::manager::ipc());

            });
         }


         namespace local
         {
            namespace
            {
               namespace call
               {

                  namespace service
                  {
                     namespace wait
                     {
                        void online( const std::string& service)
                        {
                           auto count = 100;

                           while( count-- > 0)
                           {
                              auto reply = common::service::Lookup{ service}();

                              if( reply.process)
                              {
                                 return;
                              }
                              process::sleep( std::chrono::microseconds{ 10});
                           }
                           throw exception::xatmi::service::no::Entry{ service};
                        }
                     } // wait



                  } // service


                  admin::vo::State state()
                  {
                     service::wait::online( ".casual.domain.state");

                     sf::xatmi::service::binary::Sync service( ".casual.domain.state");

                     auto reply = service();

                     admin::vo::State serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }


                  std::vector< admin::vo::scale::Instances> scale( const std::vector< admin::vo::scale::Instances>& instances)
                  {
                     service::wait::online( ".casual.domain.scale.instances");

                     sf::xatmi::service::binary::Sync service( ".casual.domain.scale.instances");
                     service << CASUAL_MAKE_NVP( instances);

                     auto reply = service();

                     std::vector< admin::vo::scale::Instances> serviceReply;
                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }

                  std::vector< admin::vo::scale::Instances> scale( const std::string& alias, std::size_t instances)
                  {
                     return scale( { { alias, instances}});
                  }

               } // call


               namespace configuration
               {

                  std::string long_running_processes_5()
                  {
                     return R"(
domain:
  executables:
    - alias: sleep
      path: sleep
      arguments: [3600]
      instances: 5    

)";
                  }

               } // configuration

               namespace predicate
               {
                  struct Manager
                  {
                     bool operator () ( const admin::vo::Executable& value) const
                     {
                        return value.alias == "casual-domain-manager";
                     }
                  };

               } // predicate


            } // <unnamed>
         } // local


         TEST( casual_domain_manager, state_long_running_processes_5__expect_5)
         {
            common::unittest::Trace trace;

            local::Manager manager{ { local::configuration::long_running_processes_5()}};
            process::ping( local::manager::ipc());

            mockup::domain::Broker broker;


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 2);
            EXPECT_TRUE( state.executables.at( 1).instances.size() == 5) << CASUAL_MAKE_NVP( state);

         }


         TEST( casual_domain_manager, state_long_running_processes_5__scale_out_to_10___expect_10)
         {
            common::unittest::Trace trace;

            local::Manager manager{ { local::configuration::long_running_processes_5()}};
            process::ping( local::manager::ipc());

            mockup::domain::Broker broker;

            // make sure we wait for broker
            common::process::instance::fetch::handle( common::process::instance::identity::broker());

            auto instances = local::call::scale( "sleep", 10);
            EXPECT_TRUE( instances.size() == 1);


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 2);
            EXPECT_TRUE( state.executables.at( 1).configured_instances == 10) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.executables.at( 1).instances.size() == 10) << CASUAL_MAKE_NVP( state);
         }

         TEST( casual_domain_manager, long_running_processes_5__scale_in_to_0___expect_0)
         {
            common::unittest::Trace trace;

            local::Manager manager{ { local::configuration::long_running_processes_5()}};
            process::ping( local::manager::ipc());

            mockup::domain::Broker broker;

            // make sure we wait for broker
            common::process::instance::fetch::handle( common::process::instance::identity::broker());


            auto instances = local::call::scale( "sleep", 0);
            EXPECT_TRUE( instances.size() == 1);


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 2);
            EXPECT_TRUE( state.executables.at( 1).configured_instances == 0) << CASUAL_MAKE_NVP( state);

            // not deterministic how long it takes for the processes to terminate.
            // EXPECT_TRUE( state.executables.at( 0).instances.size() == 0) << CASUAL_MAKE_NVP( state);
         }


         TEST( casual_domain_manager, groups_4__with_5_executables___start_with_instances_1__scale_to_10)
         {
            common::unittest::Trace trace;

            const std::string configuration{ R"(
domain:
  groups:
    - name: groupA
    - name: groupB
    - name: groupC
    - name: groupD
  executables:
    - alias: sleepA1
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupA]    
    - alias: sleepA2
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupA]
    - alias: sleepA3
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupA]
    - alias: sleepA4
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupA]
    - alias: sleepA5
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupA]

    - alias: sleepB1
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupB]    
    - alias: sleepB2
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupB]
    - alias: sleepB3
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupB]
    - alias: sleepB4
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupB]
    - alias: sleepB5
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupB]

    - alias: sleepC1
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupC]    
    - alias: sleepC2
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupC]
    - alias: sleepC3
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupC]
    - alias: sleepC4
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupC]
    - alias: sleepC5
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupC]

    - alias: sleepD1
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupD]    
    - alias: sleepD2
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupD]
    - alias: sleepD3
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupD]
    - alias: sleepD4
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupD]
    - alias: sleepD5
      path: sleep
      arguments: [3600]
      instances: 1
      memberships: [groupD]
)"};

            local::Manager manager{ { configuration}};
            process::ping( local::manager::ipc());

            mockup::domain::Broker broker;

            // make sure we wait for broker
            common::process::instance::fetch::handle( common::process::instance::identity::broker());

            auto state = local::call::state();
            state.executables = range::trim( state.executables, range::remove_if( state.executables, local::predicate::Manager{}));


            ASSERT_TRUE( state.executables.size() == 4 * 5) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.executables.at( 1).configured_instances == 1) << CASUAL_MAKE_NVP( state);


            for( auto& instance : state.executables)
            {
               local::call::scale( instance.alias, 10);
            }

            state = local::call::state();

            for( auto executable : range::remove_if( state.executables, local::predicate::Manager{}))
            {
               EXPECT_TRUE( executable.configured_instances == 10);
               EXPECT_TRUE( executable.instances.size() == 10) << "executable.instances.size(): " << executable.instances.size();
            }
         }


      } // manager

   } // domain


} // casual
