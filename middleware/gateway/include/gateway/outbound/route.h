//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/common.h"


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
                     common::log::line( verbose::log, "routing: ", *this);
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

               bool empty() const { return m_points.empty();}

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE_NAME( m_points, "points");
               })

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
               
               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( destination);
                  CASUAL_SERIALIZE( type);
               })
            };

            using Route = basic_routing< Point>;

            namespace service
            {
               struct Point
               {
                  
                  inline Point( const common::Uuid& correlation,
                        common::process::Handle destination,
                        std::string service,
                        std::string parent,
                        platform::time::point::type start)
                     : correlation( correlation), destination( destination), 
                        service( std::move( service)), parent( std::move( parent)), start( start) {}

                  common::Uuid correlation;
                  common::process::Handle destination;
                  std::string service;
                  std::string parent;
                  platform::time::point::type start;

                  CASUAL_LOG_SERIALIZE(
                  { 
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( destination);
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( parent);
                     CASUAL_SERIALIZE( start);
                  })
               };

               using Route = basic_routing< Point>;

            } // service
         } // route
         
      } // outbound
   } // gateway
} // casual
