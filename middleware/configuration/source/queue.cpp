//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/queue.h"

#include "configuration/common.h"
#include "configuration/file.h"

#include "common/environment.h"
#include "common/algorithm.h"
#include "common/file.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "serviceframework/log.h"
#include "common/serialize/create.h"


namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace queue
      {
         Queue& Queue::operator += ( const Default& value)
         {
            if( value.retry)
            {
               if( retry)
               {
                  retry.value().count = coalesce( retry.value().count, value.retry.value().count);
                  retry.value().delay = coalesce( retry.value().delay, value.retry.value().delay);
               }
               else 
                  retry = value.retry;
            }

            retries = coalesce( retries, value.retries);

            return *this;
         }

         bool operator < ( const Queue& lhs, const Queue& rhs)
         {
            return lhs.name < rhs.name;
         }

         bool operator == ( const Queue& lhs, const Queue& rhs)
         {
            return lhs.name == rhs.name;
         }

         bool operator < ( const Group& lhs, const Group& rhs)
         {
            return lhs.name < rhs.name;
         }

         bool operator == ( const Group& lhs, const Group& rhs)
         {
            return lhs.name == rhs.name; // || ( lhs.queuebase != ":memory:" && lhs.queuebase == rhs.queuebase);
         }

         namespace local
         {
            namespace
            {
               void default_values( Manager& manager)
               {
                  for( auto& group : manager.groups)
                  {
                     if( ! group.queuebase)
                        group.queuebase.emplace( manager.manager_default.directory + "/" + group.name + ".qb");

                     algorithm::for_each( group.queues, [&]( auto& queue)
                     {
                        queue += manager.manager_default.queue;
                     });
                  }


                  auto unique_alias = [mapping = std::map< std::string, platform::size::type>{}]( auto& forward) mutable
                  {
                     if( forward.alias.empty())
                        forward.alias = forward.source.name;

                     auto set_unique_alias = [&mapping]( auto& forward)
                     {
                        auto count = mapping[ forward.alias]++;
                        
                        if( count == 0)
                           return true;

                        forward.alias += string::compose( '.', count);
                        return false;
                     };

                     while( ! set_unique_alias( forward))
                        ; // iterate 
                  };

                  auto& defaults = manager.manager_default.forward;

                  auto update_service = [&unique_alias, &defaults]( auto& forward)
                  {
                     unique_alias( forward);

                     if( ! forward.instances)
                        forward.instances = defaults.service.instances;
                     
                     if( forward.reply)
                        forward.reply.value().delay = coalesce( forward.reply.value().delay, defaults.service.reply.delay);
                  };

                  auto update_queue = [&unique_alias, &defaults]( auto& forward)
                  {
                     unique_alias( forward);

                     if( ! forward.instances)
                        forward.instances = defaults.queue.instances;
                     
                     forward.target.delay = coalesce( forward.target.delay, defaults.queue.target.delay);
                  };

                  algorithm::for_each( manager.forward.services, update_service);
                  algorithm::for_each( manager.forward.queues, update_queue);
               }

               struct Validate
               {
                  void operator ()( const Queue& queue) const
                  {
                     if( queue.name.empty())
                        code::raise::error( code::casual::invalid_configuration, "queue has to have a name");

                     if( queue.retries)
                        log::line( log::category::error, "configuration - queue.retries is deprecated - use queue.retry.count instead");
                  }

                  void operator ()( const Group& group) const
                  {
                     if( group.name.empty())
                        code::raise::error( code::casual::invalid_configuration, "queue group has to have a name");

                     if( group.queuebase.value_or( "").empty())
                        code::raise::error( code::casual::invalid_configuration, "queue group has to have a queuebase path");

                     common::algorithm::for_each( group.queues, *this);
                  }
               };

               void validate( const Manager& manager)
               {
                  common::algorithm::for_each( manager.groups, Validate{});

                  // Check unique groups
                  {
                     using G = decltype( manager.groups.front());

                     auto order_group_name = []( G lhs, G rhs){ return lhs.name < rhs.name; };
                     auto equality_group_name = []( G lhs, G rhs){ return lhs.name == rhs.name; };

                     auto groups = common::range::to_reference( manager.groups);

                     if( common::algorithm::adjacent_find( common::algorithm::sort( groups, order_group_name), equality_group_name))
                        code::raise::error( code::casual::invalid_configuration, "queue groups has to have unique names and queuebase paths");

                     auto order_group_qb = []( G lhs, G rhs){ return lhs.queuebase < rhs.queuebase;};
                     auto equality_group_gb = []( G lhs, G rhs){ return lhs.queuebase == rhs.queuebase;};

                     // remove in-memory queues when we validate uniqueness
                     auto persitent_groups = algorithm::filter( groups, []( G g){ return g.queuebase.value_or( "") != ":memory:";});

                     if( common::algorithm::adjacent_find( common::algorithm::sort( persitent_groups, order_group_qb), equality_group_gb))
                        code::raise::error( code::casual::invalid_configuration, "queue groups has to have unique names and queuebase paths");
                  }

                  // Check unique queues
                  {
                     using queue_type = std::remove_reference_t< decltype( manager.groups.front().queues.front())>;
                     std::vector< std::reference_wrapper< queue_type>> queues;

                     for( auto& group : manager.groups)
                        common::algorithm::append( group.queues, queues);

                     if( common::algorithm::adjacent_find( common::algorithm::sort( queues)))
                        code::raise::error( code::casual::invalid_configuration, "queues has to be unique");
                  }
               }

               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     if( auto found = common::algorithm::find( lhs, value))
                        *found = std::move( value);
                     else
                        lhs.push_back( std::move( value));
                  }
               }


               template< typename G>
               Manager& append( Manager& lhs, G&& rhs)
               {
                  // defaults just propagates to the left...
                  lhs.manager_default = std::move( rhs.manager_default);

                  local::replace_or_add( lhs.groups, std::move( rhs.groups));

                  local::replace_or_add( lhs.forward.services, std::move( rhs.forward.services));
                  local::replace_or_add( lhs.forward.queues, std::move( rhs.forward.queues));

                  return lhs;
               }

            } // <unnamed>
         } // local

         Manager::Default::Default() : directory{ "${CASUAL_DOMAIN_HOME}/queue/groups"}
         {
         }

         void Manager::finalize()
         {
            Trace trace{ "config::queue::Manager::finalize"};

            // Complement with default values
            local::default_values( *this);

            // Make sure we've got valid configuration
            local::validate( *this);
         }

         Manager& Manager::operator += ( const Manager& rhs)
         {
            local::append( *this, rhs);
            return *this;
         }

         Manager& Manager::operator += ( Manager&& rhs)
         {
            local::append( *this, std::move( rhs));
            return *this;
         }

         Manager operator + ( const Manager& lhs, const Manager& rhs)
         {
            auto result = lhs;
            result += rhs;
            return result;
         }

         namespace unittest
         {
            void validate( const Manager& manager)
            {
               local::validate( manager);
            }

            void default_values( Manager& manager)
            {
               local::default_values( manager);
            }
         } // unittest

      } // queue

   } // config

} // casual
