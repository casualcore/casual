//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ROUTING_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ROUTING_H_


#include "common/uuid.h"
#include "common/process.h"
#include "common/exception/system.h"
#include "common/algorithm.h"
#include "common/message/type.h"
#include "common/string.h"

#include <vector>
#include <mutex>


namespace casual
{
   namespace gateway
   {
      namespace outbound
      {

         template< typename P>
         struct basic_routing
         {
            using point_type = P;
            using container_type = std::vector< point_type>;
            using lock_type = std::lock_guard< std::mutex>;

            basic_routing() = default;

            basic_routing( basic_routing&& rhs)
            {
               lock_type lock{ rhs.m_mutex};
               m_points = std::move( rhs.m_points);
            }

            basic_routing& operator = ( basic_routing&& rhs)
            {
               std::lock( m_mutex, rhs.m_mutex);
               lock_type lock_this{ m_mutex, std::adopt_lock};
               lock_type lock_rhs{ rhs.m_mutex, std::adopt_lock};

               m_points = std::move( rhs.m_points);

               return *this;
            }

            basic_routing( const basic_routing&) = delete;
            basic_routing& operator = ( const basic_routing&) = delete;


            template< typename... Args>
            void emplace( Args&&... args) const
            {
               lock_type lock{ m_mutex};
               m_points.emplace_back( std::forward< Args>( args)...);
            }

            template< typename M>
            void add( const M& message) const
            {
               emplace( message.correlation, message.process, common::message::type( message));
            }


            point_type get( const common::Uuid& correlation) const
            {
               lock_type lock{ m_mutex};
               auto found = common::algorithm::find_if( m_points, [&]( const auto& p){
                  return correlation == p.correlation;
               });

               if( ! found)
               {
                  throw common::exception::system::invalid::Argument{ common::string::compose( "failed to find correlation: ", correlation)};
               }

               auto result = std::move( *found);
               m_points.erase( std::begin( found));
               return result;
            }

            //!
            //!
            //! @return all current routing points
            //!
            container_type extract() const
            {
               lock_type lock{ m_mutex};
               return std::exchange( m_points,{});
            }

         private:
            mutable std::mutex m_mutex;
            mutable container_type m_points;

         };

         namespace routing
         {
            struct Point
            {
               //Point();
               Point( const common::Uuid& correlation, common::process::Handle destination, common::message::Type type);

               common::Uuid correlation;
               common::process::Handle destination;
               common::message::Type type;

               friend std::ostream& operator << ( std::ostream& out, const Point& value);
            };

         } // routing

         using Routing = basic_routing< routing::Point>;


         namespace service
         {
            namespace routing
            {
               struct Point
               {
                  //Point();
                  Point( const common::Uuid& correlation,
                        common::process::Handle destination,
                        std::string service,
                        common::platform::time::point::type start);

                  common::Uuid correlation;
                  common::process::Handle destination;
                  std::string service;
                  common::platform::time::point::type start;

                  friend std::ostream& operator << ( std::ostream& out, const Point& value);
               };
            } // routing

            using Routing = basic_routing< routing::Point>;

         } // service


      } // outbound
   } // gateway

} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_TASK_H_
