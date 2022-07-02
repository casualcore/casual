//! 
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

namespace casual
{
   // overloaded idiom helper
   template< typename... Ts> struct overloaded : Ts... { using Ts::operator()...;};
   template< typename... Ts> overloaded( Ts...) -> overloaded<Ts...>;

} // casual