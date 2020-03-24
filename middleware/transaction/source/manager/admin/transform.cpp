//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/manager/admin/transform.h"

#include "common/algorithm.h"
#include "common/transcode.h"

namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace admin
         {
            namespace transform
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

               struct Branch
               {
                  struct ID
                  {
                     admin::model::Branch::ID operator () ( const common::transaction::ID& id) const
                     {
                        admin::model::Branch::ID result;

                        result.owner = id.owner();
                        result.type = id.xid.formatID;
                        result.global = common::transcode::hex::encode( common::transaction::id::range::global( id));
                        result.branch = common::transcode::hex::encode( common::transaction::id::range::branch( id));

                        return result;
                     }
                  };

                  admin::model::Branch operator () ( const manager::Transaction::Branch& branch) const
                  {
                     admin::model::Branch result;

                     common::algorithm::transform( branch.resources, result.resources, []( auto& r)
                     {
                        return r.id;
                     });

                     result.trid = ID{}( branch.trid);
                     result.state = static_cast< long>( branch.results());

                     return result;
                  }
               };

               struct Transaction 
               {
                  admin::model::Transaction operator () ( const manager::Transaction& transaction) const
                  {
                     admin::model::Transaction result;
                     result.global.id = common::transcode::hex::encode( transaction.global.global());
                     result.global.owner = transaction.owner();
                     result.state = static_cast< long>( transaction.results());

                     common::algorithm::transform( transaction.branches, result.branches, Branch{});

                     return result;
                  }
               };


               namespace resource
               {

                  admin::model::resource::Instance Instance::operator () ( const state::resource::Proxy::Instance& value) const
                  {
                     admin::model::resource::Instance result;

                     result.id = value.id;
                     result.process = value.process;
                     result.state = static_cast< admin::model::resource::Instance::State>( value.state());
                     result.metrics = transform::metrics( value.metrics);

                     return result;
                  }

                  admin::model::resource::Proxy Proxy::operator () ( const state::resource::Proxy& value) const
                  {
                     admin::model::resource::Proxy result;

                     result.id = value.id;
                     result.name = value.name;
                     result.key = value.key;
                     result.openinfo = value.openinfo;
                     result.closeinfo = value.closeinfo;
                     result.concurency = value.concurency;
                     result.metrics = transform::metrics( value.metrics);

                     common::algorithm::transform( value.instances, result.instances, Instance{});

                     return result;
                  }

               } // resource


               namespace local
               {
                  namespace
                  {
                     namespace pending
                     {
                        auto request()
                        {
                           return []( auto& value)
                           {
                              admin::model::pending::Request result;

                              result.resource = value.resource;
                              result.correlation = value.message.correlation;
                              result.type = common::message::convert::type( value.message.type);

                              return result;
                           };
                        }

                        auto reply()
                        {
                           return []( auto& value)
                           {
                              admin::model::pending::Reply result;

                              result.destinations = value.destinations;
                              result.type = common::message::convert::type( value.complete.type);
                              result.correlation = value.complete.correlation;

                              return result;
                           };
                        }
                     } // pending
                     
                  } // <unnamed>
               } // local


               admin::model::Log log( const manager::Log::Stats& log)
               {
                  admin::model::Log result;

                  result.update.prepare = log.update.prepare;
                  result.update.remove = log.update.remove;
                  result.writes = log.writes;

                  return result;
               }

               admin::model::State state( const manager::State& state)
               {
                  admin::model::State result;

                  common::algorithm::transform( state.resources, result.resources, transform::resource::Proxy{});
                  common::algorithm::transform( state.transactions, result.transactions, transform::Transaction{});

                  common::algorithm::transform( state.pending.requests, result.pending.requests, local::pending::request());
                  common::algorithm::transform( state.persistent.replies, result.pending.persistent.replies, local::pending::reply());

                  result.log = transform::log( state.persistent.log.stats());

                  return result;
               }

            } // transform

            
         } // admin
      } // manager
   } // transaction
} // casual
