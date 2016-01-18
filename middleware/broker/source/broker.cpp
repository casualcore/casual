//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "broker/broker.h"
#include "broker/handle.h"
#include "broker/transform.h"

#include "broker/admin/server.h"

#include "config/domain.h"

#include "common/environment.h"

#include "common/internal/trace.h"
#include "common/internal/log.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/process.h"


#include "sf/log.h"


#include <xatmi.h>

#include <fstream>
#include <algorithm>
#include <iostream>




namespace casual
{
   using namespace common;

	namespace broker
	{
		namespace local
		{
			namespace
			{
				template< typename Q>
				void exportBrokerQueueKey( const Q& queue, const std::string& path)
				{

					if( common::file::exists( path))
					{
					   std::ifstream file( path);

					   if( file)
					   {
					      try
					      {

					         common::signal::timer::Scoped alarm{ std::chrono::seconds( 5)};

                        decltype( queue.id()) id;
                        file >> id;

                        common::message::server::ping::Request request;
                        request.process = common::process::handle();

                        auto reply = communication::ipc::call( id, request, communication::ipc::policy::Blocking{});

                        //
                        // There are another running broker for this domain - abort
                        //
                        common::log::error << "Other running broker - action: terminate" << std::endl;
                        throw common::exception::signal::Terminate( "other running broker", __FILE__, __LINE__);

					      }
					      catch( const common::exception::signal::Timeout&)
					      {
					         common::log::error << "failed to get ping response from potentially running broker - to risky to start - action: terminate" << std::endl;
					         throw common::exception::signal::Terminate( "other running broker", __FILE__, __LINE__);
					      }
					   }

					   //
                  // TODO: ping to see if there are another broker running
                  //
					   common::file::remove( path);
					}

					log::internal::debug << "writing broker queue file: " << path << std::endl;

					std::ofstream brokerQueueFile( path);

					if( brokerQueueFile)
					{
                  brokerQueueFile << queue.id() << std::endl;
                  brokerQueueFile.close();
					}
					else
					{
					   throw exception::NotReallySureWhatToNameThisException( "failed to write broker queue file: " + path);
					}

				}

			}
		}




		Broker::Broker( const Settings& arguments)
		  : m_brokerQueueFile( common::process::singleton( common::environment::file::brokerQueue()))
		{
         //
         // Configure
         //
         {
            common::trace::internal::Scope trace{ "broker configuration"};


            config::domain::Domain domain;

            try
            {
               domain = config::domain::get( arguments.configurationfile);
            }
            catch( const exception::invalid::File& exception)
            {
               common::log::information << "failed to open '" << arguments.configurationfile << "' - start anyway..." << std::endl;
            }

            //
            // Set domain name
            //
            environment::domain::name( domain.name);

            common::log::internal::debug << CASUAL_MAKE_NVP( domain);

            m_state = transform::configuration::Domain{}( domain);
         }
		}



		Broker::~Broker()
		{
		   try
		   {
            //
            // Terminate
            //
            terminate();
		   }
         catch( ...)
         {
            common::error::handler();
         }
		}

		void Broker::terminate()
		{
		   common::trace::internal::Scope trace{ "Broker::terminate()"};

         try
         {
            const auto domain = common::environment::domain::name();

            common::log::information << "shutting down domain '" << domain << "'\n";

            //
            // We have to remove this process first, so we don't try to terminate our self
            //
            m_state.remove_process( common::process::id());

            //
            // Terminate children
            //
            common::process::children::terminate( &handle::process_exit, m_state.processes());


            common::log::information << "domain '" << domain << "' is off-line\n";
         }
         catch( const common::exception::signal::Timeout& exception)
         {
            auto pids  = m_state.processes();
            common::log::error << "failed to get response for terminated processes in a timely manner - pids: " << range::make( pids) << " - action: abort" << std::endl;
         }
         catch( ...)
         {
            common::error::handler();
         }

		}

      void Broker::start()
      {
         try
         {

            auto start = common::platform::clock_type::now();


            {
               common::trace::internal::Scope trace( "boot domain");

               //
               // boot the domain...
               //
               handle::boot( m_state);
            }


            auto end = common::platform::clock_type::now();

            common::log::information << "domain: \'" << common::environment::domain::name() << "\' is on-line - " << m_state.processes().size() << " processes - boot time: "
                  << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;


            message::pump( m_state);


         }
         catch( const common::exception::signal::Terminate&)
         {
            // we do nothing, and let the dtor take care of business
            common::log::internal::debug << "broker has been terminated\n";
         }
         catch( ...)
         {
            common::error::handler();
         }
		}



      namespace message
      {
         void pump( State& state)
         {
            try
            {
               //
               // Prepare message-pump handlers
               //

               common::log::internal::debug << "prepare message-pump handlers\n";


               auto handler = broker::handler( state);

               common::log::internal::debug << "start message pump\n";

               //static const communication::error::handler::callback::on::Terminate callback{ handle::dead::Process{ state.ipc()}};

               while( true)
               {
                  if( state.pending.replies.empty())
                  {
                     handler( ipc::device().blocking_next());
                  }
                  else
                  {

                     //
                     // Send pending replies
                     //
                     {

                        common::log::internal::debug << "pending replies: " << range::make( state.pending.replies) << '\n';

                        decltype( state.pending.replies) replies;
                        std::swap( replies, state.pending.replies);

                        auto remain = std::get< 1>( common::range::partition(
                              replies,
                              common::message::pending::sender(
                                    communication::ipc::policy::ignore::signal::non::Blocking{},
                                    ipc::device().error_handler())));

                        range::move( remain, state.pending.replies);
                     }

                     //
                     // Take care of broker dispatch
                     //
                     {
                        //
                        // If we've got pending that is 'never' sent, we still want to
                        // do a lot of broker stuff. Hence, if we got into an 'error state'
                        // we'll still function...
                        //
                        // TODO: Should we have some sort of TTL for the pending?
                        //
                        auto count = common::platform::batch::transaction;

                        while( handler( ipc::device().non_blocking_next()) && count-- > 0)
                           ;
                     }

                  }
               }


            }
            catch( const common::exception::signal::Terminate&)
            {
               // we do nothing, and let the dtor take care of business
               common::log::internal::debug << "broker has been terminated\n";
            }
            catch( ...)
            {
               common::error::handler();
            }
         }

      } // message


      namespace update
      {
         void instances( State& state, const std::vector< admin::update::InstancesVO>& instances)
         {
            common::trace::internal::Scope trace( "broker::update::instances");

            auto updateInstances = [&]( const admin::update::InstancesVO& value)
                  {
                     for( auto&& server : state.servers)
                     {
                        if( server.second.alias == value.alias)
                        {
                           server.second.configured_instances = value.instances;
                           handle::update::instances( state, server.second);
                        }
                     }
                  };
            common::range::for_each( instances, updateInstances);
         }

      } // update

      admin::ShutdownVO shutdown( State& state, bool broker)
      {
         common::trace::internal::Scope trace( "broker::shutdown");

         admin::ShutdownVO result;


         if( state.mode == State::Mode::shutdown)
         {
            log::error << "broker already in shutdown mode" << std::endl;
            return result;
         }



         auto orginal = state.processes();



         handle::shutdown( state);

         result.online = state.processes();
         result.offline = range::to_vector( range::difference( orginal, result.online));

         if( broker)
         {
            handle::send_shutdown();
         }

         return result;
      }
	} // broker
} // casual





