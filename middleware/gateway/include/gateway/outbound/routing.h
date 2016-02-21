//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ROUTING_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_ROUTING_H_


#include "common/uuid.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/algorithm.h"

#include <vector>
#include <mutex>


namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         struct Routing
         {
            using lock_type = std::lock_guard< std::mutex>;

            Routing();

            Routing( Routing&& lhs);
            Routing& operator = ( Routing&& lhs);

            Routing( const Routing&) = delete;
            Routing& operator = ( const Routing&) = delete;

            struct Point
            {
               Point();
               Point( const common::Uuid& correlation, common::process::Handle destination);

               common::Uuid correlation;
               common::process::Handle destination;

               friend std::ostream& operator << ( std::ostream& out, const Point& value);
            };

            void add( const common::Uuid& correlation, common::process::Handle destination) const;

            Point get( const common::Uuid& correlation) const;

         private:
            mutable std::mutex m_mutex;
            mutable std::vector< Point> m_points;
         };
      } // outbound
   } // gateway

} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_TASK_H_
