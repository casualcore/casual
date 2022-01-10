//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/event/model.h"

#include <memory>
#include <functional>

namespace casual
{
   namespace event
   {
      inline namespace v1 {
      
      namespace detail
      {
         struct Dispatch 
         {
         private:
            template< typename T, typename... Ts> 
            Dispatch( T&& callback, Ts&&... callbacks) : Dispatch()
            {
               initialize( std::forward< T>( callback), std::forward< Ts>( callbacks)...);
            }
            
            Dispatch();
            Dispatch( Dispatch&&) noexcept;
            Dispatch& operator = ( Dispatch&&) noexcept;
            ~Dispatch();

            void start();

            template< typename T, typename... Ts> 
            friend void dispatch( T&& callback, Ts&&... callbacks);
            
            void add( std::function< void( const model::service::Call&)> callback);
            void add( std::function< void( model::service::Call&&)> callback);
            void add( std::function< void()> empty);

            // sentinel
            inline void initialize() {}

            template< typename T, typename... Ts>
            void initialize( T&& callback, Ts&&... callbacks)
            {
               add( std::forward< T>( callback));
               initialize( std::forward< Ts>( callbacks)...);
            }

         private:

            [[deprecated]] void add( std::function< void( const model::v1::service::Call&)> callback);
            [[deprecated]] void add( std::function< void( model::v1::service::Call&&)> callback);

            struct Implementation;
            std::unique_ptr< Implementation> m_implementation;
         };

         // just to be able to befriend this function...
         template< typename T, typename... Ts> 
         void dispatch( T&& callback, Ts&&... callbacks)
         {
            Dispatch( std::forward< T>( callback), std::forward< Ts>( callbacks)...).start();
         }
      } // detail

      //! start listen to events.
      //!
      //! dispatches incoming events to the corresponding 
      //! callbacks.
      //! blocks until domain-manager asks for a shutdown, and the control
      //! goes back to caller
      //!
      //! possible callbacks to provide:
      //!  * void()                         - invoked when 'queue' is empty -> may be used to "flush" stuff
      //!  * void( model::service::Call&&)  - system call metrics -> see casual/event/model.h 
      //!  * void( const model::service::Call&) 
      //!
      //! @throws std::system_error when things go wrong.
      template< typename T, typename... Ts> 
      void dispatch( T&& callback, Ts&&... callbacks)
      {
         detail::dispatch( std::forward< T>( callback), std::forward< Ts>( callbacks)...);
      }


      } // v1
   } // event
} // casual