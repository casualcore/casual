//!
//! transform.cpp
//!
//! Created on: Jun 14, 2015
//!     Author: Lazan
//!

#include "transaction/manager/admin/transform.h"

#include "common/algorithm.h"
#include "common/transcode.h"

namespace casual
{
   namespace transaction
   {
      namespace transform
      {
         struct Process
         {
            vo::Process operator () ( const common::process::Handle& process) const
            {
               vo::Process result;

               result.pid = process.pid;
               result.queue = process.queue;

               return result;
            }
         };

         struct Transaction
         {
            struct ID
            {
               vo::Transaction::ID operator () ( const common::transaction::ID& id) const
               {
                  vo::Transaction::ID result;

                  result.owner = Process{}( id.owner());
                  result.type = id.xid.formatID;
                  result.global = common::transcode::hex::encode( common::transaction::global( id));
                  result.branch = common::transcode::hex::encode( common::transaction::branch( id));

                  return result;
               }
            };

            vo::Transaction operator () ( const transaction::Transaction& transaction) const
            {
               vo::Transaction result;

               common::range::transform( transaction.resources, result.resources, []( const transaction::Transaction::Resource& r){
                  return r.id;
               });

               result.trid = ID{}( transaction.trid);
               result.state = transaction::Transaction::Resource::convert( transaction.results());

               return result;
            }
         };


         namespace resource
         {
            struct Instance
            {
               vo::resource::Instance operator () ( const state::resource::Proxy::Instance& value) const
               {
                  vo::resource::Instance result;

                  result.id = value.id;
                  result.process = transform::Process{}( value.process);
                  result.state = static_cast< vo::resource::Instance::State>( value.state);

                  return result;
               }
            };

            struct Proxy
            {
               vo::resource::Proxy operator () ( const state::resource::Proxy& value) const
               {
                  vo::resource::Proxy result;

                  result.id = value.id;
                  result.key = value.key;
                  result.openinfo = value.openinfo;
                  result.closeinfo = value.closeinfo;

                  common::range::transform( value.instances, result.instances, Instance{});

                  return result;
               }

            };

         } // resource

         namespace pending
         {
            struct Reqeust
            {
               vo::pending::Request operator () ( const state::pending::Request& value) const
               {
                  vo::pending::Request result;

                  result.resources = value.resources;

                  return result;
               }
            };

            struct Reply
            {
               vo::pending::Reply operator () ( const state::pending::Reply& value) const
               {
                  vo::pending::Reply result;

                  result.queue = value.target;
                  result.type = value.message.type;
                  result.correlation = value.message.correlation;

                  return result;
               }
            };

         } // pending


         vo::State state( const State& state)
         {
            vo::State result;

            common::range::transform( state.resources, result.resources, transform::resource::Proxy{});
            common::range::transform( state.transactions, result.transactions, transform::Transaction{});

            common::range::transform( state.pendingRequests, result.pending.requests, transform::pending::Reqeust{});
            common::range::transform( state.persistentRequests, result.persistent.requests, transform::pending::Reqeust{});
            common::range::transform( state.persistentReplies, result.persistent.replies, transform::pending::Reply{});

            return result;
         }

      } // transform

   } // transaction



} // casual
