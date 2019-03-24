//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "common/mockup/process.h"
#include "common/mockup/file.h"


namespace casual
{
   namespace test
   {
      namespace domain
      {
         struct Manager
         {
            Manager( common::file::scoped::Path file);
            Manager( std::vector< common::file::scoped::Path> files);
            ~Manager();

            inline const common::mockup::Process& process() const { return m_process;}

            //! tries to activate this domain, that is, resets all global domain specifc paths
            //! and outbound-instances
            void activate();

         private:

            struct Preconstruct
            {
               Preconstruct();

               //! domain root directory
               common::mockup::directory::temporary::Scoped home;
            };
            
            // sets environment variables and stuff
            Preconstruct m_preconstruct;

            std::vector< common::file::scoped::Path> m_files;
         public:

            common::mockup::Process m_process;
         };
      } // domain
   } // test
} // casual