//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/domain.h"
#include "configuration/example/create.h"
#include "configuration/common.h"

#include "common/file.h"
#include "common/serialize/macro.h"
#include "common/serialize/create.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         user::Domain domain()
         {
            Trace trace{ "configuration::example::domain"};

            static constexpr auto yaml = R"(
domain:
  name: domain.A42
  default:
    environment:
#      files:
#        - /some/path/to/environment/file
#        - /some/other/file

      variables:
        - key: SOME_VARIABLE
          value: 42

        - key: SOME_OTHER_VARIABLE
          value: some value

    server:
      instances: 1
      restart: true

    executable:
      instances: 1
    service:
      timeout: 90s


  transaction:
    default:
      resource:
        key: db2_rm
        instances: 3

    log: /some/fast/disk/domain.A42/transaction.log
    resources:
      - name: customer-db
        instances: 5
        note: this resource is named 'customer-db' - using the default rm-key (db_rm) - overrides the default rm-instances to 5
        openinfo: db=customer,uid=db2,pwd=db2

      - name: sales-db
        note: this resource is named 'sales-db' - using the default rm-key (db_rm) - using default rm-instances
        openinfo: db=sales,uid=db2,pwd=db2

      - name: event-queue
        key: mq_rm
        note: this resource is named 'event-queue' - overrides rm-key - using default rm-instances
        openinfo: some-mq-specific-stuff
        closeinfo: some-mq-specific-stuff

  groups:
    - name: common-group
      note: group that logically groups 'common' stuff

    - name: customer-group
      note: group that logically groups 'customer' stuff
      resources:
        - customer-db

      dependencies:
        - common-group

    - name: sales-group
      note: group that logically groups 'customer' stuff
      resources:
        - sales-db
        - event-queue

      dependencies:
        - customer-group

  servers:
    - path: customer-server-1
      memberships:
        - customer-group

    - path: customer-server-2
      memberships:
        - customer-group

    - path: sales-server
      alias: sales-pre
      note: the only services that will be advertised are 'preSalesSaveService' and 'preSalesGetService'
      instances: 10
      memberships:
        - sales-group

      restrictions:
        - preSalesSaveService
        - preSalesGetService

    - path: sales-server
      alias: sales-post
      note: the only services that will be advertised are 'postSalesSaveService' and 'postSalesGetService'
      memberships:
        - sales-group

      restrictions:
        - postSalesSaveService
        - postSalesGetService

    - path: sales-broker
      memberships:
        - sales-group

      environment:
        variables:
          - key: SALES_BROKER_VARIABLE
            value: 556

      resources:
        - event-queue

  executables:
    - path: mq-server
      arguments:
        - --configuration
        - /path/to/configuration

      memberships:
        - common-group

  services:
    - name: postSalesSaveService
      timeout: 2h
      routes:
        - postSalesSaveService
        - sales/post/save

    - name: postSalesGetService
      timeout: 130ms

  gateway:
    default:
      connection:
        restart: true

    inbounds: 
      - alias: unique-inbound-alias
        limit:
           size: 2097152
        note: if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below

        connections: 
          - address: localhost:7779
            note: can be several listening host:port per inbound instance
          - address: some.host.org:7779

      - limit:
           size: 10485760
           messages: 10
        note: (generated alias) listeners - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume

        connections:
          - address: some.host.org:7780
          - address: some.host.org:4242

      - note: (generated alias) listeners - no limits
        connections:
           - address: some.host.org:4242
        

    outbounds: 
      - alias: primary
        connections:
         - address: a45.domain.host.org:7779
           note: connection to domain 'a45' - we expect to find service 's1' and 's2' there.
           services:
             - s1
             - s2
         - address: a46.domain.host.org:7779
           note: connection to domain 'a46' - we expect to find queues 'q1' and 'q2' and service 's1' - casual will only route 's1' to a46 if it's not accessible in a45 (or local)
           services:
             - s1
           queues:
             - q1
             - q2

      - alias: fallback
        connections:
           - address: a99.domain.host.org:7780
             note: will be chosen if _resources_ are not found at connections in the 'primary' outbound

    reverse:
      inbounds:
         - alias: unique-alias-name
           note: connect to other reverse outbound that is listening on this port - then treat it as a regular inbound
           limit:
              messages: 42
           connections:
              - note: one of possible many addresses to connect to
                address: localhost:7780

      outbounds:
         - alias: primary
           note: listen for connection from reverse inbound - then treat it as a regular outbound
           connections:
            - note: one of possible many listining addresses.
              address: localhost:7780

         - alias: secondary
           note: onther instance (proces) that handles (multiplexed) traffic on it's own
           connections:
            - note: one of possible many listining addresses.
              address: localhost:7781

)";

            auto domain = create::model< configuration::user::Domain>( yaml, "domain");
            domain.queue = example::queue();

            return domain;
         }

         configuration::user::queue::Manager queue()
         {
            Trace trace{ "configuration::example::queue"};

            static constexpr auto yaml = R"(
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
      -  name: C
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

)";

            return create::model< configuration::user::queue::Manager>( yaml, "queue");
         }

      } // example
   } // configuration
} // casual

