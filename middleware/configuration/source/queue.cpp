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
#include "common/exception/casual.h"



#include "serviceframework/log.h"
#include "serviceframework/archive/create.h"


namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace queue
      {
         Queue::Queue() = default;
         Queue::Queue( std::function<void( Queue&)> foreign) { foreign( *this);}


         bool operator < ( const Queue& lhs, const Queue& rhs)
         {
            return lhs.name < rhs.name;
         }

         bool operator == ( const Queue& lhs, const Queue& rhs)
         {
            return lhs.name == rhs.name;
         }

         Group::Group() = default;
         Group::Group( std::function<void( Group&)> foreign) { foreign( *this);}

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
                     {
                        group.queuebase.emplace( manager.manager_default.directory + "/" + group.name + ".qb");
                     }

                     for( auto& queue : group.queues)
                     {
                        queue.retries = coalesce( queue.retries, manager.manager_default.queue.retries);
                     }
                  }
               }

               struct Validate
               {
                  void operator ()( const Queue& queue) const
                  {
                     if( queue.name.empty())
                     {
                        throw common::exception::casual::invalid::Configuration{ "queue has to have a name"};
                     }
                  }

                  void operator ()( const Group& group) const
                  {
                     if( group.name.empty())
                     {
                        throw common::exception::casual::invalid::Configuration{ "queue group has to have a name"};
                     }

                     if( group.queuebase.value_or( "").empty())
                     {
                        throw common::exception::casual::invalid::Configuration{ "queue group has to have a queuebase path"};
                     }
                     common::algorithm::for_each( group.queues, *this);
                  }
               };

               void validate( const Manager& manager)
               {
                  common::algorithm::for_each( manager.groups, Validate{});

                  //
                  // Check unique groups
                  //
                  {
                     using G = decltype( manager.groups.front());

                     auto order_group_name = []( G lhs, G rhs){ return lhs.name < rhs.name; };
                     auto equality_group_name = []( G lhs, G rhs){ return lhs.name == rhs.name; };

                     auto groups = common::range::to_reference( manager.groups);

                     if( common::algorithm::adjacent_find( common::algorithm::sort( groups, order_group_name), equality_group_name))
                     {
                        throw common::exception::casual::invalid::Configuration{ "queue groups has to have unique names and queuebase paths"};
                     }

                     auto order_group_qb = []( G lhs, G rhs){ return lhs.queuebase < rhs.queuebase;};
                     auto equality_group_gb = []( G lhs, G rhs){ return lhs.queuebase == rhs.queuebase;};

                     // remote in-memory queues when we validate uniqueness
                     auto persitent_groups = std::get< 0>( algorithm::partition( groups, []( G g){ return g.queuebase.value_or( "") != ":memory:";}));

                     if( common::algorithm::adjacent_find( common::algorithm::sort( persitent_groups, order_group_qb), equality_group_gb))
                     {
                        throw common::exception::casual::invalid::Configuration{ "queue groups has to have unique names and queuebase paths"};
                     }
                  }

                  //
                  // Check unique queues
                  //
                  {
                     using queue_type = std::remove_reference_t< decltype( manager.groups.front().queues.front())>;
                     std::vector< std::reference_wrapper< queue_type>> queues;

                     for( auto& group : manager.groups)
                     {
                        common::algorithm::copy( group.queues, std::back_inserter( queues));
                     }

                     if( common::algorithm::adjacent_find( common::algorithm::sort( queues)))
                     {
                        throw common::exception::casual::invalid::Configuration{ "queues has to be unique"};
                     }

                  }
               }

               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = common::algorithm::find( lhs, value);

                     if( found)
                     {
                        *found = std::move( value);
                     }
                     else
                     {
                        lhs.push_back( std::move( value));
                     }
                  }
               }


               template< typename G>
               Manager& append( Manager& lhs, G&& rhs)
               {
                  local::replace_or_add( lhs.groups, std::move( rhs.groups));

                  return lhs;
               }

            } // <unnamed>
         } // local

         namespace manager
         {
            Default::Default() : directory{ "${CASUAL_DOMAIN_HOME}/queue/groups"}
            {
               queue.retries.emplace( 0);
            }
         } // manager

         Manager::Manager()
         {

         }

         void Manager::finalize()
         {
            Trace trace{ "config::queue::Manager::finalize"};

            //
            // Complement with default values
            //
            local::default_values( *this);

            //
            // Make sure we've got valid configuration
            //
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
