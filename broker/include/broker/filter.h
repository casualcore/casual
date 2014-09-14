//!
//! filter.h
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_FILTER_H_
#define CASUAL_BROKER_FILTER_H_

#include "broker/state.h"

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

            bool operator () ( const common::message::service::name::lookup::Request& request)
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

            private:
               state::Group::id_type id;

            };

         } // group


      } // filter
   } // broker



} // casual

#endif // FILTER_H_
