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


         /*
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
         */

         /*
         std::vector< std::vector< broker::Group::id_type>> bootOrder( State& state)
         {
            std::vector< std::vector< broker::Group::id_type>> result;

            std::vector< broker::Group> groups = state.groups;

            common::range::sort( groups);

            //
            // First round we try to find 'root' groups, i.e. without any dependencies...
            //
            auto start = std::begin( groups);
            auto parents_end = std::begin( groups);

            while( start != std::end( groups))
            {
               //
               // partition groups that has all dependencies resolved,
               // that is, we have already process them [begin, parents_end)
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
         */

         namespace transform
         {

            broker::Group Group::operator() ( const config::domain::Group& group) const
            {
               broker::Group result;

               result.name = group.name;
               result.note = group.note;

               if( ! group.resource.key.empty())
               {
                 broker::Group::Resource resource;
                 resource.instances = std::stoul( group.resource.instances);
                 resource.key = group.resource.key;
                 resource.openinfo = group.resource.openinfo;
                 resource.closeinfo = group.resource.closeinfo;
                 result.resource.emplace_back( std::move( resource));
               }

               return result;
            }

         } // transform

         namespace remove
         {
            void group( State& state, const std::string& name)
            {
               auto group = common::range::find_if( state.groups, find::group::Name( name));

               if( group)
               {
                  for( auto& server : state.servers)
                  {
                     auto found = common::range::find( server.second->memberships, group->id);

                     if( found)
                     {
                        server.second->memberships.erase( found.first);
                     }
                  }
                  state.groups.erase( group.first);
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
            namespace resolve
            {
               struct Dependencies : Base
               {
                  using Base::Base;

                  void operator () ( const config::domain::Group& group) const
                  {
                     auto found = common::range::find_if( m_state.groups, find::group::Name{ group.name});

                     if( found)
                     {
                        for( auto&& dependency : group.dependencies)
                        {
                           auto name = common::range::find_if( m_state.groups, find::group::Name{ dependency});

                           if( name)
                           {
                              found->dependencies.push_back( name->id);

                           }
                        }
                     }
                  }

               };
            } // resolve



            void groups( State& state, const std::vector< config::domain::Group>& groups)
            {
               common::range::transform( groups, state.groups, transform::Group{});

               common::range::for_each( groups, resolve::Dependencies{ state});
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

         namespace boot
         {

            void instance( State& state, const std::shared_ptr< broker::Server>& server)
            {

               auto pid = common::process::spawn( server->path, server->arguments);

               auto instance = std::make_shared< broker::Server::Instance>();
               instance->pid = pid;
               instance->server = server;
               instance->alterState( broker::Server::Instance::State::prospect);

               server->instances.push_back( instance);

               state.instances.emplace( pid, instance);
            }
         }

      } // action
   } // broker
} // casual
