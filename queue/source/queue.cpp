//!
//! queue.cpp
//!
//! Created on: Jul 22, 2014
//!     Author: Lazan
//!

#include "queue/queue.h"




namespace casual
{
   namespace queue
   {

      namespace xatmi
      {
         struct Message::Implementation
         {

            common::Uuid id;
            std::string correlation;
            std::string reply;
            common::platform::time_type available;

            common::platform::raw_buffer_type payload;

         };

         Message::Message() = default;
         Message::~Message() = default;

         const common::Uuid& Message::id() const
         {
            return m_implementation->id;
         }

         const std::string& Message::correlation() const
         {
            return m_implementation->correlation;
         }

         Message& Message::correlation( std::string value)
         {
            m_implementation->correlation = std::move( value);
            return *this;
         }

         const std::string& Message::reply() const
         {
            return m_implementation->reply;
         }

         Message& Message::reply( std::string value)
         {
            m_implementation->reply = std::move( value);
            return *this;
         }

         common::platform::time_type Message::available() const
         {
            return m_implementation->available;
         }

         Message& Message::available( common::platform::time_type value)
         {
            m_implementation->available = std::move( value);
            return *this;
         }

         common::platform::raw_buffer_type Message::payload() const
         {
            return m_implementation->payload;
         }

         Message& Message::payload( common::platform::raw_buffer_type value)
         {
            m_implementation->payload = value;
            return *this;
         }

      } // xatmi


   } // queue
} // casual
