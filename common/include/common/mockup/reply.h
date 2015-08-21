//!
//! reply.h
//!
//! Created on: Jul 18, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MOCKUP_REPLY_H_
#define CASUAL_COMMON_MOCKUP_REPLY_H_

#include "common/mockup/ipc.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace reply
         {
            struct result_t
            {
               template< typename M>
               result_t( platform::queue_id_type queue, M&& message)
                  : queue( queue), complete( marshal::complete( std::forward< M>( message))) {}

               platform::queue_id_type queue;
               common::ipc::message::Complete complete;
            };

            class Handler
            {
               struct base_reply
               {
                  std::vector< result_t> operator () ( common::ipc::message::Complete& complete)
                  {
                     return dispatch( complete);
                  }

                  virtual std::vector< result_t> dispatch( common::ipc::message::Complete& complete) = 0;
               };

               template< typename F>
               struct basic_reply : base_reply
               {
                  using traits_type = traits::function< F>;
                  using message_type = typename std::decay< typename traits_type::template argument< 0>::type>::type;

                  template< typename T>
                  basic_reply( T&& functor) : m_functor( std::move( functor)) {}

                  std::vector< result_t> dispatch( common::ipc::message::Complete& complete) override
                  {
                     transform_t transform( complete);

                     return m_functor( transform);
                  }

               private:

                  struct transform_t
                  {
                     transform_t( common::ipc::message::Complete& complete) : complete( complete) {}

                     template< typename T>
                     operator T ()
                     {
                        T message;
                        complete >> message;
                        return message;
                     }

                     common::ipc::message::Complete& complete;
                  };

                  F m_functor;
               };


               using repliers_type = std::map< platform::message_type_type, std::unique_ptr< base_reply>>;

               repliers_type m_repliers;

            public:

               template< typename H>
               void insert( H&& handler)
               {
                  using message_type = typename std::decay< typename traits::function< H>::template argument< 0>::type>::type;

                  std::unique_ptr< base_reply> replier{ new basic_reply< typename std::decay< H>::type>{ std::forward< H>( handler)}};

                  m_repliers[ message_type::message_type] = std::move( replier);
               }

               template< typename H, typename... Hs>
               void insert( H&& handler, Hs&&... handlers)
               {
                  insert( std::forward< H>( handler));
                  insert( std::forward< Hs>( handlers)...);
               }


               Handler() = default;

               Handler( Handler&&) = default;
               Handler& operator = ( Handler&&) = default;
               Handler( const Handler&) = default;
               // gcc seams to use forward-reference ctor when type is non-const lvalue
               Handler( Handler&) = default;
               Handler& operator = ( const Handler&) =  default;


               template< typename... Hs>
               Handler( Hs&&... handlers)
               {
                  insert( std::forward< Hs>( handlers)...);
               }


               std::vector< result_t> operator () ( common::ipc::message::Complete& message)
               {
                  auto found = range::find( m_repliers, message.type);

                  if( found)
                  {
                     return (*found->second)( message);
                  }
                  return {};
               }


            };

         } // transform
      } // mockup
   } // common


} // casual

#endif // REPLY_H_
