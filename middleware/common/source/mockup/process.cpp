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

            m_process.pid = common::process::spawn( executable, arguments, {});
         }


         Process::~Process()
         {
            Trace trace{ "common::mockup::Process::~Process()", log::internal::debug};

            try
            {
               common::process::lifetime::terminate( { m_process.pid});

               //
               // We clear all pending signals
               //
               common::signal::clear();
            }
            catch( ...)
            {
               error::handler();
            }
         }

         common::process::Handle Process::handle() const
         {
            if( ! m_process)
            {
               m_process = process::lookup( m_process.pid);
            }
            return m_process;
         }



      } // mockup
   } // common
} // casual
