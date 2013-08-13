//!
//! mockup.h
//!
//! @attention Only for unittest purposes
//!
//! Created on: Jul 16, 2013
//!     Author: Lazan
//!

#ifndef MOCKUP_H_
#define MOCKUP_H_

#include "common/queue.h"
#include <cassert>

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace queue
         {
            typedef platform::message_type_type message_type_type;

            template< typename M>
            class mockup_base
            {
            public:

               typedef common::platform::queue_id_type id_type;

               struct ipc_type
               {
                  typedef common::platform::queue_id_type id_type;

                  ipc_type( id_type id) : id{ id} {}

                  id_type id;
               };

               typedef M message_type;

               mockup_base( ipc_type queue)
               {
                  queue_id = queue.id;
               }

              bool send( marshal::output::Binary& archive, message_type_type type)
              {
                 assert( type == M::message_type);

                 marshal::input::Binary input{ std::move( archive)};
                 M value;
                 input >> value;
                 queue.push_back( std::move( value));

                 return true;
              }

              static void reset()
              {
                 queue.clear();
                 queue_id = 0;
              }

              static id_type queue_id;
              static std::deque< message_type> queue;
            };

            template< typename M>
            typename mockup_base< M>::id_type mockup_base< M>::queue_id = 0;

            template< typename M>
            std::deque< M> mockup_base< M>::queue = std::deque< message_type>{};




            namespace non_blocking
            {
               template< typename M>
               using base_writer = mockup_base< M>;

               //
               // Use the blocking reader as base,
               //
               template< typename M>
               struct base_reader : public mockup_base< M>
               {
                  using mockup_base< M>::mockup_base;

                  std::vector< marshal::input::Binary> next()
                  {
                     std::vector< marshal::input::Binary> result;

                     if( ! mockup_base< M>::queue.empty())
                     {
                        marshal::output::Binary output;

                        output << mockup_base< M>::queue.front();
                        mockup_base< M>::queue.pop_front();

                        result.emplace_back( std::move( output));
                     }

                     return result;
                  }

                  std::vector< marshal::input::Binary> read( message_type_type type)
                  {
                     assert( type == M::message_type);

                     return next();
                  }
               };

            } // non_blocking


            namespace blocking
            {

               template< typename M>
               using base_writer = mockup_base< M>;

               template< typename M>
               struct base_reader : public non_blocking::base_reader< M>
               {
                  typedef non_blocking::base_reader< M> base_type;
                  using base_type::base_reader;

                  marshal::input::Binary next()
                  {
                     return base_type::next.at( 0);
                  }

                  marshal::input::Binary read( message_type_type type)
                  {
                     return std::move( base_type::read( type).at( 0));
                  }
               };
            }// blocking

         /*
            template< typename M>
            struct WriteMessage
            {
               typedef common::platform::queue_id_type id_type;
               typedef M message_type;

               //! so it can be used with ipc_wrapper
               typedef id_type ipc_type;

               WriteMessage( id_type id)
               {
                  reset();
                  queue_id = id;
               }

               template< typename T>
               bool operator () ( T& value)
               {
                  //
                  // Value as a lvalue, and we can't just move it.
                  // So, why not write and read to a real queue, hence also test the queue stuff...
                  //

                  common::queue::ipc_wrapper< common::queue::blocking::Reader> reader;

                  common::queue::ipc_wrapper< common::queue::blocking::Writer> writer( reader.ipc().id());
                  writer( value);

                  T result;
                  reader( result);
                  replies.push_back( std::move( result));

                  return true;
               }

               static void reset()
               {
                  replies.clear();
                  queue_id = 0;
               }

               static id_type queue_id;
               static std::vector< message_type> replies;
            };

            template< typename M>
            typename WriteMessage< M>::id_type WriteMessage< M>::queue_id = 0;

            template< typename M>
            std::vector< M> WriteMessage< M>::replies = std::vector< message_type>{};




            template< typename M>
            struct ReadMessage
            {
               typedef common::platform::queue_id_type id_type;
               typedef M message_type;

               //! so it can be used with ipc_wrapper
               typedef id_type ipc_type;

               ReadMessage( id_type id)
               {
                  queue_id = id;
               }

               template< typename T>
               void operator () ( T& value)
               {
                  if( ! replies.empty())
                  {
                     value = std::move( replies.front());
                     replies.pop_front();
                  }
               }

               static void reset()
               {
                  replies.clear();
                  queue_id = 0;
               }

               static id_type queue_id;
               static std::deque< message_type> replies;
            };

            template< typename M>
            typename ReadMessage< M>::id_type ReadMessage< M>::queue_id = 0;

            template< typename M>
            std::deque< M> ReadMessage< M>::replies = std::deque< message_type>{};

             */
         } // queue

         namespace xa_switch
         {
            struct State
            {


            };

         }


      } // mockup
   } // common
} // casual

extern "C"
{
   extern struct xa_switch_t casual_mockup_xa_switch_static;
}


#endif /* MOCKUP_H_ */
