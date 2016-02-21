//!
//! casual 
//!

#include "common/mockup/process.h"

#include "common/process.h"
#include "common/trace.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         Process::Process( const std::string& executable, const std::vector< std::string>& arguments)
         {
            Trace trace{ "common::mockup::Process::Process()", log::internal::debug};
            auto pid = common::process::spawn( executable, arguments, {});

            do
            {
               process::sleep( std::chrono::milliseconds{ 1});
               m_process = process::lookup( pid);
            }
            while( m_process.queue == 0);

            //
            // We need to wait until it's up'n running
            //
            common::process::ping( m_process.queue);
         }


         Process::~Process()
         {
            Trace trace{ "common::mockup::Process::~Process()", log::internal::debug};

            common::process::lifetime::terminate( { m_process.pid});

            //
            // We clear all pending signals
            //
            common::signal::clear();
         }

         common::process::Handle Process::handle() const { return m_process;}



      } // mockup
   } // common
} // casual
