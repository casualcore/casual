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
                           *found += std::move( value);
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
   
               } // range

            } // <unnamed>
         } // local

         namespace system
         {
            Model& Model::operator += ( Model rhs)
            {
               local::range::replace( std::move( rhs.resources), resources, local::predicate::equal::key());

               return *this;
            }
         } // system


         namespace domain
         {
            Environment& Environment::operator += ( Environment rhs)
            {
               local::range::replace( std::move( rhs.variables), variables, []( auto& l, auto& r){ return l.name() == r.name();});
               return *this;
            }

            Model& Model::operator += ( Model rhs)
            {
               name = algorithm::coalesce( std::move( rhs.name), std::move( name));

               environment += std::move( rhs.environment);

               local::range::replace( std::move( rhs.groups), groups, local::predicate::equal::name());
               local::range::replace( std::move( rhs.servers), servers, local::predicate::equal::alias());
               local::range::replace( std::move( rhs.executables), executables, local::predicate::equal::alias());

               return *this;
            }
            
         } // domain
         

         namespace transaction
         {
            Model& Model::operator += ( Model rhs)
            {
               log = algorithm::coalesce( std::move( rhs.log), std::move( log));
               local::range::replace( std::move( rhs.resources), resources, local::predicate::equal::name());
               local::range::replace( std::move( rhs.mappings), mappings, local::predicate::equal::alias());

               return *this;
            }
            
         } // transaction

         namespace service
         {
            Restriction& Restriction::operator += ( Restriction rhs)
            {
               local::range::replace( std::move( rhs.servers), servers, local::predicate::equal::alias());
               return *this;
            }

            Timeout& Timeout::operator += ( Timeout rhs)
            {
               if( rhs.duration)
                  duration = std::move( rhs.duration);

               contract = rhs.contract;
               return *this;
            }

            Global& Global::operator += ( Global rhs)
            {
               timeout += std::move( rhs.timeout);

               if( rhs.discoverable)
                  discoverable = std::move( rhs.discoverable);

               return *this;
            }

            Model& Model::operator += ( Model rhs)
            {
               global += std::move( rhs.global);

               local::range::replace( std::move( rhs.services), services, local::predicate::equal::name());

               restriction += std::move( rhs.restriction);
               
               return *this;
            }
            
         } // service

         namespace queue
         {
            Group& Group::operator += ( Group rhs)
            {
               note = algorithm::coalesce( std::move( rhs.note), std::move( note));
               local::range::replace( std::move( rhs.queues), queues, local::predicate::equal::name());
               
               return *this;
            }

            namespace forward
            {

               Group& Group::operator += ( Group rhs)
               {
                  note = algorithm::coalesce( std::move( rhs.note), std::move( note));
                  local::range::replace( std::move( rhs.services), services, local::predicate::equal::alias());
                  local::range::replace( std::move( rhs.queues), queues, local::predicate::equal::alias());

                  return *this;
               }   
            } // forward

            Forward& Forward::operator += ( Forward rhs)
            {
               local::range::update( std::move( rhs.groups), groups, local::predicate::equal::alias());
               
               return *this;
            }

            Model& Model::operator += ( Model rhs)
            {
               note = algorithm::coalesce( std::move( rhs.note), std::move( note));

               local::range::update( std::move( rhs.groups), groups, local::predicate::equal::alias());
               forward += std::move( rhs.forward);

               return *this;
            }
            
         } // queue

         namespace gateway
         {
            namespace inbound
            {

               Connection& Connection::operator += ( Connection rhs)
               {
                  note = algorithm::coalesce( std::move( rhs.note), std::move( note));
                  discovery = rhs.discovery;

                  return *this;
               }

               Group& Group::operator += ( Group rhs)
               {
                  note = algorithm::coalesce( std::move( rhs.note), std::move( note));
                  connect = rhs.connect;
                  limit = rhs.limit;
                  local::range::update( std::move( rhs.connections), connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return *this;
               }
               
            } // inbound

            Inbound& Inbound::operator += ( Inbound rhs)
            {
               local::range::update( std::move( rhs.groups), groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));
               return *this;
            }


            namespace outbound
            {
               Connection& Connection::operator += ( Connection rhs)
               {
                  note = algorithm::coalesce( std::move( rhs.note), std::move( note));

                  algorithm::append_unique( rhs.services, services);
                  algorithm::append_unique( rhs.queues, queues);

                  return *this;
               }

               Group& Group::operator += ( Group rhs)
               {
                  note = algorithm::coalesce( std::move( rhs.note), std::move( note));
                  connect = rhs.connect;
                  local::range::update( std::move( rhs.connections), connections, []( auto& l, auto& r){ return l.address == r.address;});

                  return *this;
               }
            } // outbound

            Outbound& Outbound::operator += ( Outbound rhs)
            {
               local::range::update( std::move( rhs.groups), groups, predicate::conjunction( local::predicate::equal::alias(), local::predicate::equal::connect()));

               return *this;
            }

            Model& Model::operator += ( Model rhs)
            {
               inbound += std::move( rhs.inbound);
               outbound += std::move( rhs.outbound);
               return *this;
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
      

      Model& Model::operator += ( Model rhs)
      {
         system += std::move( rhs.system);
         domain += std::move( rhs.domain);
         transaction += std::move( rhs.transaction);
         service += std::move( rhs.service);
         queue += std::move( rhs.queue);
         gateway += std::move( rhs.gateway);

         return *this;
      }
      
   } // configuration
} // casual