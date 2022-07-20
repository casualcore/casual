//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "sql/database.h"

#include "queue/group/queuebase/statement.h"
#include "queue/common/ipc/message.h"

#include "common/string.h"

#include <filesystem>

namespace casual
{
   namespace queue::group
   {

      namespace queuebase
      {
         namespace queue
         {
            enum class Type : int
            {
               queue = 1,
               error_queue = 2,
            };

            std::ostream& operator << ( std::ostream& out, Type value);

            struct Retry 
            {
               platform::size::type count{};
               platform::time::unit delay{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( delay);
               )
            };
            
         } // queue

         struct Queue
         {
            Queue() = default;
            inline Queue( std::string name, queue::Retry retry) : name{ std::move( name)}, retry{ retry} {}
            inline Queue( std::string name) : name{ std::move( name)} {};

            common::strong::queue::id id;
            std::string name;
            queue::Retry retry;
            common::strong::queue::id error;

            inline queue::Type type() const { return error ? queue::Type::queue : queue::Type::error_queue;}

            inline friend bool operator == ( const Queue& lhs, common::strong::queue::id id) { return lhs.id == id;}
            inline friend bool operator == ( const Queue& lhs, std::string_view name) { return lhs.name == name;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retry);
               CASUAL_SERIALIZE( error);
            )
         };      
   
         namespace message
         {
            enum class State
            {
               added = 1,
               enqueued = 2,
               removed = 3,
               dequeued = 4
            };

            std::ostream& operator << ( std::ostream& out, State value);

            struct Available
            {
               common::strong::queue::id queue;
               platform::size::type count;
               platform::time::point::type when;

               inline friend bool operator == ( const Available& lhs, common::strong::queue::id rhs) { return lhs.queue == rhs;}
               
               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( when);
               )
            };
         } // message

      } // queuebase

      class Queuebase
      {
      public:
         Queuebase() = default;

         //! open/creates the queuebase, and starts a transaction
         Queuebase( std::filesystem::path database);
         //! commits current transaction, if any.
         ~Queuebase();

         Queuebase( Queuebase&&) = default;
         Queuebase& operator =( Queuebase&&) = default;

         inline explicit operator bool() const noexcept { return m_connection && true;}

         const std::filesystem::path& file() const;

         //! creates or updates a queue
         //! @returns the created or updated queue
         queuebase::Queue create( queuebase::Queue queue);
         void update( const queuebase::Queue& queue);
         void remove( common::strong::queue::id id);

         //! @returns the created queues
         std::vector< queuebase::Queue> update( std::vector< queuebase::Queue> update, const std::vector< common::strong::queue::id>& remove);

         //! @returns the queue if it exists
         //! @{
         std::optional< queuebase::Queue> queue( std::string_view name);
         std::optional< queuebase::Queue> queue( common::strong::queue::id id);
         //! @}

         queue::ipc::message::group::enqueue::Reply enqueue( const queue::ipc::message::group::enqueue::Request& message);
         
         queue::ipc::message::group::dequeue::Reply dequeue( 
            const queue::ipc::message::group::dequeue::Request& message, 
            const platform::time::point::type& now);

         //! 'meta peek' to get information about messages
         queue::ipc::message::group::message::meta::peek::Reply peek( const queue::ipc::message::group::message::meta::peek::Request& request);
         //! actual peek of messages
         queue::ipc::message::group::message::peek::Reply peek( const queue::ipc::message::group::message::peek::Request& request);

         //! @returns all queues that is found in `queues` and has messages potentially available for dequeue
         std::vector< queuebase::message::Available> available( std::vector< common::strong::queue::id> queues) const;

         //! @returns the earliest available message in the queue, if any.
         std::optional< platform::time::point::type> available( common::strong::queue::id queue) const;

         //! @return number of restored messages for the queue
         platform::size::type restore( common::strong::queue::id id);

         //! @returns number of 'deleted' messages for the queue
         //! @note: only `enqueued` messages are deleted (no pending for commit)
         platform::size::type clear( common::strong::queue::id queue);

         //! @returns id:s of the messages that was removed
         std::vector< common::Uuid> remove( common::strong::queue::id queue, std::vector< common::Uuid> messages);

         void commit( const common::transaction::ID& id);
         void rollback( const common::transaction::ID& id);

         //! information
         std::vector< queue::ipc::message::group::state::Queue> queues();
         //! information
         std::vector< queue::ipc::message::group::message::Meta> meta( common::strong::queue::id id);

         void metric_reset( const std::vector< common::strong::queue::id>& ids);
         

         //! @return the number of rows affected by the last statement.
         platform::size::type affected() const;

         //! commits current sqlite transaction, and starts a new one
         void persist();
         //! rollback current sqlite transaction and starts a new one
         void rollback();

         //! @returns the id of of the queue corresponding to the name
         common::strong::queue::id id( std::string_view name) const;


         //! If message has a queue-id that will be returned,
         //! otherwise we lookup id from name
         //!
         //! @param message enqueue or dequeue message
         //! @return id to the queue
         template< typename M>
         auto id( const M& message) const -> decltype( id( message.name))
         {
            if( message.queue)
               return message.queue;
            else 
               return id( message.name);
         }

         sql::database::Version version();

         //! logs explain performance metrics to the provided `path`
         void explain( const std::filesystem::path& path);

      private:

         sql::database::Connection m_connection;
         queuebase::Statement m_statement;
      };

   } // queue::group
} // casual


