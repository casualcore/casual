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
   global:
      note: "'domain global' config. Aggregates right to left"
      service:
         note: Will be used for services that are not explicitly configured. 
         execution:
            timeout:
               duration: 2h
               contract: linger
   default:
      note: "'default', fallback, configuration. Will only affect 'local' configuration and will not aggregate 'between' configurations"
      server:
         instances: 1
         restart: true
         memberships: [ customer-group]
         environment:
            variables:
               -  key: SOME_VARIABLE
                  value: foo
      executable:
         instances: 1
         restart: false
      service:
         execution:
            timeout:
               duration: 20s
         visibility: discoverable

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

   services:
      -  name: a
         routes: [ b, c]
         execution:
            timeout:
               duration: 64ms
               contract: abort
         visibility: undiscoverable
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
      -  name: implementation/detail/service
         visibility: undiscoverable
         note: "service 'implementation/detail/service' is not discoverable from other domains"
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
            note: "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groups/A.qb"
            queues:
               -  name: a1
                  enable:
                     enqueue: true
                     dequeue: false
                  note: dequeue is disabled -> dequeue will give no_queue error (unless the queue is found elsewhere)
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
         -  alias: D
            capacity:
               size: "10KiB"
            note: group limited to a total message size of 10KiB, after which further enqueues will give no_queue error
            queues:
               -  name: d1
               -  name: d2

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
                  -  alias: fwd-b1
                     source: b1
                     target: 
                        service: casual/example/echo
                     instances: 4
                     reply: 
                        queue: a3
                        delay: 10ms
               queues:
                  -  alias: fwd-c1
                     source: c1
                     target:
                        queue: a4
                     note: gets the alias 'c1'
            -  alias: forward-group-2
               services:
                  -  alias: bar
                     source: b2
                     target:
                        service: casual/example/echo
                     note: will get alias b2
                  -  source: b2
                     target:
                        service: casual/example/sink
                     note: will get alias b2.2
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
            -  alias: in-A
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

            -  alias: in-B
               limit:
                  size: 10485760
                  messages: 10
               note: threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume
               connections:
                  -  address: some.host.org:7780
                  -  address: some.host.org:4242

            -  note: (generated alias) listeners - no limits
               connections:
                  -  address: some.host.org:4242
        

      outbound:
         groups: 
            -  alias: out-A
               order: 1
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

            -  alias: out-B
               order: 1
               note: has the same order as out-A group -> services found in out-A and out-B will be load balanced.
               connections:
                  -  address: a47.domain.host.org:7779

            -  alias: fallback
               order: 10
               connections:
                  -  address: a99.domain.host.org:7780
                     note: will be chosen if _resources_ are not found at connections in the out-A or out-B group

      reverse:
         inbound:
            groups:
               -  alias: in-C
                  note: connect to other reverse outbound that is listening on this port - then treat it as a regular inbound
                  limit:
                     messages: 42
                  connections:
                     -  note: one of possible many addresses to connect to
                        address: localhost:7780

         outbound:
            groups:
               -  alias: out-C
                  order: 1
                  note: listen for connection from reverse inbound - then treat it as a regular outbound
                  connections:
                     -  note: one of possible many listening addresses.
                        address: localhost:7780

               -  alias: out-D
                  order: 2
                  note: another instance (process) that handles (multiplexed) traffic on it's own
                  connections:
                     -  note: one of possible many listening addresses.
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

