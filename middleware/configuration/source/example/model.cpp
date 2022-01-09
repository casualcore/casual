//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/model.h"
#include "configuration/example/create.h"
#include "configuration/common.h"

#include "configuration/model/transform.h"


namespace casual
{
   namespace configuration::example
   {

      namespace user::part
      {
         configuration::user::Model system()
         {
            return create::model< configuration::user::Model>( R"(
system:
   resources:
      -  key: db2
         server: rm-proxy-db2-static
         xa_struct_name: db2xa_switch_static_std
         libraries:
            - db2
         paths:
            library:
               -  ${DB2DIR}/lib64

      -  key: rm-mockup
         server: rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
         paths:
            library:
               -  ${CASUAL_HOME}/lib

)");
         }

         namespace domain
         {
            configuration::user::Model general()
            {
               return create::model< configuration::user::Model>( R"(
domain:
   name: domain-name
   environment:
      variables:
         -  key: SOME_VARIABLE
            value: 42
         -  key: SOME_OTHER_VARIABLE
            value: some value
   default:
      server:
         instances: 1
         restart: true
      executable:
         instances: 1

   groups:
      -  name: common-group
         note: group that logically groups 'common' stuff

      -  name: customer-group
         note: group that logically groups 'customer' stuff
         resources:
            -  customer-db

         dependencies:
            -  common-group

      -  name: sales-group
         note: group that logically groups 'customer' stuff
         resources:
            -  sales-db
            -  event-queue

         dependencies:
            -  customer-group

   servers:
      -  path: /some/path/customer-server-1
         memberships:
            -  customer-group

      -  path: /some/path/customer-server-2
         memberships:
            -  customer-group

      -  path: /some/path/sales-server
         alias: sales-pre
         note: the only services that will be advertised are the ones who matches regex "preSales.*"
         instances: 10
         memberships:
            -  sales-group
         restrictions:
            -  preSales.*

      -  path: /some/path/sales-server
         alias: sales-post
         note: he only services that will be advertised are the ones who matches regex "postSales.*"
         memberships:
            -  sales-group
         restrictions:
            -  postSales.*

      -  path: /some/path/sales-broker
         memberships:
            -  sales-group
         environment:
            variables:
               -  key: SALES_BROKER_VARIABLE
                  value: 556
         resources:
            -  event-queue

   executables:
      -  path: /some/path/mq-server
         arguments:
            -  --configuration
            -  /path/to/configuration
         memberships:
            -  common-group

)");
            }

            configuration::user::Model service()
            {
               return create::model< configuration::user::Model>( R"(
domain:
   default:
      service:
         execution:
            timeout:
               duration: 90s
   services:
      -  name: postSalesSaveService
         execution:
            timeout:
               duration: 2h
         routes:
            -  postSalesSaveService
            -  sales/post/save

      -  name: postSalesGetService
         execution:
            timeout:
               duration: 130ms
)");
            }

            configuration::user::Model transaction()
            {
               return create::model< configuration::user::Model>( R"(
domain:
   transaction:
      default:
         resource:
            key: db2_rm
            instances: 3

      log: /some/fast/disk/domain.A42/transaction.log
      resources:
         -  name: customer-db
            instances: 5
            note: this resource is named 'customer-db' - using the default rm-key (db_rm) - overrides the default rm-instances to 5
            openinfo: db=customer,uid=db2,pwd=db2

         -  name: sales-db
            note: this resource is named 'sales-db' - using the default rm-key (db_rm) - using default rm-instances
            openinfo: db=sales,uid=db2,pwd=db2

         -  name: event-queue
            key: mq_rm
            note: this resource is named 'event-queue' - overrides rm-key - using default rm-instances
            openinfo: some-mq-specific-stuff
            closeinfo: some-mq-specific-stuff
)");
            }

            configuration::user::Model queue()
            {
               return create::model< configuration::user::Model>( R"(
domain:
   queue:
      default:
         directory: ${CASUAL_DOMAIN_HOME}/queue/groups
         queue:
            retry:
               count: 3
               delay: 20s
      
      note: > 
         retry.count - if number of rollbacks is greater, message is moved to error-queue 
         retry.delay - the amount of time before the message is available for consumption, after rollback

      groups:
         -  alias: A
            note: "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb"
            queues:
               -  name: a1
               -  name: a2
                  retry:
                     count: 10
                     delay: 100ms
                  note: after 10 rollbacked dequeues, message is moved to a2.error
               -  name: a3
               -  name: a4
         -  alias: B
            queuebase: /some/fast/disk/queue/groupB.qb
            queues:
            -  name: b1
            -  name: b2
               retry:
                  count: 20
               note: after 20 rollbacked dequeues, message is moved to b2.error. retry.delay is 'inherited' from default, if any
         -  alias: C
            queuebase: ":memory:"
            note: group is an in-memory queue, hence no persistence
            queues:
               -  name: c1
               -  name: c2

      forward:
         default:
            service:
               instances: 3
               reply:
                  delay: 2s
            queue:
               instances: 1
               target:
                  delay: 500ms
         groups:
            -  alias: forward-group-1
               services:
                  -  source: b1
                     target: 
                        service: casual/example/echo
                     instances: 4
                     reply: 
                        queue: a3
                        delay: 10ms
               queues:
                  -  source: c1
                     target:
                        queue: a4
            -  alias: forward-group-2
               services:
                  -  source: b2
                     target:
                        service: casual/example/echo

)");
            }

            configuration::user::Model gateway()
            {
               return create::model< configuration::user::Model>( R"(
domain:
   gateway:
      inbound:
         default:
            note: discovery forward is disabled default.
            connection:
               discovery:
                  forward: false

         groups: 
            -  alias: unique-inbound-alias
               limit:
                  size: 2097152
               note: if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below

               connections: 
                  -  address: localhost:7778
                     note: can be several listening host:port per inbound instance
                  -  address: some.host.org:7779
                     discovery:
                        forward: true
                     note: discovery will be forward to 'all' outbounds

            -  limit:
                  size: 10485760
                  messages: 10
               note: (generated alias) listeners - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume
               connections:
                  -  address: some.host.org:7780
                  -  address: some.host.org:4242
                     exclude:
                        services:
                           -  "foo.bar.*"
                        queues:
                           -  "queue[123]"

            -  note: (generated alias) listeners - no limits
               connections:
                  -  address: some.host.org:4242
        

      outbound:
         groups: 
            -  alias: primary
               note: casual will 'round-robin' between connections within a group for the same service/queue
               connections:
                  -  address: a45.domain.host.org:7779
                     note: connection to domain 'a45' - we expect to find service 's1' and 's2' there.
                     services:
                        -  s1
                        -  s2
                  -  address: a46.domain.host.org:7779
                     note: we expect to find queues 'q1' and 'q2' and service 's1'
                     services:
                        -  s1
                     queues:
                        -  q1
                        -  q2

            -  alias: fallback
               connections:
                  -  address: a99.domain.host.org:7780
                     note: will be chosen if _resources_ are not found at connections in the 'primary' outbound

      reverse:
         inbound:
            groups:
               -  alias: unique-alias-name
                  note: connect to other reverse outbound that is listening on this port - then treat it as a regular inbound
                  limit:
                     messages: 42
                  connections:
                     -  note: one of possible many addresses to connect to
                        address: localhost:7780

         outbound:
            groups:
               -  alias: primary
                  note: listen for connection from reverse inbound - then treat it as a regular outbound
                  connections:
                     -  note: one of possible many listining addresses.
                        address: localhost:7780

               -  alias: secondary
                  note: onther instance (proces) that handles (multiplexed) traffic on it's own
                  connections:
                     -  note: one of possible many listining addresses.
                        address: localhost:7781

)");
            }
         } // domain
      } // user::part

      configuration::Model model()
      {
         Trace trace{ "configuration::example::model"};

         return configuration::model::transform( user::part::system()) 
            + configuration::model::transform( user::part::domain::general())
            + configuration::model::transform( user::part::domain::service())
            + configuration::model::transform( user::part::domain::transaction())
            + configuration::model::transform( user::part::domain::queue())
            + configuration::model::transform( user::part::domain::gateway());
      }


   } // configuration::example
} // casual

