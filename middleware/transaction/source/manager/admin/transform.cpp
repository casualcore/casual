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


               admin::Metrics metrics( const state::Metrics& value)
               {
                  return local::metrics< admin::Metrics>( value);
               }

               state::Metrics metrics( const admin::Metrics& value)
               {
                  return local::metrics< state::Metrics>( value);
               }

               struct Transaction
               {
                  struct ID
                  {
                     admin::Transaction::ID operator () ( const common::transaction::ID& id) const
                     {
                        admin::Transaction::ID result;

                        result.owner = id.owner();
                        result.type = id.xid.formatID;
                        result.global = common::transcode::hex::encode( common::transaction::global( id));
                        result.branch = common::transcode::hex::encode( common::transaction::branch( id));

                        return result;
                     }
                  };

                  admin::Transaction operator () ( const manager::Transaction& transaction) const
                  {
                     admin::Transaction result;

                     common::algorithm::transform( transaction.resources, result.resources, []( const manager::Transaction::Resource& r){
                        return r.id;
                     });

                     result.trid = ID{}( transaction.trid);
                     result.state = static_cast< long>( transaction.results());

                     return result;
                  }
               };


               namespace resource
               {

                  admin::resource::Instance Instance::operator () ( const state::resource::Proxy::Instance& value) const
                  {
                     admin::resource::Instance result;

                     result.id = value.id;
                     result.process = value.process;
                     result.state = static_cast< admin::resource::Instance::State>( value.state());
                     result.metrics = transform::metrics( value.metrics);

                     return result;
                  }

                  admin::resource::Proxy Proxy::operator () ( const state::resource::Proxy& value) const
                  {
                     admin::resource::Proxy result;

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

               namespace pending
               {
                  struct Request
                  {
                     admin::pending::Request operator () ( const state::pending::Request& value) const
                     {
                        admin::pending::Request result;

                        result.resource = value.resource;
                        result.correlation = value.message.correlation;
                        result.type = common::message::convert::type( value.message.type);

                        return result;
                     }
                  };

                  struct Reply
                  {
                     admin::pending::Reply operator () ( const state::pending::Reply& value) const
                     {
                        admin::pending::Reply result;

                        result.queue = value.target;
                        result.type = common::message::convert::type( value.message.type);
                        result.correlation = value.message.correlation;

                        return result;
                     }
                  };

               } // pending


               admin::Log log( const manager::Log::Stats& log)
               {
                  admin::Log result;

                  result.update.prepare = log.update.prepare;
                  result.update.remove = log.update.remove;
                  result.writes = log.writes;

                  return result;
               }



               admin::State state( const manager::State& state)
               {
                  admin::State result;

                  common::algorithm::transform( state.resources, result.resources, transform::resource::Proxy{});
                  common::algorithm::transform( state.transactions, result.transactions, transform::Transaction{});

                  common::algorithm::transform( state.pending.requests, result.pending.requests, transform::pending::Request{});
                  common::algorithm::transform( state.persistent.requests, result.persistent.requests, transform::pending::Request{});
                  common::algorithm::transform( state.persistent.replies, result.persistent.replies, transform::pending::Reply{});

                  result.log = transform::log( state.persistent_log.stats());

                  return result;
               }

            } // transform

            
         } // admin
      } // manager
   } // transaction
} // casual
