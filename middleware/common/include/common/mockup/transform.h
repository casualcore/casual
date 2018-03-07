//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_MOCKUP_TRANSFORM_H_
#define CASUAL_COMMON_MOCKUP_TRANSFORM_H_


#include "common/traits.h"
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
                  using traits_type = traits::function< F>;
                  using message_type = typename std::decay< typename traits_type::template argument< 0>::type>::type;

                  basic_transform( F functor) : m_functor( std::move( functor)) {}

                  std::vector< communication::message::Complete> operator () ( communication::message::Complete& complete)
                  {
                     transform_t transform( complete);

                     return m_functor( transform);
                  }
               private:

                  struct transform_t
                  {
                     transform_t( communication::message::Complete& complete) : complete( complete) {}

                     template< typename T>
                     operator T ()
                     {
                        T message;
                        complete >> message;
                        return message;
                     }

                     communication::message::Complete& complete;
                  };

                  F m_functor;
               };




               using transformers_type = std::map< communication::message::Complete::message_type_type, ipc::transform_type>;

               transformers_type m_transformers;


               template< typename H>
               void assign( H&& handler)
               {
                  using message_type = typename std::decay< typename traits::function< H>::template argument< 0>::type>::type;

                  m_transformers.emplace(
                        message_type::type(),
                        basic_transform< typename std::decay< H>::type>{ std::forward< H>( handler)});
               }

               template< typename H, typename... Hs>
               void assign( H&& handler, Hs&&... handlers)
               {
                  assign( std::forward< H>( handler));
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


               std::vector< communication::message::Complete> operator () ( communication::message::Complete& message)
               {
                  auto found = algorithm::find( m_transformers, message.type);

                  if( found)
                  {
                     return found->second( message);
                  }
                  return {};
               }


            };

         } // transform
      } // mockup
   } // common


} // casual

#endif // DISPATCH_H_
