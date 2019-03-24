//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/process.h"

#include <string>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         struct Process
         {
            Process( const std::string& executable, const std::vector< std::string>& arguments);
            Process( const std::string& executable);

            ~Process();

            //! @attention can only be used with executables that register them self
            //!   to the (mockup) domain.
            //!
            //! @return
            common::process::Handle handle() const;

            //! explicitly set the process
            //! @throws if the pids differ
            void handle( const process::Handle& process);

         private:
            mutable common::process::Handle m_process;
         };


      } // mockup
   } // common
} // casual


