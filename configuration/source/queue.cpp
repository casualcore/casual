//!
//! queue.cpp
//!
//! Created on: Jun 30, 2014
//!     Author: Lazan
//!

#include "config/queue.h"
#include "config/file.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/file.h"
#include "common/exception.h"

#include "sf/log.h"
#include "sf/archive/maker.h"


namespace casual
{
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
            return lhs.name == rhs.name || lhs.queuebase == rhs.queuebase;
         }

         namespace local
         {
            namespace
            {

               void default_values( Domain& domain)
               {
                  for( auto& group : domain.groups)
                  {
                     for( auto& queue : group.queues)
                     {
                        if( queue.retries.empty())
                        {
                           queue.retries = domain.casual_default.queue.retries;
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

               void validate( const Domain& domain)
               {
                  common::range::for_each( domain.groups, Validate{});

                  auto groups = domain.groups;

                  //
                  // Check unique groups
                  //
                  {
                     auto unique = common::range::unique( common::range::sort( groups));

                     if( groups.size() != unique.size())
                     {
                        throw common::exception::invalid::Configuration{ "queue groups has to have unique names and queuebase paths"};
                     }
                  }

                  //
                  // Check unique queues
                  //
                  {
                     decltype( groups.front().queues) queues;

                     for( auto& group : groups)
                     {
                        queues.insert( std::end( queues), std::begin( group.queues), std::end( group.queues));
                     }

                     auto unique = common::range::unique( common::range::sort( queues));

                     if( queues.size() != unique.size())
                     {
                        throw common::exception::invalid::Configuration{ "queues has to be unique"};
                     }

                  }
               }

            } // <unnamed>
         } // local

         Domain get( const std::string& file)
         {
            queue::Domain domain;

            //
            // Create the reader and deserialize configuration
            //
            auto reader = sf::archive::reader::from::file( file);

            reader >> CASUAL_MAKE_NVP( domain);


            //
            // Complement with default values
            //
            local::default_values( domain);

            //
            // Make sure we've got valid configuration
            //
            local::validate( domain);


            common::log::internal::debug << CASUAL_MAKE_NVP( domain);

            return domain;

         }

         Domain get()
         {
            return get( config::file::queue());

         }


         namespace unittest
         {
            void validate( const Domain& domain)
            {
               local::validate( domain);
            }

            void default_values( Domain& domain)
            {
               local::default_values( domain);
            }
         } // unittest

      } // queue

   } // config

} // casual
