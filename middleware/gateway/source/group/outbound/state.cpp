//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/state.h"

#include "common/predicate.h"
#include "common/algorithm/sorted.h"
#include "common/algorithm/random.h"

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

            namespace lookup
            {
               state::lookup::Result resource( 
                  auto& resources, 
                  auto& transactions, 
                  const std::string& key, 
                  const common::transaction::ID& trid)
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

                  if( ! trid)
                     return { get_next_connection( connections), false};

                  auto gtrid = transaction::id::range::global( trid);

                  if( auto found = algorithm::find( transactions, gtrid))
                  {
                     if( auto connection = algorithm::find_first_of( found->second, connections))
                        return { *connection, false};
                     else
                        return { found->second.emplace_back( get_next_connection( connections)), false};
                  }

                  auto connection = get_next_connection( connections);
                  transactions[ gtrid].push_back( connection);
                  
                  return { connection, true};
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

                     if( auto found = algorithm::find( connections, descriptor))
                     {
                        if( add.hops != found->hops)
                           found->hops = add.hops;
                     }
                     else
                     {
                        algorithm::random::insert( connections, state::lookup::resource::Connection{ descriptor, add.hops});
                     }

                     auto hops_less = []( auto& l, auto& r){ return l.hops < r.hops;};
                     algorithm::stable_sort( connections, hops_less);

                     // always report resource name
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
         


         lookup::Result Lookup::service( const std::string& service, const common::transaction::ID& trid)
         {
            return local::lookup::resource( m_services, m_transactions, service, trid);
         }

         lookup::Result Lookup::queue( const std::string& queue, const common::transaction::ID& trid)
         {
            return local::lookup::resource( m_queues, m_transactions, queue, trid);
         }


         Lookup::descriptor_range Lookup::connections( common::transaction::global::id::range gtrid) const noexcept
         {
            if( auto found = algorithm::find( m_transactions, gtrid))
               return range::make( found->second);

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
         
         void Lookup::remove( common::transaction::global::id::range gtrid, common::strong::file::descriptor::id descriptor)
         {
            if( auto found = algorithm::find( m_transactions, gtrid))
               if( std::empty( algorithm::container::erase( found->second, descriptor)))
                  algorithm::container::erase( m_transactions, std::begin( found));
         }

         std::vector< common::transaction::global::ID> Lookup::failed( common::strong::file::descriptor::id descriptor)
         {
            std::vector< common::transaction::global::ID> result;
            algorithm::for_each( m_transactions, [ &result, descriptor]( auto& pair)
            {
               if( auto found = algorithm::find( pair.second, descriptor))
               {
                  // nil the descriptor, to indicate error
                  *found = common::strong::file::descriptor::id{};
                  result.push_back( pair.first);
               }
            });

            return result;
         } 

         void Lookup::remove( common::transaction::global::id::range gtrid)
         {
            Trace trace{ "gateway::group::outbound::state::Lookup::remove"};
            log::line( verbose::log, "gtrid: ", gtrid);

            if( auto found = algorithm::find( m_transactions, gtrid))
            {
               log::line( verbose::log, "found: ", *found);
               algorithm::container::erase( m_transactions, std::begin( found));
            }
         }

      } // state

      state::extract::Result State::failed( common::strong::file::descriptor::id descriptor)
      {
         Trace trace{ "gateway::group::outbound::State::extract"};

         // clean the disconnecting state.
         algorithm::container::erase( disconnecting, descriptor);

         return {
            external.remove( directive, descriptor),
            route.consume( descriptor),
            lookup.failed( descriptor)
         };
      }

      bool State::done() const
      {
         if( runlevel <= state::Runlevel::running)
            return false;

         return route.empty() && lookup.empty();
      }

   } // gateway::group::outbound

} // casual