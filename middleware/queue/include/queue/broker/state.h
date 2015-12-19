//!
//! state.h
//!
//! Created on: Aug 16, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_BROKER_STATE_H_
#define CASUAL_QUEUE_BROKER_STATE_H_


#include "common/ipc.h"
#include "common/message/queue.h"

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

            State( common::ipc::receive::Queue& receive);
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


            std::vector< common::platform::pid_type> processes() const;

            void operator() ( common::process::lifetime::Exit death);

            std::string group_executable;
            std::string configuration;

            std::vector< Group> groups;

            std::unordered_map< std::string, common::message::queue::lookup::Reply> queues;

            std::map< common::transaction::ID, std::vector< Group::id_type>> involved;




            struct Correlation
            {
               using id_type = common::process::Handle;

               Correlation( id_type caller, const common::Uuid& reply_correlation, std::vector< Group::id_type> groups);

               enum class Stage
               {
                  empty,
                  pending,
                  error,
                  replied,
               };

               struct Request
               {
                  Request();
                  Request( Group::id_type group);

                  Group::id_type group;
                  Stage stage = Stage::pending;
               };

               //!
               //! @return true if all request has been replied
               //!
               bool replied() const;

               Stage stage() const;
               void stage( const id_type& id, Stage state);


               id_type caller;

               common::Uuid reply_correlation;
               std::vector< Request> requests;

            };

            std::map< common::transaction::ID, Correlation> correlation;

            common::ipc::receive::Queue& ipc() { return receive;}

            common::ipc::receive::Queue& receive;
         };

      } // broker
   } // queue
} // casual

#endif // CASUAL_QUEUE_BROKER_STATE_H_
