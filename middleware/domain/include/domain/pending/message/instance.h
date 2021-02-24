//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/instance.h"

namespace casual
{
   namespace domain::pending::message::instance
   {
      inline const common::communication::instance::Identity identity{ 0xe32ece3e19544ae69aae5a6326a3d1e9_uuid, "CASUAL_DOMAIN_PENDING_MESSAGE_PROCESS"};

   } // domain::pending::message::instance
} // casual