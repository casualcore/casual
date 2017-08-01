//!
//! casual 
//!

#include "common/mockup/process.h"
#include "common/mockup/log.h"

#include "common/process.h"
#include "common/communication/ipc.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {

         Process::Process( const std::string& executable, const std::vector< std::string>& arguments)
         {
            Trace trace{ "common::mockup::Process::Process()"};

            m_process.pid = common::process::spawn( executable, arguments, {});
         }

         Process::Process( const std::string& executable) : Process{ executable, {}} {}


         Process::~Process()
         {
            Trace trace{ "common::mockup::Process::~Process()"};

            try
            {
               auto terminate = scope::execute( [&](){
                  common::process::lifetime::terminate( { m_process.pid});
               });

               if( ! m_process.queue)
               {
                  //
                  // Try to get corresponding queue
                  //
                  m_process.queue = process::instance::fetch::handle( m_process.pid, process::instance::fetch::Directive::direct).queue;
                  log << "mockup fetched process: " << m_process << '\n';
               }


               if( m_process.queue)
               {
                  communication::ipc::blocking::send( m_process.queue, message::shutdown::Request{});
                  common::process::wait( m_process.pid);
               }
               else
               {
                  terminate();
               }

               //
               // We clear all pending signals
               //
               common::signal::clear();

               terminate.release();

            }
            catch( ...)
            {
               exception::handle();
            }
         }

         common::process::Handle Process::handle() const
         {
            if( ! m_process)
            {
               m_process = process::instance::fetch::handle( m_process.pid);
            }
            return m_process;
         }



      } // mockup
   } // common
} // casual
