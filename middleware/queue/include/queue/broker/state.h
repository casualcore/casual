//!
//! casual
//!

#ifndef CASUAL_QUEUE_BROKER_STATE_H_
#define CASUAL_QUEUE_BROKER_STATE_H_


#include "common/communication/ipc.h"
#include "common/message/queue.h"
#include "common/message/gateway.h"
#include "common/domain.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace casual
{
   namespace queue
   {
      namespace broker
      {
         struct State
         {

            State();

            struct Group
            {

               using id_type = common::process::Handle;

               Group();
               Group( std::string name, id_type process);

               std::string name;
               id_type process;

               std::string queuebase;

               bool connected = false;

               friend bool operator == ( const Group& lhs, id_type process);

            };

            //!
            //! Represent a remote gateway that exports 0..* queues
            //!
            struct Gateway
            {
               Gateway();
               Gateway( common::domain::Identity id, common::process::Handle process);

               common::domain::Identity id;
               common::process::Handle process;
               std::size_t order = 0;

               friend bool operator == ( const Gateway& lhs, const common::domain::Identity& rhs);

            };

            struct Queue
            {
               Queue() = default;
               Queue( common::process::Handle process, std::size_t queue, std::size_t order = 0)
                  : process{ std::move( process)}, queue{ queue}, order{ order} {}

               common::process::Handle process;
               std::size_t queue = 0;
               std::size_t order = 0;

               friend bool operator < ( const Queue& lhs, const Queue& rhs);
               friend std::ostream& operator << ( std::ostream& out, const Queue& value);
            };


            std::vector< common::platform::pid::type> processes() const;

            std::unordered_map< std::string, std::vector< Queue>> queues;

            std::deque< common::message::queue::lookup::Request> pending;


            std::string configuration;

            std::vector< Group> groups;
            std::vector< Gateway> gateways;

            std::string group_executable;

            //!
            //! Removes all queues associated with the process
            //!
            //! @param pid process id
            //!
            void remove_queues( common::platform::pid::type pid);

            //!
            //! Removes the process (group/gateway) and all queues associated with the process
            //!
            //! @param pid process id
            //!
            void remove( common::platform::pid::type pid);

            void update( common::message::gateway::domain::Advertise& message);

         };

      } // broker
   } // queue
} // casual

#endif // CASUAL_QUEUE_BROKER_STATE_H_
