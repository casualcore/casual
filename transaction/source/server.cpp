//!
//! server.cpp
//!
//! Created on: Jul 25, 2013
//!     Author: Lazan
//!

#include "common/transaction_context.h"

#include <xa.h>

#include <iostream>

#include <map>
#include <string>



extern "C"
{
   //extern struct xa_switch_t db2xa_switch_static_std;
   //extern struct xa_switch_t db2xa_switch_std;
   extern struct xa_switch_t casual_mockup_xa_switch_static;
}

xa_switch_t* tmswitch = &casual_mockup_xa_switch_static;


const char* status( int code)
{
   return casual::common::transaction::xaError( code);
}


int open( int rmid)
{
   return tmswitch->xa_open_entry( "db=test,uid=db2,pwd=db2", rmid, TMNOFLAGS);
}

int close( int rmid)
{
   return tmswitch->xa_close_entry( "", rmid, TMNOFLAGS);
}


int main( int argc, char** argv)
{
   std::cout << "rm name: " << tmswitch->name << std::endl;
   std::cout << "rm flags: " << tmswitch->flags << std::endl;

   auto result = tmswitch->xa_open_entry( "db=test,uid=db2,pwd=db2", 1, TMNOFLAGS);

   std::cout << "open test: " << status( result) << std::endl;

   result = tmswitch->xa_open_entry( "db=test2,uid=db2,pwd=db2", 2, TMNOFLAGS);

   std::cout << "open test2: " << status( result) << std::endl;

   std::cout << "close test: " << status( close( 1)) << std::endl;
   std::cout << "close test2: " << status( close( 2)) << std::endl;

   return 0;
}

