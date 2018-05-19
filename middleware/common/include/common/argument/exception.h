//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/exception/system.h"

namespace casual
{
   namespace common
   {
      namespace argument
      {
         namespace exception
         {
            namespace invalid
            {
               using Argument = common::exception::system::invalid::Argument;

            } // invalid

            namespace user
            {
               struct Abort : std::runtime_error
               {
                  using std::runtime_error::runtime_error;
               };

               //! 
               //! Will be thrown if built in help is invoked.
               //!
               struct Help : Abort
               {
                  using Abort::Abort;
               };

               namespace bash
               {
                  //! 
                  //! Will be thrown if the 'secret' casual-bash-completion is passed
                  //!
                  struct Completion : Abort
                  {
                     using Abort::Abort;
                  };
               } // bash
            } // user
         } // exception
      } // argument
   } // common

} // casual
