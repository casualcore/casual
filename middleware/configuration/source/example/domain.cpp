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

    listeners:
      - address: localhost:7779
        limit:
          size: 2097152

        note: local host - if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below

      - address: some.host.org:7779
        limit:
          messages: 200

        note: listener that is bound to some 'external ip' - limit to max 200 calls 'in flight'

      - address: some.host.org:9999
        limit:
          size: 10485760
          messages: 10

        note: listener - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume

      - address: some.host.org:4242
        note: listener - no limits


    connections:
      - address: a45.domain.host.org:7779
        services:
          - s1
          - s2

        note: connection to domain 'a45' - we expect to find service 's1' and 's2' there.

      - address: a46.domain.host.org:7779
        services:
          - s1

        queues:
          - q1
          - q2

        note: connection to domain 'a46' - we expect to find queues 'q1' and 'q2' and service 's1' - casual will only route 's1' to a46 if it's not accessible in a45 (or local)

      - address: a99.domain.host.org:7780
        restart: false
        note: connection to domain 'a99' - if the connection is closed from a99, casual will not try to reestablish the connection

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
    queue:
      retry:
        count: 3
        delay: 20s
    directory: ${CASUAL_DOMAIN_HOME}/queue/groups

  note: > 
   retry.count - if number of rollbacks is greater, message is moved to error-queue 
   retry.delay - the amount of time before the message is available for consumption, after rollback

  groups:
    - name: groupA
      note: "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb"
      queues:
        - name: q_A1
        - name: q_A2
          retry:
            count: 10
            delay: 100ms
          note: after 10 rollbacked dequeues, message is moved to q_A2.error
        - name: q_A3
        - name: q_A4
    - name: groupB
      queuebase: /some/fast/disk/queue/groupB.qb
      queues:
        - name: q_B1
        - name: q_B2
          retry:
            count: 20
          note: after 20 rollbacked dequeues, message is moved to q_B2.error. retry.delay is 'inherited' from default, if any
    - name: groupC
      queuebase: ":memory:"
      note: group is an in-memory queue, hence no persistence
      queues:
        - name: q_C1
        - name: q_C2

  forward:
     services:
        - source: q_B1
          target: 
            service: casual/example/echo
          instances: 4
          reply: 
            queue: q_A4
            delay: 10ms

)";

            return create::model< configuration::user::queue::Manager>( yaml, "queue");
         }

      } // example
   } // configuration
} // casual

