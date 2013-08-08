//!
//! action.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#include "broker/action.h"

#include "common/process.h"


// TODO: tmp
#include <iostream>

namespace casual
{
   namespace broker
   {
      namespace action
      {
         namespace server
         {

            std::vector< common::platform::pid_type> start( const config::domain::Server& server)
            {
               std::vector< common::platform::pid_type> result;

               for( auto count = std::stol( server.instances); count > 0; --count)
               {
                  auto pid = common::process::spawn( server.path, common::string::split( server.arguments));
                  result.push_back( pid);
               }
               return result;
            }

         } // server



         void addGroups( State& state, const config::domain::Domain& domain)
         {
            for( auto& g : domain.groups)
            {
               auto group = std::make_shared< broker::Group>();
               group->name = g.name;
               group->note = g.note;
               if( !g.resource.key.empty())
               {
                  Group::Resource resource;
                  resource.key = g.resource.key;
                  resource.openinfo = g.resource.openinfo;
                  resource.closeinfo = g.resource.closeinfo;
                  group->resource.emplace_back( std::move( resource));
               }
               state.groups.emplace( group->name, std::move( group));
            }
            for( auto& g : domain.groups)
            {
               auto& group = state.groups.at( g.name);
               for( auto& dependency : g.dependecies)
               {
                  group->dependencies.push_back( state.groups.at( dependency));
               }
            }
         }

         namespace local
         {
            struct Dependency
            {
               typedef std::shared_ptr< broker::Group> value_type;
               typedef std::vector< value_type>::iterator group_iterator;

               Dependency( group_iterator start, group_iterator end) : start( start), end( end) {}


               bool operator () ( const std::shared_ptr< broker::Group>& value) const
               {
                  if( start == end)
                  {
                     return value->dependencies.empty();
                  }
                  return std::all_of(
                        std::begin( value->dependencies),
                        std::end( value->dependencies),
                        [=]( const value_type& value){ return std::find( start, end, value) != end;});
               }
            private:
               group_iterator start;
               group_iterator end;
            };
         } // local

         std::vector< std::vector< std::shared_ptr< broker::Group>>> bootOrder( State& state)
         {
            std::vector< std::vector< std::shared_ptr< broker::Group> > > result;

            std::vector< std::shared_ptr< broker::Group> > groups;

            for( auto& g : state.groups)
            {
               groups.push_back( g.second);
            }

            //
            // Find those that has no dependency first
            //
            auto start = std::begin( groups);

            //
            // First round we try to find 'root' groups, i.e. without any dependencies...
            //
            auto parents_end = std::begin( groups);

            while( start != std::end( groups))
            {
               //
               // partition groups that has all dependencies resolved, that is, we have already process them [begin, parents_end)
               //
               auto end = std::stable_partition(
                     start,
                     std::end( groups), local::Dependency( std::begin( groups), parents_end));

               if( end == start)
               {
                  throw common::exception::NotReallySureWhatToNameThisException( "there is a gap in the group dependency graph, or there is no 'root'");
               }

               result.emplace_back( start, end);

               parents_end = end;

               start = end;
            }

            return result;
         }



      } // action
   } // broker
} // casual


