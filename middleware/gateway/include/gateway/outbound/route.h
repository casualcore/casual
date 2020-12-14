//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/common.h"


#include "common/message/type.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

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
                  auto has_correlation = [&correlation]( auto& point){ return point.correlation == correlation;};

                  if( auto found = common::algorithm::find_if( m_points, has_correlation))
                     return common::algorithm::extract( m_points, std::begin( found));

                  return point_type{};
               }

               const container_type& points() const
               {
                  return m_points;
               }

               bool empty() const { return m_points.empty();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_points, "points");
               )

            private:
               container_type m_points;
            };

            struct Point
            {
               //! constructs an empty Point
               Point() = default;

               inline Point( const common::Uuid& correlation, common::process::Handle destination, common::message::Type type)
                  : correlation( correlation), destination( destination), type( type) {}

               common::Uuid correlation;
               common::process::Handle destination;
               common::message::Type type;

               inline explicit operator bool () const noexcept { return ! correlation.empty();}
               
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
                  //! constructs an empty Point
                  Point() = default;

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

                  inline explicit operator bool () const noexcept { return ! correlation.empty();}

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
