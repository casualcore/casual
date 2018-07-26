//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"

#include "common/uuid.h"
#include "common/process.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {

         namespace instance
         {
            namespace identity
            {
               namespace service
               {
                  const Uuid manager{ "f58e0b181b1b48eb8bba01b3136ed82a"};
               } // service

               namespace forward
               {
                  const Uuid cache{ "f17d010925644f728d432fa4a6cf5257"};
               } // forward

               namespace traffic
               {
                  const Uuid manager{ "1aa1ce0e3e254a91b32e9d2ab22a8d31"};
               } // traffic

               namespace gateway
               {
                  const Uuid manager{ "b9624e2f85404480913b06e8d503fce5"};
               } // domain

               namespace queue
               {
                  const Uuid manager{ "c0c5a19dfc27465299494ad7a5c229cd"};
               } // queue

               namespace transaction
               {
                  const Uuid manager{ "5ec18cd92b2e4c60a927e9b1b68537e7"};
               } // transaction

            } // identity


            namespace fetch
            {
               enum class Directive : char
               {
                  wait,
                  direct
               };

               std::ostream& operator << ( std::ostream& out, Directive directive);

               process::Handle handle( const Uuid& identity, Directive directive = Directive::wait);

               //!
               //! Fetches the handle for a given pid
               //!
               //! @param pid
               //! @param directive if caller waits for the process to register or not
               //! @return handle to the process
               //!
               process::Handle handle( strong::process::id pid , Directive directive = Directive::wait);


            } // fetch




            void connect( const Uuid& identity, const process::Handle& process);
            void connect( const Uuid& identity);
            void connect( const process::Handle& process);
            void connect();


            //!
            //! ping a server that owns the @p ipc-id
            //!
            //! @note will block
            //!
            //! @return the process handle
            //!
            process::Handle ping( strong::ipc::id ipc);

            namespace outbound
            {
               namespace detail
               { 
                  struct base_connector
                  {
                     using blocking_policy = ipc::outbound::Connector::blocking_policy;
                     using non_blocking_policy = ipc::outbound::Connector::blocking_policy;

                     inline base_connector( process::Handle process)
                        : m_process{ std::move( process)}, m_connector( m_process.ipc),
                           m_socket( ipc::native::detail::create::domain::socket()) {}

                     inline const Socket& socket() const { return m_socket;}

                     inline const process::Handle& process() const { return m_process;}
                     inline const ipc::Address& destination() const { return m_connector.destination();}

                     friend std::ostream& operator << ( std::ostream& out, const base_connector& rhs);

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
                     basic_connector( const Uuid& identity, std::string environment);
                     
                     void reconnect();
                     
                     friend std::ostream& operator << ( std::ostream& out, const basic_connector& rhs)
                     {
                        return out << "{ destination: " << rhs.m_process.ipc
                           << ", identity: " << rhs.m_identity
                           << ", environment: " << rhs.m_environment
                           << '}';
                     }

                  private:
                     Uuid m_identity;
                     std::string m_environment;
                  };

                  //!
                  //! Will wait until the instance is online, could block for ever.
                  //!
                  using Device = communication::outbound::Device< basic_connector< fetch::Directive::wait>>;

                  namespace optional
                  {
                     //!
                     //! Will fail if the instance is offline.
                     //!
                     using Device = communication::outbound::Device< basic_connector< fetch::Directive::direct>>;
                  } // optional
               } // detail

               namespace service
               {
                  namespace manager
                  {
                     outbound::detail::Device& device();
                  } // manager
               } // service


               namespace transaction
               {
                  namespace manager
                  {
                     outbound::detail::Device& device();
                  } // manager
               } // transaction

               namespace gateway
               {
                  namespace manager
                  {
                     outbound::detail::Device& device();


                     namespace optional
                     {
                        //!
                        //! Can be missing. That is, this will not block
                        //! until the device is found (the gateway is online)
                        //!
                        //! @return device to gateway-manager
                        //!
                        outbound::detail::optional::Device& device();
                     } // optional
                  } // manager
               } // gateway

               namespace queue
               {
                  namespace manager
                  {
                     outbound::detail::Device& device();

                     namespace optional
                     {
                        //!
                        //! Can be missing. That is, this will not block
                        //! until the device is found (the queue is online)
                        //!
                        //! @return device to queue-manager
                        //!
                        outbound::detail::optional::Device& device();
                     } // optional

                  } // manager
               } // queue

               namespace domain
               {
                  namespace manager
                  {
                     struct Connector : detail::base_connector
                     {
                        Connector();
                        void reconnect();
                     };
                     using Device = communication::outbound::Device< Connector>;
                     Device& device();

                     namespace optional
                     {
                        struct Connector : detail::base_connector
                        {
                           Connector();
                           void reconnect();
                        };

                        using Device = communication::outbound::Device< Connector>;
                        Device& device();
                     } // optional
                  } // manager
               } // domain
            } // outbound
         } // instance
      } // communication
   } // common
} // casual
