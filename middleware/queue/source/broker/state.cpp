//!
//! state.cpp
//!
//! Created on: Aug 16, 2015
//!     Author: Lazan
//!

#include "queue/broker/state.h"

#include "common/algorithm.h"

namespace casual
{
   namespace queue
   {
      namespace broker
      {

         State::State( common::communication::ipc::inbound::Device& receive) : receive( receive) {}
         State::State() : State( common::communication::ipc::inbound::device()) {}


         State::Group::Group() = default;
         State::Group::Group( std::string name, id_type process) : name( std::move( name)), process( std::move( process)) {}


         bool operator == ( const State::Group& lhs, State::Group::id_type process) { return lhs.process == process;}


         std::vector< common::platform::pid_type> State::processes() const
         {
            std::vector< common::platform::pid_type> result;

            for( auto& group : groups)
            {
               result.push_back( group.process.pid);
            }

            return result;
         }


         State::Correlation::Correlation( id_type caller, const common::Uuid& reply_correlation, std::vector< Group::id_type> groups)
            : caller( std::move( caller)), reply_correlation{ reply_correlation}
         {
            common::range::move( groups, requests);
         }

         State::Correlation::Request::Request() = default;
         State::Correlation::Request::Request( Group::id_type group) : group( std::move( group)) {}



         bool State::Correlation::replied() const
         {
            return common::range::all_of( requests, []( const Request& r){ return r.stage >= Stage::error;});
         }

         State::Correlation::Stage State::Correlation::stage() const
         {
            auto max = common::range::min( requests, []( const Request& lhs, const Request& rhs)
                  {
                     return lhs.stage < rhs.stage;
                  });

            if( max)
            {
               return max->stage;
            }
            return Stage::empty;
         }


         void State::Correlation::stage( const id_type& id, Stage stage)
         {
            auto found = common::range::find_if( requests, [&]( const Request& r){ return r.group.pid == id.pid;});

            if( found)
            {
               found->stage = stage;
            }
         }

      } // broker
   } // queue
} // casual
