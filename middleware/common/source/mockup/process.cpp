//!
//! casual 
//!

#include "common/mockup/process.h"

#include "common/process.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         Process::Process( const std::string& executable, const std::vector< std::string>& arguments)
         {
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
