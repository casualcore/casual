//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/instance.h"

namespace casual
{
   namespace service::forward::instance
   {
      inline const common::communication::instance::Identity identity{ 0xf17d010925644f728d432fa4a6cf5257_uuid, "CASUAL_SERVICE_FORWARD_PROCESS"};

   } // service::forward::instance
} // casual