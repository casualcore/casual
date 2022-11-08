//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/state/route.h"

#include "gateway/common.h"

namespace casual
{
   using namespace common;
   namespace gateway::group::outbound::state
   {
      namespace route
      {
         void Point::error()
         {
            Trace trace{ "gateway::group::outbound::state::route::Point::error"};
            log::line( verbose::log, "point: ", *this);

            callback( destination);
         }
         
      } // route

      void Route::add( route::Point point)
      {
         m_points.push_back( std::move( point));
      }

      bool Route::contains( const common::strong::correlation::id& correlation) const noexcept
      {
         return common::predicate::boolean( common::algorithm::find( m_points, correlation));
      }

      void Route::remove( const common::strong::correlation::id& correlation) noexcept
      {
         if( auto found = common::algorithm::find( m_points, correlation))
            m_points.erase( std::begin( found));
      }
      
      //! consumes and @returns the point - 'empty' point if not found 
      route::Point Route::consume( const common::strong::correlation::id& correlation)
      {
         if( auto found = common::algorithm::find( m_points, correlation))
            return common::algorithm::container::extract( m_points, std::begin( found));

         return route::Point{};
      }

      //! consumes and @returns all associated 'points' to the connection.
      std::vector< route::Point> Route::consume( common::strong::file::descriptor::id connection)
      {
         auto consume = std::get< 1>( common::algorithm::partition( m_points, common::predicate::negate( common::predicate::value::equal( connection))));

         return common::algorithm::container::extract( m_points, consume);
      }

      //! consumes and @returns all 'points'
      std::vector< route::Point> Route::consume() 
      {
         return std::exchange( m_points, {});
      }

      bool Route::associated( common::strong::file::descriptor::id connection) const noexcept
      {
         return common::predicate::boolean( common::algorithm::find( m_points, connection));
      }

   } // gateway::group::outbound::state
   
} // casual