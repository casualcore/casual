//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "common/exception/system.h"
#include "common/string.h"

#include <vector>

namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         namespace route
         {
            template< typename P>
            struct basic_routing
            {
               using point_type = P;
               using container_type = std::vector< point_type>;


               template< typename... Args>
               void emplace( Args&&... args)
               {
                  m_points.emplace_back( std::forward< Args>( args)...);
               }

               template< typename M>
               void add( const M& message)
               {
                  emplace( message.correlation, message.process, common::message::type( message));
               }

               point_type get( const common::Uuid& correlation)
               {
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

               const container_type& points() const
               {
                  return m_points;
               }

               friend std::ostream& operator << ( std::ostream& out, const basic_routing& value)
               {
                  return out << "{ points: " << common::range::make( value.m_points)
                     << '}';
               }

            private:
               container_type m_points;
            };

            struct Point
            {
               inline Point( const common::Uuid& correlation, common::process::Handle destination, common::message::Type type)
                  : correlation( correlation), destination( destination), type( type) {}

               common::Uuid correlation;
               common::process::Handle destination;
               common::message::Type type;

               friend std::ostream& operator << ( std::ostream& out, const Point& value);
            };

            using Route = basic_routing< Point>;

            namespace service
            {
               struct Point
               {
                  
                  inline Point( const common::Uuid& correlation,
                        common::process::Handle destination,
                        std::string service,
                        common::platform::time::point::type start)
                     : correlation( correlation), destination( destination), service( std::move( service)), start( start) {}

                  common::Uuid correlation;
                  common::process::Handle destination;
                  std::string service;
                  common::platform::time::point::type start;

                  friend std::ostream& operator << ( std::ostream& out, const Point& value);
               };

               using Route = basic_routing< Point>;

            } // service
         } // route
         
      } // outbound
   } // gateway
} // casual