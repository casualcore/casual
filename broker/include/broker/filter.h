//!
//! filter.h
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_FILTER_H_
#define CASUAL_BROKER_FILTER_H_


#include "broker/state.h"


#include "common/algorithm.h"
#include "common/message/server.h"

namespace casual
{
   namespace broker
   {
      namespace filter
      {
         namespace instance
         {
            struct Idle
            {
               bool operator () ( const state::Server::Instance& value) const
               {
                  return value.state == state::Server::Instance::State::idle;
               }
            };

         } // instance

         struct Pending
         {
            Pending( const state::Server::Instance& instance) : m_instance( instance) {}

            bool operator () ( const common::message::service::lookup::Request& request)
            {
               for( auto& service : m_instance.services)
               {
                  if( service.get().information.name == request.requested)
                  {
                     return true;
                  }
               }
               return false;
            }
            const state::Server::Instance& m_instance;
         };

         namespace group
         {

            struct Name
            {
               Name( std::string name) : name( std::move( name)) {}

               bool operator () ( const state::Group& group) const
               {
                  return group.name == name;
               }

            private:
               std::string name;
            };



            struct Id
            {
               Id( state::Group::id_type id) : id( std::move( id)) {}

               bool operator () ( const state::Group& group) const
               {
                  return group.id == id;
               }

               bool operator () ( const state::Executable& value) const
               {
                  return ! common::range::find( value.memberships, id).empty();
               }

            private:
               state::Group::id_type id;

            };

         } // group

         struct Booted : state::Base
         {
            using state::Base::Base;

            bool operator () ( const state::Server::Instance& instance) const
            {
               return instance.state != state::Server::Instance::State::booted;
            }

            bool operator () ( const state::Server& server) const
            {
               for( auto&& pid : server.instances)
               {
                  if( ! (*this)( m_state.getInstance( pid)))
                  {
                     return false;
                  }
               }
               return true;
            }
         };

      } // filter
   } // broker



} // casual

#endif // FILTER_H_
