//!
//! casual
//!

#ifndef CASUAL_QUEUE_BROKER_STATE_H_
#define CASUAL_QUEUE_BROKER_STATE_H_


#include "common/communication/ipc.h"
#include "common/message/queue.h"
#include "common/message/gateway.h"
#include "common/message/domain.h"
#include "common/domain.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace casual
{
   namespace queue
   {
      namespace manager
      {
         using size_type = common::platform::size::type;

         struct State
         {

            State();

            struct Group
            {

               Group();
               Group( std::string name, common::process::Handle process);

               std::string name;
               common::process::Handle process;

               std::string queuebase;

               bool connected = false;

               friend bool operator == ( const Group& lhs, common::strong::process::id pid);

            };

            //!
            //! Represent a remote gateway that exports 0..* queues
            //!
            struct Remote
            {
               Remote();
               Remote( common::domain::Identity id, common::process::Handle process);

               common::domain::Identity id;
               common::process::Handle process;
               size_type order = 0;

               friend bool operator == ( const Remote& lhs, const common::domain::Identity& rhs);

            };

            struct Queue
            {
               Queue() = default;
               Queue( common::process::Handle process, common::strong::queue::id queue, size_type order = 0)
                  : process{ std::move( process)}, queue{ queue}, order{ order} {}

               common::process::Handle process;
               common::strong::queue::id queue;
               size_type order = 0;

               friend bool operator < ( const Queue& lhs, const Queue& rhs);
               friend std::ostream& operator << ( std::ostream& out, const Queue& value);
            };


            

            std::unordered_map< std::string, std::vector< Queue>> queues;

            std::deque< common::message::queue::lookup::Request> pending;


            std::vector< Group> groups;
            std::vector< Remote> remotes;

            std::string group_executable;


            std::vector< common::strong::process::id> processes() const;

            //!
            //! Removes all queues associated with the process
            //!
            //! @param pid process id
            //!
            void remove_queues( common::strong::process::id pid);

            //!
            //! Removes the process (group/gateway) and all queues associated with the process
            //!
            //! @param pid process id
            //!
            void remove( common::strong::process::id pid);

            void update( common::message::gateway::domain::Advertise& message);

            const common::message::domain::configuration::queue::Group* group_configuration( const std::string& name);
            common::message::domain::configuration::queue::Manager configuration;

         };

      } // manager
   } // queue
} // casual

#endif // CASUAL_QUEUE_BROKER_STATE_H_
