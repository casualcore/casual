//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/reverse/outbound/state.h"

#include "common/predicate.h"

namespace casual
{
   namespace gateway::reverse::outbound
   {
      using namespace ::casual::common;

      namespace local
      {
         namespace
         {
            namespace global
            {
               const common::transaction::ID trid;
               
            } // global

            namespace predicate
            {
               auto is_internal( const transaction::ID& internal)
               {
                  return [&internal]( auto& mapping){ return mapping.internal == internal;};
               }

               auto is_global( const transaction::ID& trid)
               {
                  return [&trid]( auto& mapping)
                  { 
                     return transaction::id::range::global( trid) == transaction::id::range::global( mapping.internal);
                  };
               }

               auto is_external( const transaction::ID& external)
               {
                  return common::predicate::make_and(
                     is_global( external),
                     [&external]( auto& mapping)
                     {
                        return ! algorithm::find( mapping.externals, external).empty();
                     }
                  );
               }
            } // predicate

            namespace lookup
            {

               state::Lookup::Mapping& transaction( std::vector< state::Lookup::Mapping>& transactions, const transaction::ID& internal)
               {
                  if( auto mapping = algorithm::find_if( transactions, predicate::is_internal( internal)))
                     return *mapping;

                  return transactions.emplace_back( internal);
               }

               template< typename R>
               state::Lookup::Result resource( 
                  R& resources, 
                  std::vector< state::Lookup::Mapping>& transactions, 
                  const std::string& key, 
                  const common::transaction::ID& internal)
               {
                  auto resource = algorithm::find( resources, key);

                  if( ! resource)
                     return{};

                  auto connections = range::make( resource->second);

                  auto get_next_connection = []( auto& connections)
                  {
                     auto result = range::front( connections);
                     algorithm::rotate( connections, std::begin( connections) + 1);
                     return result;
                  };

                  if( ! internal)
                     return { { get_next_connection( connections), {}}, false};

                  auto& mapping = lookup::transaction( transactions, internal);

                  // if we got a connection that has the service AND is associated with the 
                  // transaction before, we use it.
                  if( auto found = std::get< 0>( algorithm::intersection( mapping.externals, connections)))
                     return { *found, false};

                  // we create a new branch for the connection.
                  return { mapping.branch( get_next_connection( connections)), true};
               }

            } // lookup

            namespace add
            {
               template< typename R>
               auto resource( common::strong::file::descriptor::id descriptor, R& resources, const std::vector< std::string>& keys) 
               {
                  std::vector< std::string> result;

                  algorithm::copy_if( keys, result, [descriptor, &resources]( auto& key)
                  {
                     auto& connections = resources[ key];
                     connections.push_back( descriptor);

                     return range::size( connections) == 1;
                  });

                  return result;
               }
               
            } // add

            namespace remove
            {
               template< typename R>
               auto connection( common::strong::file::descriptor::id descriptor, R& resources) 
               {
                  std::vector< std::string> result;

                  algorithm::erase_if( resources, [descriptor, &result]( auto& pair)
                  {
                     auto& connections = pair.second;

                     if( ! algorithm::trim( connections, algorithm::remove( connections, descriptor)).empty())
                        return false;
                     
                     result.push_back( pair.first);
                     return true;   
                  });

                  return result;
               }

               template< typename R>
               auto connection( common::strong::file::descriptor::id descriptor, R& resources, const std::vector< std::string>& keys) 
               {
                  std::vector< std::string> result;

                  algorithm::copy_if( keys, result, [descriptor, &resources]( auto& key)
                  {
                     if( auto found = algorithm::find( resources, key))
                     {
                        auto& connections = found->second;
                        algorithm::trim( connections, algorithm::remove( connections, descriptor));

                        if( ! connections.empty())
                        {
                           resources.erase( std::begin( found));
                           return true;
                        }
                     }
                     return false;
                  });

                  return result;
               }
               
            } // remove
            
         } // <unnamed>
      } // local

      namespace state
      {

         const Lookup::Mapping::External& Lookup::Mapping::branch( common::strong::file::descriptor::id connection)
         {
            return externals.emplace_back( connection, transaction::id::branch( internal));
         }


         Lookup::Result Lookup::service( const std::string& service, const common::transaction::ID& trid)
         {
            return local::lookup::resource( services, transactions, service, trid);
         }

         Lookup::Result Lookup::queue( const std::string& queue, const common::transaction::ID& trid)
         {
            return local::lookup::resource( queues, transactions, queue, trid);
         }

         const common::transaction::ID& Lookup::external( const common::transaction::ID& internal, common::strong::file::descriptor::id connection) const
         {
            if( auto found = algorithm::find_if( transactions, local::predicate::is_internal( internal)))
            {
               if( auto external = algorithm::find( found->externals, connection))
                  return external->trid;
            }
            
            return local::global::trid;
         }

         const common::transaction::ID& Lookup::internal( const common::transaction::ID& external) const
         {
            if( auto found = algorithm::find_if( transactions, local::predicate::is_external( external)))
               return found->internal;

            return local::global::trid;
         }

         common::strong::file::descriptor::id Lookup::connection( const common::transaction::ID& external) const
         {
            if( auto global = algorithm::find_if( transactions, local::predicate::is_global( external)))
               if( auto found = algorithm::find( global->externals, external))
                  return found->connection;
                  
            return {};
         }


         Lookup::Advertise Lookup::add( 
            common::strong::file::descriptor::id descriptor, 
            std::vector< std::string> services, 
            std::vector< std::string> queues)
         {
            return {
               local::add::resource( descriptor, Lookup::services, services),
               local::add::resource( descriptor, Lookup::queues, queues)
            };
         }

         Lookup::Advertise Lookup::remove( common::strong::file::descriptor::id descriptor)
         {
            return {
               local::remove::connection( descriptor, services),
               local::remove::connection( descriptor, queues)
            };
         }

         Lookup::Advertise Lookup::remove( common::strong::file::descriptor::id descriptor, std::vector< std::string> services, std::vector< std::string> queues)
         {
            return {
               local::remove::connection( descriptor, Lookup::services, services),
               local::remove::connection( descriptor, Lookup::queues, queues)
            };
         }

         void Lookup::remove( const common::transaction::ID& internal)
         {
            if( auto found = algorithm::find_if( transactions, local::predicate::is_internal( internal)))
               transactions.erase( std::begin( found));
            else
               log::line( log::category::error, code::casual::invalid_semantics, " failed to correlate the internal trid: ", internal, " - action: ignore");
         }

      } // state

   } // gateway::reverse::inbound

} // casual