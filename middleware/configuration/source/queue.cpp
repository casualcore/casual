//!
//! casual
//!

#include "config/queue.h"
#include "config/file.h"
#include "config/common.h"

#include "common/internal/log.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/file.h"
#include "common/exception.h"

#include "sf/log.h"
#include "sf/archive/maker.h"


namespace casual
{
   using namespace common;

   namespace config
   {
      namespace queue
      {

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
            if( lhs.name == rhs.name)
               return lhs.queuebase < rhs.queuebase;

            return lhs.name < rhs.name;
         }

         bool operator == ( const Group& lhs, const Group& rhs)
         {
            return lhs.name == rhs.name || ( lhs.queuebase != ":memory:" && lhs.queuebase == rhs.queuebase);
         }

         namespace local
         {
            namespace
            {

               void default_values( Manager& manager)
               {
                  for( auto& group : manager.groups)
                  {
                     for( auto& queue : group.queues)
                     {
                        if( queue.retries.empty())
                        {
                           queue.retries = manager.casual_default.queue.retries;
                        }
                     }
                  }
               }

               struct Validate
               {
                  void operator ()( const Queue& queue) const
                  {
                     if( queue.name.empty())
                     {
                        throw common::exception::invalid::Configuration{ "queue has to have a name"};
                     }

                     if( ! common::string::integer( queue.retries))
                     {
                        throw common::exception::invalid::Configuration{ "queue has to have numeric retry set", CASUAL_NIP( queue.retries)};
                     }
                  }

                  void operator ()( const Group& group) const
                  {
                     if( group.name.empty())
                     {
                        throw common::exception::invalid::Configuration{ "queue group has to have a name"};
                     }

                     if( group.queuebase.empty())
                     {
                        throw common::exception::invalid::Configuration{ "queue group has to have a queuebase path"};
                     }
                     common::range::for_each( group.queues, *this);
                  }
               };

               void validate( const Manager& manager)
               {
                  common::range::for_each( manager.groups, Validate{});

                  //
                  // Check unique groups
                  //
                  {
                     auto groups = common::range::to_reference( manager.groups);

                     if( common::range::adjacent_find( common::range::sort( groups)))
                     {
                        throw common::exception::invalid::Configuration{ "queue groups has to have unique names and queuebase paths"};
                     }
                  }

                  //
                  // Check unique queues
                  //
                  {
                     using queue_type = common::traits::remove_reference_t< decltype( manager.groups.front().queues.front())>;
                     std::vector< std::reference_wrapper< queue_type>> queues;

                     for( auto& group : manager.groups)
                     {
                        common::range::copy( group.queues, std::back_inserter( queues));
                     }

                     if( common::range::adjacent_find( common::range::sort( queues)))
                     {
                        throw common::exception::invalid::Configuration{ "queues has to be unique"};
                     }

                  }
               }

               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = common::range::find( lhs, value);

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

         Default::Default()
         {
            queue.retries = "0";
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


         namespace transform
         {
            common::message::domain::configuration::queue::Reply manager( const Manager& value)
            {
               message::domain::configuration::queue::Reply result;

               auto transform_group = []( const config::queue::Group& g){

                  message::domain::configuration::queue::Group result;

                  result.name = g.name;
                  result.queuebase = g.queuebase;
                  result.note = g.note;

                  auto transform_queue = []( const config::queue::Queue& q){
                     message::domain::configuration::queue::Queue result;

                     result.name = q.name;
                     if( ! q.retries.empty()) { result.retries = std::stoul( q.retries );}

                     result.note = q.note;

                     return result;
                  };

                  range::transform( g.queues, result.queues, transform_queue);

                  return result;
               };

               range::transform( value.groups, result.groups, transform_group);


               return result;
            }

         } // transform


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
