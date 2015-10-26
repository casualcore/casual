//!
//! queue.cpp
//!
//! Created on: Nov 23, 2014
//!     Author: Lazan
//!

#include "queue/api/queue.h"

#include "common/buffer/type.h"
#include "common/buffer/pool.h"


#include "sf/xatmi_call.h"
#include "sf/trace.h"


namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            template< typename M>
            sf::platform::Uuid enqueue( const std::string& queue, M&& message)
            {
               sf::Trace trace( "casual::queue::enqueue", common::log::internal::queue);

               sf::xatmi::service::binary::Sync service{ "casual.enqueue"};
               service << CASUAL_MAKE_NVP( queue);
               service << CASUAL_MAKE_NVP( message);

               auto reply = service();

               sf::platform::Uuid returnValue;

               reply >> CASUAL_MAKE_NVP( returnValue);

               return returnValue;
            }

         } // <unnamed>
      } // local

      sf::platform::Uuid enqueue( const std::string& queue, const Message& message)
      {
         sf::Trace trace( "casual::queue::enqueue", common::log::internal::queue);

         return local::enqueue( queue, message);
      }

      std::vector< Message> dequeue( const std::string& queue, const Selector& selector)
      {
         sf::Trace trace{ "casual::queue::dequeue", common::log::internal::queue};

         sf::xatmi::service::binary::Sync service{ "casual.dequeue"};
         service << CASUAL_MAKE_NVP( queue);
         service << CASUAL_MAKE_NVP( selector);

         auto reply = service();

         std::vector< Message> returnValue;

         reply >> CASUAL_MAKE_NVP( returnValue);

         return returnValue;
      }

      std::vector< Message> dequeue( const std::string& queue)
      {
         return dequeue( queue, Selector{});
      }

      namespace xatmi
      {
         namespace copy
         {
            //
            // To hold reference data, so we don't need to copy the buffer.
            //
            struct Payload
            {
               template< typename T, typename Iter>
               Payload( T&& type_, Iter first, Iter last)
                 : data( first, last)
               {
                  type.type = type_.name;
                  type.subtype = type_.subname;
               }

               Payload( Payload&&) = default;
               Payload& operator = ( Payload&&) = default;

               queue::Payload::type_t type;

               sf::platform::binary_type data;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( data);
               })
            };

            using Message = basic_message< Payload>;


         } // reference

         sf::platform::Uuid enqueue( const std::string& queue, const Message& message)
         {
            sf::Trace trace{ "casual::queue::xatmi::enqueue", common::log::internal::queue};

            auto send = common::buffer::pool::Holder::instance().get( message.payload.buffer, message.payload.size);

            //
            // We have to send only the real size of the buffer [buffer.begin, buffer.begin + transport_size)
            //
            // We could do this in several ways, but the most clean end ease way is to do
            // a copy... This is probably the only place we do a copy of a buffer.
            //
            // TODO: get rid of the copy in a conformant way
            //         we probably need to change the interface for 'binary' in write-archives (to take a range, or iterator first, last)
            //
            copy::Message send_message{
               message.id, message.attributes,
                 { send.payload.type, send.payload.memory.begin(), send.payload.memory.begin() + send.transport}};

            return local::enqueue( queue, send_message);
         }



         std::vector< Message> dequeue( const std::string& queue, const Selector& selector)
         {
            sf::Trace trace{ "casual::queue::xatmi::dequeue", common::log::internal::queue};

            std::vector< Message> results;

            for( auto& message : queue::dequeue( queue, selector))
            {
               Message result;
               result.attributes = std::move( message.attributes);
               result.id = std::move( message.id);

               {
                  common::buffer::Payload payload;
                  payload.type.name = std::move( message.payload.type.type);
                  payload.type.subname = std::move( message.payload.type.subtype);
                  payload.memory = std::move( message.payload.data);

                  result.payload.size = payload.memory.size();
                  result.payload.buffer = common::buffer::pool::Holder::instance().insert( std::move( payload));
               }

               results.push_back( std::move( result));
            }
            return results;
         }

         std::vector< Message> dequeue( const std::string& queue)
         {
            return xatmi::dequeue( queue, Selector{});
         }

      } // xatmi

      namespace peek
      {
         std::vector< Message> queue( const std::string& queue)
         {
            std::vector< Message> result;

            return result;
         }

      } // peek

   } // queue
} // casual
