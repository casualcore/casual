//!
//! dispatch.h
//!
//! Created on: Jan 8, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MOCKUP_TRANSFORM_H_
#define CASUAL_COMMON_MOCKUP_TRANSFORM_H_

#include "common/ipc.h"

#include "common/mockup/ipc.h"
#include "common/marshal/binary.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace transform
         {
            class Handler
            {

               template< typename F>
               struct basic_transform
               {
                  typedef typename F::message_type message_type;

                  basic_transform( F functor) : m_functor( std::move( functor)) {}

                  std::vector< common::ipc::message::Complete> operator () ( common::ipc::message::Complete& complete)
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




               using transformers_type = std::map< platform::message_type_type,ipc::transform_type>;

               transformers_type m_transformers;



               template< typename H>
               static ipc::transform_type assign_helper( H&& handler)
               {
                  return basic_transform< typename std::decay< H>::type>{ std::forward< H>( handler)};
               }


               template< typename H>
               void assign( H&& handler)
               {
                  m_transformers.emplace( H::message_type::message_type, assign_helper( std::forward< H>( handler)));
               }

               template< typename H, typename... Hs>
               void assign( H&& handler, Hs&&... handlers)
               {
                  m_transformers.emplace( H::message_type::message_type, assign_helper( std::forward< H>( handler)));
                  assign( std::forward< Hs>( handlers)...);
               }

            public:

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
                  assign( std::forward< Hs>( handlers)...);
               }


               std::vector< common::ipc::message::Complete> operator () ( common::ipc::message::Complete& message)
               {
                  return m_transformers.at( message.type)( message);
               }


            };

         } // transform
      } // mockup
   } // common


} // casual

#endif // DISPATCH_H_
