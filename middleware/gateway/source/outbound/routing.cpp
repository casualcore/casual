//!
//! casual 
//!

#include "gateway/outbound/routing.h"


namespace casual
{
   namespace gateway
   {
      namespace outbound
      {

         Routing::Point::Point() = default;
         Routing::Point::Point( const common::Uuid& correlation, common::process::Handle destination)
         : correlation{ correlation}, destination{ destination} {}


         Routing::Routing() = default;

         Routing::Routing( Routing&& rhs)
         {
            lock_type lock{ rhs.m_mutex};
            m_points = std::move( rhs.m_points);
         }

         Routing& Routing::operator = ( Routing&& rhs)
         {
            std::lock( m_mutex, rhs.m_mutex);
            lock_type lock_this{ m_mutex, std::adopt_lock};
            lock_type lock_rhs{ rhs.m_mutex, std::adopt_lock};

            m_points = std::move( rhs.m_points);

            return *this;
         }


         void Routing::add( const common::Uuid& correlation, common::process::Handle destination) const
         {
            lock_type lock{ m_mutex};
            m_points.emplace_back( correlation, destination);
         }

         Routing::Point Routing::get( const common::Uuid& correlation) const
         {
            lock_type lock{ m_mutex};
            auto found = common::range::find_if( m_points, [&]( const Point& p){
               return correlation == p.correlation;
            });

            if( ! found)
            {
               throw common::exception::invalid::Argument{ "failed to find correlation - Routing::get", CASUAL_NIP( correlation)};
            }

            auto result = std::move( *found);
            m_points.erase( std::begin( found));
            return result;
         }


         std::ostream& operator << ( std::ostream& out, const Routing::Point& value)
         {
            return out << "{ correlation: " << value.correlation << ", destination: " << value.destination << "}\n";
         }

      } // outbound

   } // gateway

} // casual
