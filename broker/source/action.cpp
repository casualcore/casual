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
         /*
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
         */


         namespace local
         {
            struct Dependency
            {
               typedef std::shared_ptr< broker::Group> value_type;
               typedef std::vector< value_type>::iterator group_iterator;

               Dependency( group_iterator start, group_iterator end) : start( start), end( end) {}


               bool operator () ( const value_type& value) const
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
            // First round we try to find 'root' groups, i.e. without any dependencies...
            //
            auto start = std::begin( groups);
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


         namespace remove
         {
            void group( State& state, const std::string& name)
            {
               auto group = state.groups.find( name);

               if( group != std::end( state.groups))
               {
                  state.groups.erase( group);

                  for( auto& server : state.servers)
                  {
                     auto found = std::find(
                           std::begin( server.second->memberships),
                           std::end( server.second->memberships),
                           group->second);
                     if( found != std::end( server.second->memberships))
                     {
                        server.second->memberships.erase( found);
                     }
                  }
               }
               else
               {
                  // TODO: error? "warning"?
               }

            }

            void groups( State& state, const std::vector< std::string>& remove)
            {
               for( auto& group : remove)
               {
                  remove::group( state, group);
               }
            }
         } // remove

         namespace add
         {
            namespace prepare
            {
               std::shared_ptr< broker::Group> group( State& state, const config::domain::Group& group)
               {
                  auto findGroup = state.groups.find( group.name);

                  if( findGroup == std::end( state.groups))
                  {
                     auto groupToAdd = std::make_shared< broker::Group>();
                     groupToAdd->name = group.name;
                     groupToAdd->note = group.note;

                     if( ! group.resource.key.empty())
                     {
                       Group::Resource resource;
                       resource.instances = std::stoul( group.resource.instances);
                       resource.key = group.resource.key;
                       resource.openinfo = group.resource.openinfo;
                       resource.closeinfo = group.resource.closeinfo;
                       groupToAdd->resource.emplace_back( std::move( resource));
                     }

                     findGroup = state.groups.emplace( groupToAdd->name, std::move( groupToAdd)).first;
                  }
                  return findGroup->second;
               }

            } // local

            void group( State& state, const config::domain::Group& group)
            {
               auto groupToAdd = prepare::group( state, group);

               for( auto& dependency : group.dependencies)
               {
                  groupToAdd->dependencies.push_back( state.groups.at( dependency));
               }
            }

            void groups( State& state, const std::vector< config::domain::Group>& groups)
            {
               for( auto& group : groups)
               {
                  add::prepare::group( state, group);
               }

               for( auto& group : groups)
               {
                  add::group( state, group);
               }
            }

         } // add

         namespace update
         {
            void groups( State& state, const std::vector< config::domain::Group>& update, const std::vector< std::string>& remove)
            {
               remove::groups( state, remove);
               add::groups( state, update);
            }

         } // update


      } // action
   } // broker
} // casual
