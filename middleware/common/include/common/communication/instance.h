//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"

#include "common/uuid.h"
#include "common/process.h"

#include <iosfwd>

namespace casual
{
   namespace common::communication::instance
   {
      //! holds an instance id and corresponding environment variables
      struct Identity
      {
         Uuid id;
         std::string_view environment;
      };

      std::ostream& operator << ( std::ostream& out, const Identity& value);

      namespace identity
      {
         namespace service
         {
            inline const Identity manager{ 0xf58e0b181b1b48eb8bba01b3136ed82a_uuid, "CASUAL_SERVICE_MANAGER_PROCESS"};
         } // service

         namespace forward
         {
            //inline const Uuid cache{ "f17d010925644f728d432fa4a6cf5257"};
         } // forward

         namespace gateway
         {
             inline const Identity manager{ 0xb9624e2f85404480913b06e8d503fce5_uuid, "CASUAL_GATEWAY_MANAGER_PROCESS"};
         } // domain

         namespace queue
         {
            inline const Identity manager{ 0xc0c5a19dfc27465299494ad7a5c229cd_uuid, "CASUAL_QUEUE_MANAGER_PROCESS"};
         } // queue

         namespace transaction
         {
            inline const Identity manager{ 0x5ec18cd92b2e4c60a927e9b1b68537e7_uuid, "CASUAL_TRANSACTION_MANAGER_PROCESS"};
         } // transaction

      } // identity


      namespace fetch
      {
         enum class Directive : short
         {
            wait,
            direct
         };

         std::ostream& operator << ( std::ostream& out, Directive directive);

         process::Handle handle( const Uuid& identity, Directive directive = Directive::wait);

         //! Fetches the handle for a given pid
         //!
         //! @param pid
         //! @param directive if caller waits for the process to register or not
         //! @return handle to the process
         process::Handle handle( strong::process::id pid , Directive directive = Directive::wait);


      } // fetch

      //! @{ connect regular 'server' to casual local domain
      void connect( const process::Handle& process);
      void connect();
      //! @}


      //! connect with a 'whitelist' context. These processes are 
      //! 'protected' and special to casual. All managers (and "all" their
      //! children) connect this way
      namespace whitelist
      {
         //! connect a whitelisted process, that casual will "protect" more than not whitelisted.
         void connect();

         //! @{ connect 'singelton' 'manager' to casual local domain
         void connect( const instance::Identity& identity, const process::Handle& process);
         void connect( const instance::Identity& identity);
         //! @{
         
         //! connect without _environemnt state_, usefull for 'singletons' that is not known to others. 
         void connect( const Uuid& id);
         
      } // whitelist



      //! ping a server that owns the @p ipc-id
      //!
      //! @note will block
      //!
      //! @return the process handle
      process::Handle ping( strong::ipc::id ipc);

      namespace outbound
      {
         namespace detail
         { 
            struct base_connector
            {
               using blocking_policy = ipc::outbound::Connector::blocking_policy;
               using non_blocking_policy = ipc::outbound::Connector::non_blocking_policy;

               inline base_connector( process::Handle process)
                  : m_process{ std::move( process)}, m_connector( m_process.ipc),
                     m_socket( ipc::native::detail::create::domain::socket()) {}

               inline const Socket& socket() const { return m_socket;}

               inline const process::Handle& process() const { return m_process;}
               inline const ipc::Address& destination() const { return m_connector.destination();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_process, "process");
                  CASUAL_SERIALIZE_NAME( m_connector, "connector");
                  CASUAL_SERIALIZE_NAME( m_socket, "socket");
               )

            protected:
               inline void reset( process::Handle process)
               {
                  m_process = std::move( process);
                  m_connector = ipc::outbound::Connector{ m_process.ipc};
               }

               process::Handle m_process;
               ipc::outbound::Connector m_connector;
               Socket m_socket;
            };

            template< fetch::Directive directive>
            struct basic_connector : base_connector
            {
               basic_connector( instance::Identity identity);
               
               void reconnect();

               //! clear the connector
               void clear();

               CASUAL_LOG_SERIALIZE(
                  base_connector::serialize( archive);
                  CASUAL_SERIALIZE_NAME( m_identity, "identity");
               )
               
            private:
               instance::Identity m_identity;
            };

            //! Will wait until the instance is online, could block for ever.
            using Device = communication::device::Outbound< basic_connector< fetch::Directive::wait>>;

            namespace optional
            {
               //! Will fail if the instance is offline.
               using Device = communication::device::Outbound< basic_connector< fetch::Directive::direct>>;
            } // optional
         } // detail

         namespace service::manager
         {
            outbound::detail::Device& device();
         } // service::manager


         namespace transaction::manager
         {
            outbound::detail::Device& device();
         } // transaction::manager

         namespace gateway::manager
         {
            outbound::detail::Device& device();

            namespace optional
            {
               //! Can be missing. That is, this will not block
               //! until the device is found (the gateway is online)
               //!
               //! @return device to gateway-manager
               outbound::detail::optional::Device& device();
            } // optional

         } // gateway::manager

         namespace queue::manager
         {
            outbound::detail::Device& device();

            namespace optional
            {
               //! Can be missing. That is, this will not block
               //! until the device is found (the queue is online)
               //!
               //! @return device to queue-manager
               outbound::detail::optional::Device& device();
            } // optional

         } // queue::manager

         namespace domain::manager
         {
            struct Connector : detail::base_connector
            {
               Connector();
               void reconnect();
               void clear();
            };
            using Device = communication::device::Outbound< Connector>;
            Device& device();

            namespace optional
            {
               struct Connector : detail::base_connector
               {
                  Connector();
                  void reconnect();
                  void clear();
               };

               using Device = communication::device::Outbound< Connector>;
               Device& device();
            } // optional

         } // domain::manager

         //! resets all outbound instances, hence they will start
         //! configure them self from the environment
         //! @attention only for unittests
         //void reset();

      } // outbound

   } // common::communication::instance
} // casual
