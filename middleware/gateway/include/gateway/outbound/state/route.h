//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"

#include "common/algorithm.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include <vector>

namespace casual
{
   using namespace common;
   namespace gateway::outbound::state
   {
      namespace route
      {
         template< typename Point>
         struct basic_routing
         {
            template< typename... Args>
            void emplace( Args&&... args)
            {
               m_points.emplace_back( std::forward< Args>( args)...);
            }

            template< typename M>
            void add( const M& message, common::strong::file::descriptor::id connection)
            {
               emplace( message.correlation, message.process, common::message::type( message), connection);
            }

            void add( Point&& point)
            {
               m_points.push_back( std::move( point));
            }
            
            //! consumes and @returns the point - 'empty' point if not found 
            Point consume( const common::Uuid& correlation)
            {
               if( auto found = common::algorithm::find( m_points, correlation))
                  return common::algorithm::extract( m_points, std::begin( found));

               return Point{};
            }

            //! consumes and @returns all associated 'points' to the connection.
            auto consume( common::strong::file::descriptor::id connection)
            {
               auto consume = std::get< 1>( algorithm::partition( m_points, predicate::negate( predicate::value::equal( connection))));

               return common::algorithm::extract( m_points, consume);
            }

            //! consumes and @returns all 'points'
            auto consume() 
            {
               return std::exchange( m_points, {});
            }

            auto associated( common::strong::file::descriptor::id connection) const
            {
               return predicate::boolean( algorithm::find( m_points, connection));
            }

            const auto& points() const { return m_points;}
            bool empty() const { return m_points.empty();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_points, "points");
            )

         private:
            std::vector< Point> m_points;
         };

         struct Point
         {
            //! constructs an empty Point
            Point() = default;

            inline Point( 
               const common::Uuid& correlation, 
               common::process::Handle process, 
               common::message::Type type, 
               common::strong::file::descriptor::id connection)
               : correlation{ correlation}, process{ process}, type{ type}, connection{ connection} {}

            common::Uuid correlation;
            common::process::Handle process;
            common::message::Type type;
            common::strong::file::descriptor::id connection;

            inline explicit operator bool () const noexcept { return ! correlation.empty();}
            inline friend bool operator == ( const Point& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}
            inline friend bool operator == ( const Point& lhs, common::strong::file::descriptor::id rhs) { return lhs.connection == rhs;}
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( type);
               CASUAL_SERIALIZE( connection);
            )
         };

         using Message = basic_routing< Point>;

         namespace service
         {
            struct Point
            {  
               //! constructs an empty Point
               Point() = default;

               inline Point( const common::Uuid& correlation,
                     common::process::Handle process,
                     std::string service,
                     std::string parent,
                     platform::time::point::type start,
                     common::strong::file::descriptor::id connection)
                  : correlation( correlation), process( process), 
                     service( std::move( service)), parent( std::move( parent)), start( start), connection{ connection} {}

               common::Uuid correlation;
               common::process::Handle process;
               std::string service;
               std::string parent;
               platform::time::point::type start;
               common::strong::file::descriptor::id connection;

               inline explicit operator bool () const noexcept { return ! correlation.empty();}
               inline friend bool operator == ( const Point& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}
               inline friend bool operator == ( const Point& lhs, common::strong::file::descriptor::id rhs) { return lhs.connection == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( parent);
                  CASUAL_SERIALIZE( start);
                  CASUAL_SERIALIZE( connection);
               )
            };

            using Message = route::basic_routing< Point>;

         } // service
      } // route

   } // gateway::outbound::state
} // casual
