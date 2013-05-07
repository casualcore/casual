//!
//! message_dispatch.h
//!
//! Created on: Dec 1, 2012
//!     Author: Lazan
//!

#ifndef MESSAGE_DISPATCH_H_
#define MESSAGE_DISPATCH_H_

#include "common/marshal.h"


#include <map>
#include <memory>

namespace casual
{
   namespace common
   {

      namespace dispatch
      {

         class base_handler
         {
         public:
            virtual ~base_handler() = default;
            virtual void marshal( marshal::input::Binary& binary) = 0;


         };



         template< typename H>
         class basic_handler : public base_handler
         {
         public:

            typedef H handler_type;
            typedef typename handler_type::message_type message_type;

            template< typename... Arguments>
            basic_handler( Arguments&&... arguments) : m_handler( std::forward< Arguments>( arguments)...) {}

            void marshal( marshal::input::Binary& binary)
            {
               message_type message;
               binary >> message;

               m_handler.dispatch( message);
            }

         private:
            handler_type m_handler;
         };



         class Handler
         {
         public:

            typedef std::map< int, std::unique_ptr< base_handler> > handlers_type;

            template< typename H, typename... Arguments>
            void add( Arguments&& ...arguments)
            {
               //basic_handler< H>

               handlers_type::mapped_type handler( new basic_handler< H>{ std::forward< Arguments>( arguments)...});

               m_handlers[ H::message_type::message_type] = std::move( handler);
               //m_handlers[ decltype( handler.createMessage())::message_type] = std::move( handler);
            }

            bool dispatch( marshal::input::Binary& binary)
            {
               auto findIter = m_handlers.find( binary.type());

               if( findIter != m_handlers.end())
               {
                  findIter->second->marshal( binary);
                  return true;
               }
               return false;
            }


            std::size_t size() const
            {
               return m_handlers.size();
            }

         private:
            handlers_type m_handlers;

         };

      }


   }


}


#endif /* MESSAGE_DISPATCH_H_ */
