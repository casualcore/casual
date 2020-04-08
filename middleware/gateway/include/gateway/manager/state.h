//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "gateway/manager/listen.h"

#include "common/process.h"
#include "common/domain.h"

#include "common/communication/tcp.h"
#include "common/communication/select.h"

#include "common/message/coordinate.h"


namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         struct State;

         namespace state
         {
            using size_type = platform::size::type;
            
            struct base_connection
            {
               enum class Runlevel : short
               {
                  absent,
                  connecting,
                  online,
                  offline,
                  error
               };
               friend std::ostream& operator << ( std::ostream& out, const Runlevel& value);

               struct Address
               {
                  std::string local;
                  std::string peer;

                  CASUAL_LOG_SERIALIZE(
                  { 
                     CASUAL_SERIALIZE( local);
                     CASUAL_SERIALIZE( peer);
                  })
               };


               bool running() const;

               common::process::Handle process;
               common::domain::Identity remote;
               Address address;
               Runlevel runlevel = Runlevel::absent;

               friend bool operator == ( const base_connection& lhs, common::strong::process::id rhs);
               inline friend bool operator == ( common::strong::process::id lhs, const base_connection& rhs)
               {
                  return rhs == lhs;
               }

               CASUAL_CONST_CORRECT_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( remote);
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( runlevel);
               })
            };

            namespace inbound
            {
               struct Connection : base_connection
               {
               };


            } // inbound

            namespace outbound
            {
               struct Connection : base_connection
               {
                  //! configured services
                  std::vector< std::string> services;

                  //! configured queues
                  std::vector< std::string> queues;

                  size_type order = 0;
                  bool restart = false;

                  void reset();

                  friend inline bool operator < ( const Connection& lhs, const Connection& rhs)
                  {
                     return lhs.order < rhs.order;
                  }

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  { 
                     base_connection::serialize( archive);
                     CASUAL_SERIALIZE( services);
                     CASUAL_SERIALIZE( queues);
                     CASUAL_SERIALIZE( order);
                     CASUAL_SERIALIZE( restart);
                  })
               };

            } // outbound

            struct Connections
            {
               std::vector< outbound::Connection> outbound;
               std::vector< inbound::Connection> inbound;

               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE( outbound);
                  CASUAL_SERIALIZE( inbound);
               })
            };

            namespace coordinate
            {
               namespace outbound
               {

                  struct Policy
                  {
                     using message_type = common::message::gateway::domain::discover::accumulated::Reply;

                     void operator() ( message_type& message, common::message::gateway::domain::discover::Reply& reply);

                     // nothing to "log"
                     CASUAL_LOG_SERIALIZE()
                  };

                  using Discover = common::message::Coordinate< Policy>;

                  namespace rediscover
                  {
                     struct Tasks
                     {
                        struct Task 
                        {
                           common::Uuid correlation;
                           std::vector< common::strong::process::id> outbounds;
                           std::string description;

                           friend inline bool operator == ( const Task& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}

                           CASUAL_LOG_SERIALIZE(
                           { 
                              CASUAL_SERIALIZE( correlation);
                              CASUAL_SERIALIZE( outbounds);
                              CASUAL_SERIALIZE( description);
                           })
                        };

                        struct Result
                        {
                           struct Request
                           {
                              common::process::Handle destination; 
                              message::outbound::rediscover::Request message;

                              CASUAL_LOG_SERIALIZE(
                              { 
                                 CASUAL_SERIALIZE( destination);
                                 CASUAL_SERIALIZE( message);
                              })
                           };
                           common::Uuid correlation;
                           common::optional< Request> request;
                           std::string description;

                           CASUAL_LOG_SERIALIZE(
                           { 
                              CASUAL_SERIALIZE( correlation);
                              CASUAL_SERIALIZE( request);
                              CASUAL_SERIALIZE( description);
                           })
                        };


                        Result add( State& state, std::string description);
                        Result reply( State& state, const message::outbound::rediscover::Reply& message);

                        //! @returns tasks that got 'done' due to the removed pid
                        std::vector< Task> remove( common::strong::process::id pid);

                        inline bool empty() const { return m_tasks.empty();}
                        
                        CASUAL_LOG_SERIALIZE(
                        { 
                           CASUAL_SERIALIZE_NAME( m_tasks, "tasks");
                        })

                     private:
                        std::vector< Task> m_tasks;
                     };

                  } // rediscover

               } // outbound
            } // coordinate
         } // state

         struct State
         {
            enum class Runlevel
            {
               startup,
               online,
               shutdown
            };
            friend std::ostream& operator << ( std::ostream& out, const Runlevel& value);

            //! @return true if we have any running connections or listeners
            bool running() const;

            void add( listen::Entry entry);
            void remove( common::strong::file::descriptor::id listener);
            listen::Connection accept( common::strong::file::descriptor::id listener);

            inline const std::vector< listen::Entry>& listeners() const { return m_listeners;}
            inline const std::vector< common::strong::file::descriptor::id>& descriptors() const { return m_descriptors;}

            const state::outbound::Connection* outbound( common::strong::process::id pid);

            common::communication::select::Directive directive;
            state::Connections connections;
            
            struct
            {
               state::coordinate::outbound::Discover outbound;

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( outbound);
               })

            } discover;

            struct
            {
               state::coordinate::outbound::rediscover::Tasks tasks;

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( tasks);
               })

            } rediscover;

            Runlevel runlevel = Runlevel::startup;

            CASUAL_LOG_SERIALIZE(
            { 
               CASUAL_SERIALIZE( directive);
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( discover);
               CASUAL_SERIALIZE_NAME( m_listeners, "listeners");
               CASUAL_SERIALIZE_NAME( m_descriptors, "descriptors");
            })

         private:
            std::vector< listen::Entry> m_listeners;
            std::vector< common::strong::file::descriptor::id> m_descriptors;
         };

      } // manager
   } // gateway
} // casual


