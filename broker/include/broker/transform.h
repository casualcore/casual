//!
//! transform.h
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_BROKERTRANSFORM_H_
#define CASUAL_BROKERTRANSFORM_H_

#include "broker/state.h"


#include "config/domain.h"

#include "common/message/server.h"
#include "common/message/transaction.h"


namespace casual
{
   namespace broker
   {
      namespace transform
      {
         namespace configuration
         {

            struct Service
            {
               state::Service operator () ( const config::domain::Service& service) const;
            };


            struct Executable
            {
               using groups_type = const std::vector< state::Group>&;

               Executable( groups_type groups) : m_groups( groups) {}

               state::Executable operator () ( const config::domain::Executable& executable) const;

            protected:
               groups_type m_groups;
            };


            struct Server : Executable
            {
               using Executable::Executable;

               state::Server operator () ( const config::domain::Server& server) const;
            };

            struct Resource
            {
               state::Group::Resource operator () ( const config::domain::Resource& resource) const;
            };

            struct Group
            {
               state::Group operator () ( const config::domain::Group& group) const;
            };


            struct Domain
            {
               broker::State operator () ( const config::domain::Domain& domain) const;
            };

            namespace transaction
            {
               struct Manager
               {
                  state::Server operator () ( const config::domain::transaction::Manager& manager) const;
               };

            } // transaction

         } // configuration



         struct Service
         {
            state::Service operator () ( const common::message::Service& value) const;
         };

         struct Instance
         {
            state::Server::Instance operator () ( const common::message::server::connect::Request& message) const;

            common::message::server::Id operator () ( const state::Server::Instance& value) const;

         };

         namespace transaction
         {
            struct Resource
            {
               inline common::message::transaction::resource::Manager operator () ( const state::Group::Resource& resource) const;
            };

            common::message::transaction::manager::Configuration configuration( const broker::State& state);

            namespace client
            {

               common::message::transaction::client::connect::Reply reply( broker::State& state, const state::Server::Instance& instance);

            } // client
         } // transaction




      } // transform
   } // broker
} // casual

#endif // TRANSFORM_H_
