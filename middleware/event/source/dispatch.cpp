//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/event/dispatch.h"
#include "casual/event/common.h"

#include "common/event/listen.h"
#include "common/communication/ipc.h"
#include "common/exception/casual.h"


namespace casual
{
   using namespace common;
   namespace event
   {
      inline namespace v1 {
      
      namespace detail
      {
         struct Dispatch::Implementation 
         {
            using device_type = common::communication::ipc::inbound::Device;
            using handler_type = device_type::handler_type;

            void start() 
            {
               if( empty)
                  common::event::listen( 
                     common::event::condition::compose( common::event::condition::idle( std::move( empty))), 
                     std::move( handler));
               else 
                  common::event::listen( common::event::condition::compose(), std::move( handler));
            }

            template< typename T> 
            void add_service( T&& callback)
            {
               auto handle = [ callback = std::forward< T>( callback)]( common::message::event::service::Calls& message)
               {
                  Trace trace{ "event::detail::Implementation::handle message::event::service::Calls"};
                  log::line( verbose::log, "message: ", message);
                  
                  auto transform_metric = []( common::message::event::service::Metric& metric)
                  {
                     model::service::Call::Metric result;
                     metric.execution.copy( result.execution);
                     result.service.name = std::move( metric.service);
                     result.service.parent = std::move( metric.parent);
                     result.process.pid = metric.process.pid.value();
                     metric.process.ipc.value().copy( result.process.ipc);


                     result.start = std::move( metric.start);
                     result.end = std::move( metric.end);

                     result.pending = std::move( metric.pending);

                     result.code = common::cast::underlying( metric.code);

                     return result;
                  };
                  model::service::Call event;
                  algorithm::transform( message.metrics, event.metrics, transform_metric);
                  
                  callback( std::move( event));
               };

               handler.insert( std::move( handle));
            }

            handler_type handler;
            std::function< void()> empty;
         };

         Dispatch::Dispatch() 
            : m_implementation{ std::make_unique< Implementation>()} {}  

         Dispatch::Dispatch( Dispatch&&) noexcept = default;
         Dispatch& Dispatch::operator = ( Dispatch&&) noexcept = default;
         Dispatch::~Dispatch() = default;

         void Dispatch::start()
         {
            Trace trace{ "event::detail::Dispatch::start"};

            try 
            {
               m_implementation->start();
            }
            catch( const exception::casual::Shutdown&)
            {
               log::line( log, "shutdown");
            }
            catch( const std::system_error& error)
            {
               log::line( log::category::error, error);
               throw;
            }
         }
         
         void Dispatch::add( std::function< void( const model::service::Call&)> callback)
         {
            m_implementation->add_service( std::move( callback));
         }

         void Dispatch::add( std::function< void( model::service::Call&&)> callback)
         {
            m_implementation->add_service( std::move( callback));
         }

         void Dispatch::add( std::function< void()> empty)
         {
            m_implementation->empty = std::move( empty);
         }

      } // detail

      } // v1
   } // event
} // casual