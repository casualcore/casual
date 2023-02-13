//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/model.h"
#include "configuration/common.h"

#include "casual/assert.h"

#include "common/algorithm.h"
#include "common/algorithm/coalesce.h"
#include "common/algorithm/container.h"

namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace model
      {
         namespace local
         {
            namespace
            {
               namespace predicate
               {
                  namespace equal
                  {
                     auto alias()
                     {
                        return []( auto& l, auto& r)
                        {
                           if( l.alias.empty() && r.alias.empty())
                              return false;

                           return l.alias == r.alias;
                        };
                     }

                     auto connect()
                     {
                        return []( auto& l, auto& r)
                        {
                           return l.connect == r.connect;
                        };
                     }

                     auto name()
                     {
                        return []( auto& l, auto& r)
                        {
                           if( l.name.empty() && r.name.empty())
                              return false;
                           return l.name == r.name;
                        };
                     }

                     auto key()
                     {
                        return []( auto& l, auto& r)
                        {
                           return l.key == r.key;
                        };
                     }

                  } // equal
               } // predicate
               namespace range
               {
                  template< typename T, typename C>
                  void update( T source, T& target, C&& compare)
                  {
                     for( auto& value : source)
                     {
                        auto predicate = [compare, &value]( auto& target){ return compare( target, value);};

                        if( auto found = algorithm::find_if( target, predicate))
                           *found = set_union( *found, std::move( value));
                        else 
                           target.push_back( std::move( value));
                     }
                  }

                  template< typename T, typename C>
                  void replace( T source, T& target, C&& compare)
                  {
                     for( auto& value : source)
                     {
                        auto predicate = [compare, &value]( auto& target){ return compare( target, value);};

                        if( auto found = algorithm::find_if( target, predicate))
                           *found = std::move( value);
                        else 
                           target.push_back( std::move( value));
                     }

                  }

                  template< typename T, typename C>
                  void remove( const T source, T& target, C&& compare)
                  {
                     algorithm::container::erase( target, std::get< 0>( algorithm::intersection( target, source, compare)));
                  }

                  template< typename T, typename C>
                  void intersection( const T source, T& target, C&& compare)
                  {
                     algorithm::container::erase( target, std::get< 1>( algorithm::intersection( target, source, compare)));
                  }
   
               } // range

            } // <unnamed>
         } // local

         namespace system
         {
            Model set_union( Model lhs, Model rhs)
            {
               local::range::replace( std::move( rhs.resources), lhs.resources, local::predicate::equal::key());

               return lhs;
            }

            Model set_difference( Model lhs, Model rhs)
            {
               local::range::remove( std::move( rhs.resources), lhs.resources, local::predicate::equal::key());

               return lhs;
            }

            Model set_intersection( Model lhs, Model rhs)
            {
               local::range::intersection( std::move( rhs.resources), lhs.resources, local::predicate::equal::key());

               return lhs;
            }
         } // system


         namespace domain
         {
            Environment set_union( Environment lhs, Environment rhs)
            {
               local::range::replace( std::move( rhs.variables), lhs.variables, []( auto& l, auto& r){ return l.name() == r.name();});

               return lhs;
            }

            Environment set_difference( Environment lhs, Environment rhs)
            {
               local::range::remove( std::move( rhs.variables), lhs.variables, []( auto& l, auto& r){ return l.name() == r.name();});

               return lhs;
            }

            Environment set_intersection( Environment lhs, Environment rhs)
            {
               local::range::intersection( std::move( rhs.variables), lhs.variables, []( auto& l, auto& r){ return l.name() == r.name();});

               return lhs;
            }

            Model set_union( Model lhs, Model rhs)
            {
               lhs.name = algorithm::coalesce( std::move( rhs.name), std::move( lhs.name));

               lhs.environment = set_union( lhs.environment, std::move( rhs.environment));

               local::range::replace( std::move( rhs.groups), lhs.groups, local::predicate::equal::name());
               local::range::replace( std::move( rhs.servers), lhs.servers, local::predicate::equal::alias());
               local::range::replace( std::move( rhs.executables), lhs.executables, local::predicate::equal::alias());

               return lhs;
            }

            Model set_difference( Model lhs, Model rhs)
            {
               lhs.environment = set_difference( lhs.environment, std::move( rhs.environment));

               local::range::remove( std::move( rhs.groups), lhs.groups, local::predicate::equal::name());
               local::range::remove( std::move( rhs.servers), lhs.servers, local::predicate::equal::alias());
               local::range::remove( std::move( rhs.executables), lhs.executables, local::predicate::equal::alias());

               return lhs;
            }

            Model set_intersection( Model lhs, Model rhs)
            {
               lhs.environment = set_intersection( lhs.environment, std::move( rhs.environment));

               local::range::intersection( std::move( rhs.groups), lhs.groups, local::predicate::equal::name());
               local::range::intersection( std::move( rhs.servers), lhs.servers, local::predicate::equal::alias());
               local::range::intersection( std::move( rhs.executables), lhs.executables, local::predicate::equal::alias());

               return lhs;
            }
            
         } // domain
         

         namespace transaction
         {
            Model set_union( Model lhs, Model rhs)
            {
               lhs.log = algorithm::coalesce( std::move( rhs.log), std::move( lhs.log));
               local::range::replace( std::move( rhs.resources), lhs.resources, local::predicate::equal::name());
               local::range::replace( std::move( rhs.mappings), lhs.mappings, local::predicate::equal::alias());

               return lhs;
            }

            Model set_difference( Model lhs, Model rhs)
            {
               local::range::remove( std::move( rhs.resources), lhs.resources, local::predicate::equal::name());
               local::range::remove( std::move( rhs.mappings), lhs.mappings, local::predicate::equal::alias());

               return lhs;
            }

            Model set_intersection( Model lhs, Model rhs)
            {
               local::range::intersection( std::move( rhs.resources), lhs.resources, local::predicate::equal::name());
               local::range::intersection( std::move( rhs.mappings), lhs.mappings, local::predicate::equal::alias());

               return lhs;
            }
            
         } // transaction

         namespace service
         {
            Restriction set_union( Restriction lhs, Restriction rhs)
            {
               local::range::replace( std::move( rhs.servers), lhs.servers, local::predicate::equal::alias());
               return lhs;
            }

            Restriction set_difference( Restriction lhs, Restriction rhs)
            {
               local::range::remove( std::move( rhs.servers), lhs.servers, local::predicate::equal::alias());
               return lhs;
            }

            Restriction set_intersection( Restriction lhs, Restriction rhs)
            {
               local::range::intersection( std::move( rhs.servers), lhs.servers, local::predicate::equal::alias());
               return lhs;
            }

            Timeout set_union( Timeout lhs, Timeout rhs)
            {
               if( rhs.duration)
                  lhs.duration = std::move( rhs.duration);

               lhs.contract = rhs.contract;
               return lhs;
            }

            Global set_union( Global lhs, Global rhs)
            {
               lhs.timeout = set_union( lhs.timeout, std::move( rhs.timeout));
               return lhs;
            }

            Model set_union( Model lhs, Model rhs)
            {
               lhs.global = set_union( lhs.global, std::move( rhs.global));

               local::range::replace( std::move( rhs.services), lhs.services, local::predicate::equal::name());

               lhs.restriction = set_union( lhs.restriction, std::move( rhs.restriction));
               
               return lhs;
            }

            Model set_difference( Model lhs, Model rhs)
            {
               local::range::remove( std::move( rhs.services), lhs.services, local::predicate::equal::name());
               
               return lhs;
            }

            Model set_intersection( Model lhs, Model rhs)
            {
               local::range::intersection( std::move( rhs.services), lhs.services, local::predicate::equal::name());
               
               return lhs;
            }
         } // service

         namespace queue
         {
            Group set_union( Group lhs, Group rhs)
            {
               lhs.note = algorithm::coalesce( std::move( rhs.note), std::move( lhs.note));
               local::range::replace( std::move( rhs.queues), lhs.queues, local::predicate::equal::name());
               
               return lhs;
            }

            Group set_difference( Group lhs, Group rhs)
            {
               local::range::remove( std::move( rhs.queues), lhs.queues, local::predicate::equal::name());

               return lhs;
            }

            Group set_intersection( Group lhs, Group rhs)
            {
               local::range::intersection( std::move( rhs.queues), lhs.queues, local::predicate::equal::name());

               return lhs;
            }

            namespace forward
            { 
               Group set_union( Group lhs, Group rhs)
               {
                  lhs.note = algorithm::coalesce( std::move( rhs.note), std::move( lhs.note));
                  local::range::replace( std::move( rhs.services), lhs.services, local::predicate::equal::alias());
                  local::range::replace( std::move( rhs.queues), lhs.queues, local::predicate::equal::alias());

                  return lhs;
               }

               Group set_difference( Group lhs, Group rhs)
               {
                  local::range::remove( std::move( rhs.services), lhs.services, local::predicate::equal::alias());
                  local::range::remove( std::move( rhs.queues), lhs.queues, local::predicate::equal::alias());

                  return lhs;
               }

               Group set_intersection( Group lhs, Group rhs)
               {
                  local::range::intersection( std::move( rhs.services), lhs.services, local::predicate::equal::alias());
                  local::range::intersection( std::move( rhs.queues), lhs.queues, local::predicate::equal::alias());

                  return lhs;
               }
            } // forward

            Forward set_union( Forward lhs, Forward rhs)
            {
               local::range::update( std::move( rhs.groups), lhs.groups, local::predicate::equal::alias());
               
               return lhs;
            }

            Forward set_difference( Forward lhs, Forward rhs)
            {
               local::range::remove( std::move( rhs.groups), lhs.groups, local::predicate::equal::alias());
               
               return lhs;
            }

            Forward set_intersection( Forward lhs, Forward rhs)
            {
               local::range::intersection( std::move( rhs.groups), lhs.groups, local::predicate::equal::alias());
               
               return lhs;
            }

            Model set_union( Model lhs, Model rhs)
            {
               lhs.note = algorithm::coalesce( std::move( rhs.note), std::move( lhs.note));

               local::range::update( std::move( rhs.groups), lhs.groups, local::predicate::equal::alias());
               lhs.forward = set_union( lhs.forward, std::move( rhs.forward));

               return lhs;
            }

            Model set_difference( Model lhs, Model rhs)
            {
               local::range::remove( std::move( rhs.groups), lhs.groups, local::predicate::equal::alias());
               lhs.forward = set_difference( lhs.forward, std::move( rhs.forward));

               return lhs;
            }

            Model set_intersection( Model lhs, Model rhs)
            {
               local::range::intersection( std::move( rhs.groups), lhs.groups, local::predicate::equal::alias());
               lhs.forward = set_intersection( lhs.forward, std::move( rhs.forward));

               return lhs;
            }
            
         } // queue

         namespace gateway
         {
            namespace inbound
            {
               Connection set_union( Connection lhs, Connection rhs)
               {
                  lhs.note = algorithm::coalesce( std::move( rhs.note), std::move( lhs.note));
                  lhs.discovery = rhs.discovery;

                  return lhs;
               }

               Group set_union( Group lhs, Group rhs)
               {
                  lhs.note = algorithm::coalesce( std::move( rhs.note), std::move( lhs.note));
                  lhs.connect = rhs.connect;
                  lhs.limit = rhs.limit;
                  local::range::update( std::move( rhs.connections), lhs.connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return lhs;
               }

               Group set_difference( Group lhs, Group rhs)
               {
                  local::range::remove( std::move( rhs.connections), lhs.connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return lhs;
               }

               Group set_intersection( Group lhs, Group rhs)
               {
                  local::range::intersection( std::move( rhs.connections), lhs.connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return lhs;
               }
               
            } // inbound

            Inbound set_union( Inbound lhs, Inbound rhs)
            {
               local::range::update( std::move( rhs.groups), lhs.groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));
               
               return lhs;
            }

            Inbound set_difference( Inbound lhs, Inbound rhs)
            {
               local::range::remove( std::move( rhs.groups), lhs.groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));
               
               return lhs;
            }

            Inbound set_intersection( Inbound lhs, Inbound rhs)
            {
               local::range::intersection( std::move( rhs.groups), lhs.groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));
               
               return lhs;
            }


            namespace outbound
            {
               Connection set_union( Connection lhs, Connection rhs)
               {
                  lhs.note = algorithm::coalesce( std::move( rhs.note), std::move( lhs.note));

                  algorithm::append_unique( rhs.services, lhs.services);
                  algorithm::append_unique( rhs.queues, lhs.queues);

                  return lhs;
               }

               Connection set_difference( Connection lhs, Connection rhs)
               {
                  local::range::remove( std::move( rhs.services), lhs.services, []( auto& l, auto& r){ return l == r;});
                  local::range::remove( std::move( rhs.queues), lhs.queues, []( auto& l, auto& r){ return l == r;});

                  return lhs;
               }

               Connection set_intersection( Connection lhs, Connection rhs)
               {
                  local::range::intersection( std::move( rhs.services), lhs.services, []( auto& l, auto& r){ return l == r;});
                  local::range::intersection( std::move( rhs.queues), lhs.queues, []( auto& l, auto& r){ return l == r;});

                  return lhs;
               }

               Group set_union( Group lhs, Group rhs)
               {
                  lhs.note = algorithm::coalesce( std::move( rhs.note), std::move( lhs.note));
                  lhs.connect = rhs.connect;
                  local::range::update( std::move( rhs.connections), lhs.connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return lhs;
               }

               Group set_difference( Group lhs, Group rhs)
               {
                  local::range::remove( std::move( rhs.connections), lhs.connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return lhs;
               }

               Group set_intersection( Group lhs, Group rhs)
               {
                  local::range::intersection( std::move( rhs.connections), lhs.connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return lhs;
               }
            } // outbound

            Outbound set_union( Outbound lhs, Outbound rhs)
            {
               local::range::update( std::move( rhs.groups), lhs.groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));

               return lhs;
            }

            Outbound set_difference( Outbound lhs, Outbound rhs)
            {
               local::range::remove( std::move( rhs.groups), lhs.groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));

               return lhs;
            }

            Outbound set_intersection( Outbound lhs, Outbound rhs)
            {
               local::range::intersection( std::move( rhs.groups), lhs.groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));

               return lhs;
            }

            Model set_union( Model lhs, Model rhs)
            {
               lhs.inbound = set_union( lhs.inbound, std::move( rhs.inbound));
               lhs.outbound = set_union( lhs.outbound, std::move( rhs.outbound));

               return lhs;
            }

            Model set_difference( Model lhs, Model rhs)
            {
               lhs.inbound = set_difference( lhs.inbound, std::move( rhs.inbound));
               lhs.outbound = set_difference( lhs.outbound, std::move( rhs.outbound));

               return lhs;
            }

            Model set_intersection( Model lhs, Model rhs)
            {
               lhs.inbound = set_intersection( lhs.inbound, std::move( rhs.inbound));
               lhs.outbound = set_intersection( lhs.outbound, std::move( rhs.outbound));

               return lhs;
            }
            
         } // gateway

      } // model

      Model normalize( Model model)
      {
         Trace trace{ "configuration::normalize"};
         alias::normalize::State state;
                  
         {
            auto normalizer = alias::normalize::mutator( state, []( auto& value){ return value.path.filename();});
            algorithm::for_each( model.domain.executables, normalizer);
            algorithm::for_each( model.domain.servers, normalizer);
         }

         {
            state.count = {};
            auto forward_alias = alias::normalize::mutator( state, []( auto&){ return "forward";});

            algorithm::for_each( model.queue.forward.groups, [&forward_alias, &state]( auto& group)
            {
               forward_alias( group);
               
               // normalize the forwards
               auto normalizer = alias::normalize::mutator( state, []( auto& value){ return value.source;});
               algorithm::for_each( group.services, normalizer);
               algorithm::for_each( group.queues, normalizer);
            });;
         }

         {
            state.count = {};
            
            {
               auto normalizer = alias::normalize::mutator( state, []( auto& value)
               { 
                  if( value.connect == decltype( value.connect)::reversed)
                     return "reverse.inbound";
                  return "inbound";
               
               });
               algorithm::for_each( model.gateway.inbound.groups, normalizer);
            }

            {
               auto normalizer = alias::normalize::mutator( state, []( auto& value)
               { 
                  if( value.connect == decltype( value.connect)::reversed)
                     return "reverse.outbound";
                  return "outbound";
               
               });
               algorithm::for_each( model.gateway.outbound.groups, normalizer);
            }
         }

         // take care of alias placeholder mappings...
         {
            auto resolve_placeholders = [ &state]( auto& value)
            {
               if( alias::is::placeholder( value.alias))
                  value.alias = state.placeholders.at( value.alias);
            };

            algorithm::for_each( model.service.restriction.servers, resolve_placeholders);
            algorithm::for_each( model.transaction.mappings, resolve_placeholders);
         }

         return model;
      }

      Model set_union( Model lhs, Model rhs)
      {
         Trace trace{ "configuration::set_union"};

         lhs.system = set_union( lhs.system, std::move( rhs.system));
         lhs.domain = set_union( lhs.domain, std::move( rhs.domain));
         lhs.transaction = set_union( lhs.transaction, std::move( rhs.transaction));
         lhs.service = set_union( lhs.service, std::move( rhs.service));
         lhs.queue = set_union( lhs.queue, std::move( rhs.queue));
         lhs.gateway = set_union( lhs.gateway, std::move( rhs.gateway));

         return lhs;
      }

      Model set_difference( Model lhs, Model rhs)
      {
         Trace trace{ "configuration::set_difference"};

         lhs.system = set_difference( lhs.system, std::move( rhs.system));
         lhs.domain = set_difference( lhs.domain, std::move( rhs.domain));
         lhs.transaction = set_difference( lhs.transaction, std::move( rhs.transaction));
         lhs.service = set_difference( lhs.service, std::move( rhs.service));
         lhs.queue = set_difference( lhs.queue, std::move( rhs.queue));
         lhs.gateway = set_difference( lhs.gateway, std::move( rhs.gateway));

         return lhs;
      }

      Model set_intersection( Model lhs, Model rhs)
      {
         Trace trace{ "configuration::set_intersection"};

         lhs.system = set_intersection( lhs.system, std::move( rhs.system));
         lhs.domain = set_intersection( lhs.domain, std::move( rhs.domain));
         lhs.transaction = set_intersection( lhs.transaction, std::move( rhs.transaction));
         lhs.service = set_intersection( lhs.service, std::move( rhs.service));
         lhs.queue = set_intersection( lhs.queue, std::move( rhs.queue));
         lhs.gateway = set_intersection( lhs.gateway, std::move( rhs.gateway));

         return lhs;
      }
      
   } // configuration
} // casual