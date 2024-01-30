//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"

#include "common/algorithm/container.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include <vector>

namespace casual
{
   namespace gateway::group::outbound::state
   {

      namespace route
      {
         struct Destination
         {
            common::strong::correlation::id correlation;
            common::strong::ipc::id ipc;
            common::strong::execution::id execution;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( ipc);
            )
         };

         namespace error::callback
         {
            using type = common::unique_function< void( const route::Destination&)>;
         } // error::callback

         struct Point
         {
            //! constructs an empty Point
            Point() = default;
            
            template< typename M>
            inline Point( const M& message, common::strong::file::descriptor::id connection, error::callback::type callback)
               : destination{ message.correlation, message.process.ipc, message.execution}, connection{ connection}, callback{ std::move( callback)} {}

            route::Destination destination;
            common::strong::socket::id connection;
            error::callback::type callback;

            void error();


            inline explicit operator bool () const noexcept { return common::predicate::boolean( destination.correlation);}
            inline friend bool operator == ( const Point& lhs, const common::strong::correlation::id& rhs) { return lhs.destination.correlation == rhs;}
            inline friend bool operator == ( const Point& lhs, common::strong::file::descriptor::id rhs) { return lhs.connection == rhs;}
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( destination);
               CASUAL_SERIALIZE( connection);
               //CASUAL_SERIALIZE( callback);
            )
         };
         
      } // route

      struct Route
      {
         template< typename M>
         void add( const M& message, common::strong::file::descriptor::id connection, route::error::callback::type error)
         {
            m_points.emplace_back( message, connection, std::move( error));
         }
         void add( route::Point point);

         bool contains( const common::strong::correlation::id& correlation) const noexcept;

         void remove( const common::strong::correlation::id& correlation) noexcept;
         
         //! consumes and @returns the point - 'nil' point if not found 
         route::Point consume( const common::strong::correlation::id& correlation);

         //! consumes and @returns all associated 'points' to the connection.
         std::vector< route::Point> consume( common::strong::file::descriptor::id connection);

         //! consumes and @returns all 'points'
         std::vector< route::Point> consume();

         bool associated( common::strong::file::descriptor::id connection) const noexcept;

         inline const auto& points() const noexcept { return m_points;}
         inline bool empty() const noexcept { return m_points.empty();}
         inline platform::size::type size() const noexcept { return m_points.size();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_points, "points");
         )

      private:
         std::vector< route::Point> m_points;
      };

   } // gateway::group::outbound::state
} // casual
