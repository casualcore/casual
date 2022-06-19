//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/state.h"

#include "common/predicate.h"
#include "common/algorithm/sorted.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::outbound
   {
      namespace local
      {
         namespace
         {
            namespace global
            {
               const transaction::ID trid;
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
                  return common::predicate::conjunction(
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

               state::lookup::Mapping& transaction( std::vector< state::lookup::Mapping>& transactions, const transaction::ID& internal)
               {
                  if( auto mapping = algorithm::find_if( transactions, predicate::is_internal( internal)))
                     return *mapping;

                  return transactions.emplace_back( internal);
               }

               template< typename R>
               state::Lookup::Result resource( 
                  R& resources, 
                  std::vector< state::lookup::Mapping>& transactions, 
                  const std::string& key, 
                  const common::transaction::ID& internal)
               {
                  auto resource = algorithm::find( resources, key);

                  if( ! resource)
                     return {};

                  auto connections = range::make( resource->second);

                  auto get_next_connection = []( auto& connections)
                  {
                     // TODO maintainence - do we gain anything to have a pre constructed range with lowest hops?
                     auto hops_less = []( auto& l, auto& r){ return l.hops < r.hops;};
                     auto range = std::get< 0>( algorithm::sorted::upper_bound( connections, range::front( connections), hops_less));
                     auto result = range->id;
                     algorithm::rotate( range, std::begin( range) + 1);
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
               template< typename R, typename A>
               auto resource( common::strong::file::descriptor::id descriptor, R& resources, A added) 
               {
                  return algorithm::accumulate( added, std::vector< std::string>{}, [descriptor, &resources]( auto result, auto& add)
                  {
                     auto& connections = resources[ add.name];

                     // find the current lowest _hops_ for the service|queue, if any.
                     auto lowest_hops = [&connections]()
                     {
                        if( connections.empty())
                           return std::numeric_limits< platform::size::type>::max();
                        return connections.front().hops;
                     }();

                     if( auto found = algorithm::find( connections, descriptor))
                     {
                        if( add.hops != found->hops)
                           found->hops = add.hops;
                     }
                     else
                     {
                        connections.push_back( state::lookup::resource::Connection{ descriptor, add.hops});
                     }

                     auto hops_less = []( auto& l, auto& r){ return l.hops < r.hops;};
                     algorithm::stable_sort( connections, hops_less);

                     // If the _hops_ has changed, we add service for (re-)advertise, so service-manager
                     // gets to know the new hops.
                     if( connections.front().hops != lowest_hops)
                        result.push_back( std::move( add.name));
                     
                     return result;
                  });
               }
               
            } // add

            namespace remove
            {
               template< typename R>
               auto connection( common::strong::file::descriptor::id descriptor, R& resources) 
               {
                  std::vector< std::string> result;

                  algorithm::container::erase_if( resources, [descriptor, &result]( auto& pair)
                  {
                     auto& connections = pair.second;

                     if( ! algorithm::container::trim( connections, algorithm::remove( connections, descriptor)).empty())
                        return false;
                     
                     result.push_back( pair.first);
                     return true;   
                  });

                  return result;
               }

               template< typename R>
               auto connection( common::strong::file::descriptor::id descriptor, R& resources, const std::vector< std::string>& keys) 
               {
                  return algorithm::accumulate( keys, std::vector< std::string>{}, [descriptor, &resources]( auto result, auto& key)
                  {
                     if( auto found = algorithm::find( resources, key))
                     {
                        // if connections become 'absent' we remove the 'resource' and add key to the 'unadvertise-directive'
                        if( algorithm::container::trim( found->second, algorithm::remove( found->second, descriptor)).empty())
                        {
                           resources.erase( std::begin( found));
                           result.push_back( key);
                        }
                     }

                     return result;
                  });
               }
               
            } // remove
            
         } // <unnamed>
      } // local

      namespace state
      {
         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
               case Runlevel::error: return "error";
            }
            return "<unknown>";
         }
         
         namespace lookup
         {
            const Mapping::External& Mapping::branch( common::strong::file::descriptor::id connection)
            {
               return externals.emplace_back( connection, transaction::id::branch( internal));
            }
            
         } // lookup


         Lookup::Result Lookup::service( const std::string& service, const common::transaction::ID& trid)
         {
            return local::lookup::resource( m_services, m_transactions, service, trid);
         }

         Lookup::Result Lookup::queue( const std::string& queue, const common::transaction::ID& trid)
         {
            return local::lookup::resource( m_queues, m_transactions, queue, trid);
         }

         const common::transaction::ID& Lookup::external( const common::transaction::ID& internal, common::strong::file::descriptor::id connection) const
         {
            if( auto found = algorithm::find_if( m_transactions, local::predicate::is_internal( internal)))
            {
               if( auto external = algorithm::find( found->externals, connection))
                  return external->trid;
            }
            
            return local::global::trid;
         }

         const common::transaction::ID& Lookup::internal( const common::transaction::ID& external) const
         {
            if( auto found = algorithm::find_if( m_transactions, local::predicate::is_external( external)))
               return found->internal;

            return local::global::trid;
         }

         common::strong::file::descriptor::id Lookup::connection( const common::transaction::ID& external) const
         {
            auto global = algorithm::find_if( m_transactions, local::predicate::is_global( external));

            while( global)
            {
               if( auto found = algorithm::find( global->externals, external))
                  return found->connection;

               global = algorithm::find_if( ++global, local::predicate::is_global( external));
            }
                  
            return {};
         }

         lookup::Resources Lookup::resources() const
         {
            return {
               algorithm::transform( m_services, predicate::adapter::first()),
               algorithm::transform( m_queues, predicate::adapter::first())
            };
         }


         lookup::Resources Lookup::add( 
            common::strong::file::descriptor::id descriptor, 
            std::vector< lookup::Resource> services, 
            std::vector< lookup::Resource> queues)
         {
            return {
               local::add::resource( descriptor, Lookup::m_services, std::move( services)),
               local::add::resource( descriptor, Lookup::m_queues, std::move( queues))
            };
         }

         lookup::Resources Lookup::remove( common::strong::file::descriptor::id descriptor)
         {
            return {
               local::remove::connection( descriptor, m_services),
               local::remove::connection( descriptor, m_queues)
            };
         }

         lookup::Resources Lookup::remove( common::strong::file::descriptor::id descriptor, std::vector< std::string> services, std::vector< std::string> queues)
         {
            return {
               local::remove::connection( descriptor, Lookup::m_services, services),
               local::remove::connection( descriptor, Lookup::m_queues, queues)
            };
         }

         lookup::Resources Lookup::clear()
         {
            auto result = resources();

            m_services.clear();
            m_queues.clear();

            return result;
         }  

         void Lookup::remove( const common::transaction::ID& external)
         {
            Trace trace{ "gateway::group::outbound::state::Lookup::remove"};
            log::line( verbose::log, "external: ", external);

            auto remove_external = [&external]( auto& transaction)
            {
               if( transaction::id::range::global( transaction.internal) != transaction::id::range::global( external))
                  return false;

               if( auto found = algorithm::find( transaction.externals, external))
               {
                  transaction.externals.erase( std::begin( found));
                  return true;
               }
               
               return false;
            };

            if( auto found = algorithm::find_if( m_transactions, remove_external))
            {
               if( found->externals.empty())
                  m_transactions.erase( std::begin( found));
            }
            else 
               log::line( log::category::error, code::casual::invalid_semantics, " failed to correlate the external trid: ", external, " - action: ignore");

            log::line( verbose::log, "transactions: ", m_transactions);
         }

      } // state

      bool State::done() const
      {
         if( runlevel <= state::Runlevel::running)
            return false;

         return route.empty();
      }

   } // gateway::group::outbound

} // casual