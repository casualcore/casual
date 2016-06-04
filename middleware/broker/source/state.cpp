//!
//! state.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#include "broker/state.h"
#include "broker/transform.h"

#include "common/server/service.h"
#include "common/server/lifetime.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/exception.h"
#include "common/algorithm.h"

#include "common/process.h"



#include <functional>

namespace casual
{

   namespace broker
   {
      using namespace common;


      namespace local
      {
         namespace
         {
            template< typename M, typename ID>
            auto get( M& map, ID&& id) -> decltype( map.at( id))
            {
               auto found = common::range::find( map, id);

               if( ! found)
               {
                  throw state::exception::Missing{ "missing", CASUAL_NIP( id)};
               }
               return found->second;
            }

         } // <unnamed>
      } // local


      namespace state
      {


         bool operator == ( const Service::Instance& lhs, common::platform::pid::type rhs)
         {
            return lhs.instance.get().process.pid == rhs;
         }

         bool Service::Instance::idle() const
         {
            return instance.get().state() == state::Instance::State::idle;
         }

         void Service::remove( common::platform::pid::type instance)
         {
            range::trim( instances, range::remove( instances, instance));
         }

         void Service::add( state::Instance& instance)
         {
            instances.emplace_back( instance);
         }


         Service::Instance* Service::idle()
         {
            auto found = range::find_if( instances, std::mem_fn( &Service::Instance::idle));

            if( found)
            {
               return &(*found);
            }
            return nullptr;
         }

         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ name: " << service.information.name
                  << ", type: " << service.information.type
                  << ", transaction: " << service.information.transaction
                  << ", timeout: " << service.information.timeout.count()
                  << "}";
          }

      } // state


      void State::traffic_t::monitors_t::add( common::process::Handle process)
      {
         processes.push_back( process);
      }

      void State::traffic_t::monitors_t::remove( common::platform::pid::type pid)
      {
         range::trim( processes, range::remove_if( processes, [pid]( const process::Handle& p){ return p.pid == pid;}));
      }

      std::vector< common::platform::ipc::id::type> State::traffic_t::monitors_t::get() const
      {
         return range::transform( processes, []( const process::Handle& p){ return p.queue;});
      }


      state::Service& State::service( const std::string& name)
      {
         return local::get( services, name);
      }

      state::Instance& State::instance( common::platform::pid::type pid)
      {
         return local::get( instances, pid);
      }



      void State::remove_process( common::platform::pid::type pid)
      {
         Trace trace{ "broker::State::remove_process", log::internal::debug};

         auto found = common::range::find( instances, pid);

         if( found)
         {
            log::internal::debug << "remove process pid: " << pid << std::endl;

            for( auto& s : services)
            {
               s.second.remove( pid);
            }

            instances.erase( std::begin( found));
         }
      }

      void State::add( common::process::Handle process, std::vector< common::message::Service> services)
      {
         Trace trace{ "broker::State::add services", log::internal::debug};

         auto& instance = find_or_add( process);

         for( auto& s : services)
         {
            auto& service = find_or_add( std::move( s));
            service.add( instance);
         }
      }

      void State::remove( common::process::Handle process, const std::vector< common::message::Service>& services)
      {
         Trace trace{ "broker::State::remove", log::internal::debug};

         auto& instance = find_or_add( process);

         for( auto& s : services)
         {
            auto service = find_service( s.name);

            if( service)
            {
               service->remove( instance.process.pid);
            }
         }
      }

      state::Service* State::find_service(  const std::string& name)
      {
         auto found = range::find( services, name);

         if( found)
         {
            return &found->second;
         }
         return nullptr;
      }

      state::Instance& State::find_or_add( common::process::Handle process)
      {
         Trace trace{ "broker::State::add process", log::internal::debug};

         auto found = range::find( instances, process.pid);

         if( found)
         {
            log::internal::debug << "process found\n";
            return found->second;
         }

         state::Instance instance;
         instance.process = process;

         return instances.emplace( process.pid, std::move( instance)).first->second;
      }

      state::Service& State::find_or_add( common::message::Service message)
      {
         auto found = range::find( services, message.name);

         if( found)
         {
            return found->second;
         }
         state::Service service{ message};
         return services.emplace( message.name, std::move( service)).first->second;
      }


      void State::connect_broker( std::vector< common::message::Service> services)
      {
         add( common::process::handle(), std::move( services));
      }


      std::size_t State::size() const
      {
         return instances.size();
      }


   } // broker

} // casual
