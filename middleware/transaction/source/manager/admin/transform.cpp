//!
//! casual
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

         vo::Statistics Statistics::operator () ( const state::Statistics& value) const
         {
            vo::Statistics result;

            result.min = value.min;
            result.max = value.max;
            result.total = value.total;
            result.invoked = value.invoked;

            return result;
         }


         vo::Stats Stats::operator () ( const state::Stats& value) const
         {
            vo::Stats result;

            result.resource = Statistics{}( value.resource);
            result.roundtrip = Statistics{}( value.roundtrip);

            return result;
         }



         struct Transaction
         {
            struct ID
            {
               vo::Transaction::ID operator () ( const common::transaction::ID& id) const
               {
                  vo::Transaction::ID result;

                  result.owner = id.owner();
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
               result.state = static_cast< long>( transaction.results());

               return result;
            }
         };


         namespace resource
         {

            vo::resource::Instance Instance::operator () ( const state::resource::Proxy::Instance& value) const
            {
               vo::resource::Instance result;

               result.id = value.id;
               result.process = value.process;
               result.state = static_cast< vo::resource::Instance::State>( value.state());
               result.statistics = transform::Stats{}( value.statistics);

               return result;
            }

            vo::resource::Proxy Proxy::operator () ( const state::resource::Proxy& value) const
            {
               vo::resource::Proxy result;

               result.id = value.id;
               result.name = value.name;
               result.key = value.key;
               result.openinfo = value.openinfo;
               result.closeinfo = value.closeinfo;
               result.statistics = transform::Stats{}( value.statistics);

               common::range::transform( value.instances, result.instances, Instance{});

               return result;
            }

         } // resource

         namespace pending
         {
            struct Request
            {
               vo::pending::Request operator () ( const state::pending::Request& value) const
               {
                  vo::pending::Request result;

                  result.resource = value.resource;
                  result.correlation = value.message.correlation;
                  result.type = common::message::convert::type( value.message.type);

                  return result;
               }
            };

            struct Reply
            {
               vo::pending::Reply operator () ( const state::pending::Reply& value) const
               {
                  vo::pending::Reply result;

                  result.queue = value.target.native();
                  result.type = common::message::convert::type( value.message.type);
                  result.correlation = value.message.correlation;

                  return result;
               }
            };

         } // pending


         vo::Log log( const transaction::Log::Stats& log)
         {
            vo::Log result;

            result.update.prepare = log.update.prepare;
            result.update.remove = log.update.remove;
            result.writes = log.writes;

            return result;
         }



         vo::State state( const State& state)
         {
            vo::State result;

            common::range::transform( state.resources, result.resources, transform::resource::Proxy{});
            common::range::transform( state.transactions, result.transactions, transform::Transaction{});

            common::range::transform( state.pending.requests, result.pending.requests, transform::pending::Request{});
            common::range::transform( state.persistent.requests, result.persistent.requests, transform::pending::Request{});
            common::range::transform( state.persistent.replies, result.persistent.replies, transform::pending::Reply{});

            result.log = transform::log( state.log.stats());

            return result;
         }

      } // transform

   } // transaction



} // casual
