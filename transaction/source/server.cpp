//!
//! server.cpp
//!
//! Created on: Jul 25, 2013
//!     Author: Lazan
//!

#include <xa.h>

#include <iostream>

#include <map>
#include <string>

namespace casual
{
   namespace transaction
   {



   }
}

extern "C"
{
   extern struct xa_switch_t db2xa_switch_static_std;
   //extern struct xa_switch_t db2xa_switch_std;
}

xa_switch_t* tmswitch = &db2xa_switch_static_std;




const char* status( int status)
{
   static const std::map< int, const char*> mapping{
      { XA_RBCOMMFAIL, "XA_RBCOMMFAIL"},
      { XA_RBDEADLOCK, "XA_RBDEADLOCK"},
      { XA_RBINTEGRITY, "XA_RBINTEGRITY"},
      { XA_RBOTHER, "XA_RBOTHER"},
      { XA_RBPROTO, "XA_RBPROTO"},
      { XA_RBTIMEOUT, "XA_RBTIMEOUT"},
      { XA_RBTRANSIENT, "XA_RBTRANSIENT"},
      { XA_NOMIGRATE, "XA_NOMIGRATE"},
      { XA_HEURHAZ, "XA_HEURHAZ"},
      { XA_HEURCOM, "XA_HEURCOM"},
      { XA_HEURRB, "XA_HEURRB"},
      { XA_HEURMIX, "XA_HEURMIX"},
      { XA_RETRY, "XA_RETRY"},
      { XA_RDONLY, "XA_RDONLY"},
      { XA_OK, "XA_OK"},
      { XAER_ASYNC, "XAER_ASYNC"},
      { XAER_RMERR, "XAER_RMERR"},
      { XAER_NOTA, "XAER_NOTA"},
      { XAER_INVAL, "XAER_INVAL"},
      { XAER_PROTO, "XAER_PROTO"},
      { XAER_RMFAIL, "XAER_RMFAIL"},
      { XAER_DUPID, "XAER_DUPID"},
      { XAER_OUTSIDE, "XAER_OUTSIDE"}
   };
   return mapping.at( status);
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

