//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/outbound/transform.h"



namespace casual
{
   namespace http::outbound::transform
   {
      namespace local
      {
         namespace
         {
            auto header( const std::vector< configuration::Header>& model)
            {
               auto transform_fields = []( auto& model)
               {
                  return common::algorithm::transform( model, []( auto& field){
                     return common::service::header::Field{ field.name, field.value};
                  });
               };

               return std::make_shared< const common::service::header::Fields>( transform_fields( model));
            }
         } // <unnamed>
      } // local
      
      State configuration( configuration::Model model)
      {
         State state;

         // default headers will be used if service have 0 headers
         auto header = local::header( model.casual_default.service.headers);

         common::algorithm::for_each( model.services, [&]( auto& service){
            state::Node node;
            {
               node.url = std::move( service.url);
               if( service.headers.empty())
                  node.headers = header;
               else
                  node.headers = local::header( service.headers);

               node.discard_transaction = service.discard_transaction.value_or( 
                  model.casual_default.service.discard_transaction);
            }
            state.lookup.emplace( std::move( service.name), std::move( node));
         });
         
         return state;
      }

   } // http::outbound::transform
} // casual
