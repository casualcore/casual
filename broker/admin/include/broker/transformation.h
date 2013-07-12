//!
//! transform.h
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include "broker/servervo.h"
#include "broker/servicevo.h"
#include "broker/broker.h"

// TODO: temp
//#include <iostream>

namespace casual
{
   namespace generic
   {
      namespace link
      {
         struct Nested
         {
            template< typename T1, typename T2>
            struct Type
            {
               //Type( T1&& t1, T2&& t2) : left( std::forward< T1>( t1)), right( std::forward< T2>( t2)) {}
               Type( T1 t1, T2 t2) : left( t1), right( t2) {}

               Type( Type&&) = default;

               template< typename T>
               auto operator () ( T&& value) const -> decltype( T1()( T2()( std::forward< T>( value))))
               {
                  return left( right( std::forward< T>( value)));
               }
            private:
               T1 left;
               T2 right;
            };

            template< typename T1, typename T2>
            static Type< T1, T2> make( T1&& t1, T2&& t2)
            {
               return Type< T1, T2>( std::forward< T1>( t1), std::forward< T2>( t2));
            }
         };

      }

      template< typename Link>
      struct Chain
      {
         template< typename Arg>
         static auto link( Arg&& param) -> decltype( std::forward< Arg>( param))
         {
            return std::forward< Arg>( param);
         }

         template< typename Arg, typename... Args>
         static auto link( Arg&& param, Args&&... params) -> decltype( Link::make( std::forward< Arg>( param), link( std::forward< Args>( params)...)))
         {
            return Link::make( std::forward< Arg>( param), link( std::forward< Args>( params)...));
         }
      };


      namespace extract
      {
         struct Second
         {


            template< typename T>
            auto operator () ( T&& value) const -> decltype( value.second)
            {
               return value.second;
            }

         };

      }

   } // generic




   namespace broker
   {
      namespace admin
      {
         namespace transform
         {
            //template< typename Link>
            //using generic::Chain< generic::link::Linked> Chain;

            typedef generic::Chain< generic::link::Nested> Chain;


            struct Server
            {
               admin::ServerVO operator () ( const broker::Server& value) const
               {
                  admin::ServerVO result;

                  result.setPath( value.path);
                  result.setPid( value.pid);
                  result.setQueue( value.queue_key);
                  result.setIdle( value.idle);

                  return result;
               }
            };

            struct Pid
            {
               broker::Server::pid_type operator () ( const broker::Server* value) const
               {
                  return value->pid;
               }
            };

            struct Service
            {
               admin::ServiceVO operator () ( const broker::Service& value) const
               {
                  admin::ServiceVO result;

                  result.setNameF( value.information.name);
                  result.setTimeoutF( value.information.timeout);

                  std::vector< long> pids;

                  std::transform( std::begin( value.servers), std::end( value.servers), std::back_inserter( pids), Pid());

                  result.setPids( std::move( pids));

                  return result;
               }
            };



         }
      }


   }

}



#endif /* TRANSFORM_H_ */
