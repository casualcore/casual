//!
//! casual 
//!

#include "configuration/domain.h"


#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"

#include <fstream>


namespace casual
{
   namespace configuration
   {
      namespace example
      {
         domain::Manager domain()
         {
            domain::Manager domain;

            domain.name = "domain.A42";

            {
               domain.manager_default.environment.variables = {
                     { []( environment::Variable& v){
                        v.key = "SOME_VARIABLE";
                        v.value = "42";
                     }},
                     { []( environment::Variable& v){
                        v.key = "SOME_OTHER_VARIABLE";
                        v.value = "some value";
                     }}
               };
               domain.manager_default.environment.files = {
                     { "/some/path/to/environment/file"},
                     { "/some/other/file"}
               };

               domain.manager_default.service.timeout.emplace( "90s");
               domain.manager_default.server.restart.emplace( true);
            }

            {
               domain.transaction.manager_default.resource.instances.emplace( 3);
               domain.transaction.manager_default.resource.key.emplace( "db2_rm");

               domain.transaction.log = "/some/fast/disk/domain.A42/transaction.log";

               domain.transaction.resources = {
                     {
                           []( transaction::Resource& r){
                              r.name = "customer-db";
                              r.openinfo.emplace( "db=customer,uid=db2,pwd=db2");
                              r.instances.emplace( 5);
                              r.note = "this resource is named 'customer-db' - using the default rm-key (db_rm) - overrides the default rm-instances to 5";
                           }
                     },
                     {
                           []( transaction::Resource& r){
                              r.name = "sales-db";
                              r.openinfo.emplace( "db=sales,uid=db2,pwd=db2");
                              r.note = "this resource is named 'sales-db' - using the default rm-key (db_rm) - using default rm-instances";
                           }
                     },
                     {
                           []( transaction::Resource& r){
                              r.name = "event-queue";
                              r.key.emplace( "mq_rm");
                              r.openinfo.emplace( "some-mq-specific-stuff");
                              r.closeinfo.emplace( "some-mq-specific-stuff");
                              r.note = "this resource is named 'event-queue' - overrides rm-key - using default rm-instances";
                           }
                     }
               };
            }

            {
               domain.groups = {
                     {
                           []( group::Group& v){
                              v.name = "common-group";
                              v.note = "group that logically groups 'common' stuff";
                           }
                     },
                     {
                           []( group::Group& v){
                              v.name = "customer-group";
                              v.note = "group that logically groups 'customer' stuff";
                              v.resources.emplace( { std::string{ "customer-db"}});
                              v.dependencies.emplace( { std::string{ "common-group"}});
                           }
                     },
                     {
                            []( group::Group& v){
                               v.name = "sales-group";
                               v.note = "group that logically groups 'customer' stuff";
                               v.resources.emplace( { std::string{ "sales-db"}, std::string{ "event-queue"}});
                               v.dependencies.emplace( { std::string{ "customer-group"}});
                            }
                      },

               };

               domain.servers = {
                     {
                           []( server::Server& v){
                              v.path = "customer-server-1";
                              v.memberships.emplace( { std::string{ "customer-group"}});
                           }
                     },
                     {
                           []( server::Server& v){
                              v.path = "customer-server-2";
                              v.memberships.emplace( { std::string{ "customer-group"}});
                           }
                     },
                     {
                           []( server::Server& v){
                              v.path = "sales-server";
                              v.alias.emplace( "sales-pre");
                              v.instances.emplace( 10);
                              v.memberships.emplace( { std::string{ "sales-group"}});
                              v.restrictions.emplace( { std::string{ "preSalesSaveService"}, std::string{ "preSalesGetService"}});
                              v.note.emplace( "the only services that will be advertised are 'preSalesSaveService' and 'preSalesGetService'");
                           }
                     },
                     {
                           []( server::Server& v){
                              v.path = "sales-server";
                              v.alias.emplace( "sales-post");
                              v.memberships.emplace( { std::string{ "sales-group"}});
                              v.restrictions.emplace( { std::string{ "postSalesSaveService"}, std::string{ "postSalesGetService"}});
                              v.note.emplace( "the only services that will be advertised are 'postSalesSaveService' and 'postSalesGetService'");
                           }
                     },
                     {
                           []( server::Server& v){
                              v.path = "sales-broker";
                              v.memberships.emplace( { std::string{ "sales-group"}});
                              v.resources.emplace( { std::string{ "event-queue"}});
                              v.environment.emplace( []( Environment& e){
                                 e.variables.emplace_back( []( environment::Variable& v){
                                    v.key = "SALES_BROKER_VARIABLE";
                                    v.value = "556";
                                 });
                              });
                           }
                     },
               };

               domain.executables = {
                     {
                           []( server::Executable& v){
                              v.path = "mq-server";
                              v.memberships.emplace( { std::string{ "common-group"}});
                              v.arguments.emplace( { std::string{ "--configuration"}, std::string{ "/path/to/configuration"}});
                           }
                     }
               };


               domain.services = {
                     { []( service::Service& v){
                        v.name = "postSalesSaveService";
                        v.timeout.emplace( "2h");
                        v.routes.emplace( { std::string( "postSalesSaveService"), std::string( "sales/post/save")});
                     }},
                     { []( service::Service& v){
                        v.name = "postSalesGetService";
                        v.timeout.emplace( "130ms");
                     }},
               };
            }

            {
               domain.gateway.listeners = {
                     {
                           []( gateway::Listener& v){
                              v.address = "localhost:7779";
                              v.note = "local host - if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below";

                              gateway::listener::Limit limit;
                              limit.size = 2 * 1024 * 1024;
                              v.limit = limit;

                           }
                     },
                     {
                           []( gateway::Listener& v){
                              v.address = "some.host.org:7779";
                              v.note = "listener that is bound to some 'external ip' - limit to max 200 calls 'in flight'";
                              gateway::listener::Limit limit;
                              limit.messages = 200;
                              v.limit = limit;
                           }
                     },
                     {
                           []( gateway::Listener& v){
                              v.address = "some.host.org:9999";
                              v.note = "listener - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume";
                              gateway::listener::Limit limit;
                              limit.messages = 10;
                              limit.size = 10 * 1024 * 1024;
                              v.limit = limit;
                           }
                     },
                     {
                           []( gateway::Listener& v){
                              v.address = "some.host.org:4242";
                              v.note = "listener - no limits";
                           }
                     },
               };

               domain.gateway.connections ={
                     {
                           []( gateway::Connection& v){
                              v.address.emplace( "a45.domain.host.org:7779");
                              v.note = "connection to domain 'a45' - we expect to find service 's1' and 's2' there.";
                              v.services = { "s1", "s2"};

                           }
                     },
                     {
                           []( gateway::Connection& v){
                              v.address.emplace( "a46.domain.host.org:7779");
                              v.note = "connection to domain 'a46' - we expect to find queues 'q1' and 'q2' and service 's1' - casual will only route 's1' to a46 if it's not accessible in a45 (or local)";
                              v.queues = { "q1", "q2"};
                              v.services = { "s1"};
                           }
                     },
                     {
                           []( gateway::Connection& v){
                              v.address.emplace( "a99.domain.host.org:7780");
                              v.note = "connection to domain 'a99' - if the connection is closed from a99, casual will not try to reestablish the connection";
                              v.restart = false;

                           }
                     },
               };
            }


            {

               domain.queue.manager_default.queue.retries.emplace( 0);

               domain.queue.groups = {
                     { []( queue::Group& v){
                        v.name = "groupA";
                        v.note = "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb";
                        v.queues = {
                              { []( queue::Queue& v){
                                 v.name = "q_A1";
                              }},
                              { []( queue::Queue& v){
                                 v.name = "q_A2";
                                 v.retries.emplace( 10);
                                 v.note = "after 10 rollbacked dequeues, message is moved to q_A2.error";
                              }},
                              { []( queue::Queue& v){
                                 v.name = "q_A3";
                              }},
                              { []( queue::Queue& v){
                                 v.name = "q_A4";
                              }},
                        };
                     }},
                     { []( queue::Group& v){
                        v.name = "groupB";
                        v.queuebase.emplace( "/some/fast/disk/queue/groupB.qb");
                        v.queues = {
                              { []( queue::Queue& v){
                                 v.name = "q_B1";
                              }},
                              { []( queue::Queue& v){
                                 v.name = "q_B2";
                                 v.retries.emplace( 10);
                                 v.note = "after 10 rollbacked dequeues, message is moved to q_B2.error";
                              }},
                        };
                     }},
                     { []( queue::Group& v){
                        v.name = "groupC";
                        v.queuebase.emplace( ":memory:");
                        v.note = "group is an in-memory queue, hence no persistence";
                        v.queues = {
                              { []( queue::Queue& v){
                                 v.name = "q_C1";
                              }},
                              { []( queue::Queue& v){
                                 v.name = "q_C2";
                              }},
                        };
                     }},
               };


            }


            return domain;
         }

         void write( const domain::Manager& domain, const std::string& name)
         {
            std::ofstream file{ name};
            auto archive = sf::archive::writer::from::name( file, common::file::name::extension( name));
            archive << CASUAL_MAKE_NVP( domain);
         }

         common::file::scoped::Path temporary( const configuration::domain::Manager& domain, const std::string& extension)
         {
            common::file::scoped::Path file{ common::file::name::unique( common::directory::temporary() + "/domain_", extension)};

            write( domain, file);

            return file;
         }

      } // example
   } // configuration
} // casual

