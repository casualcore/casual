//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/manager/admin/transform.h"

#include "common/algorithm.h"
#include "common/transcode.h"

#include "casual/assert.h"

namespace casual
{
   namespace transaction::manager::admin::transform
   {

      namespace local
      {
         namespace
         {
            template< typename R, typename T>
            R metrics( const T& value)
            {
               auto transform_metric = []( auto& value)
               {
                  decltype( R{}.resource) result;

                  result.limit.min = value.limit.min;
                  result.limit.max = value.limit.max;
                  result.total = value.total;
                  result.count = value.count;

                  return result;

               };
               R result;

               result.resource = transform_metric( value.resource);
               result.roundtrip = transform_metric( value.roundtrip);

               return result;
            }

            namespace resource
            {
               admin::model::resource::Instance instance( const state::resource::Proxy::Instance& value)
               {
                  admin::model::resource::Instance result;

                  result.id = value.id;
                  result.process = value.process;
                  result.state = static_cast< admin::model::resource::Instance::State>( value.state());
                  result.metrics = transform::metrics( value.metrics);

                  return result;
               }
               
            } // resource

            admin::model::Log log( const manager::Log::Statistics& log)
            {
               admin::model::Log result;

               result.update.prepare = log.update.prepare;
               result.update.remove = log.update.remove;
               result.writes = log.writes;

               return result;
            }

            auto transaction()
            {
               return []( const state::Transaction& transaction)
               {
                  auto branch = []( auto& branch)
                  {
                     auto trid = []( auto& id)
                     {
                        admin::model::transaction::Branch::ID result;

                        result.type = id.xid.formatID;
                        result.global = common::transcode::hex::encode( common::transaction::id::range::global( id));
                        result.branch = common::transcode::hex::encode( common::transaction::id::range::branch( id));

                        return result;
                     };

                     auto resource = []( auto& resource)
                     {
                        return admin::model::transaction::branch::Resource{ resource.id, resource.code};
                     };

                     admin::model::transaction::Branch result;
                     result.trid = trid( branch.trid);
                     result.resources = common::algorithm::transform( branch.resources, resource);

                     return result;
                  };

                  auto stage = []( auto stage)
                  {
                     switch( stage)
                     {
                        case state::transaction::Stage::involved: return admin::model::transaction::Stage::involved;
                        case state::transaction::Stage::prepare: return admin::model::transaction::Stage::prepare;
                        case state::transaction::Stage::commit: return admin::model::transaction::Stage::commit;
                        case state::transaction::Stage::rollback: return admin::model::transaction::Stage::rollback;
                     }
                     casual::terminate( "unknown value for stage: ", common::cast::underlying( stage));
                  };

                  admin::model::Transaction result;
                  result.global.id = common::transcode::hex::encode( transaction.global());
                  result.owner = transaction.owner;
                  result.stage = stage( transaction.stage());


                  result.branches = common::algorithm::transform( transaction.branches, branch);

                  return result;
               };
            }

            namespace pending
            {
               auto request()
               {
                  return []( auto& value)
                  {
                     admin::model::pending::Request result;

                     result.resource = value.resource;
                     result.correlation = value.message.correlation().value();
                     result.type = common::message::convert::type( value.message.type());

                     return result;
                  };
               }

               auto reply()
               {
                  return []( auto& value)
                  {
                     admin::model::pending::Reply result;

                     result.destinations = value.destinations;
                     result.type = common::message::convert::type( value.complete.type());
                     result.correlation = value.complete.correlation().value();

                     return result;
                  };
               }
            } // pending
            
         } // <unnamed>
      } // local

      admin::model::Metrics metrics( const state::Metrics& value)
      {
         return local::metrics< admin::model::Metrics>( value);
      }

      state::Metrics metrics( const admin::model::Metrics& value)
      {
         return local::metrics< state::Metrics>( value);
      }

      namespace resource
      {

         model::resource::Proxy proxy( const state::resource::Proxy& value)
         {
            admin::model::resource::Proxy result;

            result.id = value.id;
            result.name = value.configuration.name;
            result.key = value.configuration.key;
            result.openinfo = value.configuration.openinfo;
            result.closeinfo = value.configuration.closeinfo;
            result.concurrency = value.configuration.instances;
            result.metrics = transform::metrics( value.metrics);

            common::algorithm::transform( value.instances, result.instances, &local::resource::instance);

            return result;
         }

      } // resource


      admin::model::State state( const manager::State& state)
      {
         admin::model::State result;

         common::algorithm::transform( state.resources, result.resources, &transform::resource::proxy);
         common::algorithm::transform( state.transactions, result.transactions, local::transaction());

         common::algorithm::transform( state.pending.requests, result.pending.requests, local::pending::request());
         common::algorithm::transform( state.persistent.replies, result.pending.persistent.replies, local::pending::reply());

         result.log = local::log( state.persistent.log.statistics());

         return result;
      }

   } // transaction::manager::admin::transform
} // casual
