//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/instance/identity.h"
#include "common/communication/ipc.h"

#include "common/uuid.h"
#include "common/process.h"

#include <iosfwd>

namespace casual
{
   namespace common::communication::instance
   {
      namespace lookup
      {
         enum class Directive : short
         {
            wait,
            direct
         };
         std::string_view description( Directive value) noexcept;

         //! Sends a request for a lookup of a _instance_ for the provided `identity`
         //! @returns the correlation id for the reply. 
         //! @attention `common::message::domain::process::lookup::Reply` needs to be handled for the reply.
         [[nodiscard]] strong::correlation::id request( const Uuid& identity, Directive directive = Directive::wait);

      } // lookup

      namespace fetch
      {  
         using Directive = lookup::Directive;

         //! Fetches the handel for the given `identity` (blocking)
         //! @return handle to the process, or _nil process_, if `directive` is _direct_ and `identity` was not found.
         [[nodiscard]] process::Handle handle( const Uuid& identity, Directive directive = Directive::wait);

         //! Fetches the handle for a given pid (blocking)
         //!
         //! @param pid
         //! @param directive if caller waits (block) for the process to register or not
         //! @return handle to the process, or _nil process_, if `directive` is _direct_ and `pid` was not found.
         [[nodiscard]] process::Handle handle( strong::process::id pid, Directive directive = Directive::wait);

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

         //! @{ connect 'singleton' 'manager' to casual local domain
         void connect( const instance::Identity& identity, const process::Handle& process);
         void connect( const instance::Identity& identity);
         //! @{
         
         //! connect without _environemnt state_, useful for 'singletons' that is not known to others. 
         void connect( const Uuid& id);
         
      } // whitelist



      //! ping a server that owns the @p ipc-id
      //!
      //! @note will block
      //!
      //! @return the process handle
      [[nodiscard]] process::Handle ping( strong::ipc::id ipc);

      namespace outbound
      {
         namespace detail
         { 
            struct base_connector
            {
               using blocking_policy = ipc::outbound::Connector::blocking_policy;
               using non_blocking_policy = ipc::outbound::Connector::non_blocking_policy;
               using complete_type = communication::ipc::message::Complete;

               base_connector();

               [[nodiscard]] inline const Socket& socket() const { return m_socket;}
               [[nodiscard]] inline const ipc::Address& destination() const { return m_connector.destination();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_process, "process");
                  CASUAL_SERIALIZE_NAME( m_connector, "connector");
                  CASUAL_SERIALIZE_NAME( m_socket, "socket");
               )

            protected:
               void reset( process::Handle process);

               process::Handle m_process;
               ipc::outbound::Connector m_connector;
               Socket m_socket;
            };

            template< lookup::Directive directive>
            struct basic_connector : base_connector
            {
               basic_connector( instance::Identity identity);
               
               void reconnect();
               [[nodiscard]] const process::Handle& process();

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
            using Device = communication::device::Outbound< basic_connector< lookup::Directive::wait>>;

            namespace optional
            {
               //! Will fail if the instance is offline.
               using Device = communication::device::Outbound< basic_connector< lookup::Directive::direct>>;
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
               void reconnect();
               const process::Handle& process();
               void clear();
            };
            using Device = communication::device::Outbound< Connector>;
            Device& device();

            namespace optional
            {
               struct Connector : detail::base_connector
               {
                  void reconnect();
                  const process::Handle& process();
                  void clear();
               };

               using Device = communication::device::Outbound< Connector>;
               Device& device();
            } // optional

         } // domain::manager

      } // outbound

   } // common::communication::instance
} // casual
