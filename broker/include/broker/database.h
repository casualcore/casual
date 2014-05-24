//!
//! database.h
//!
//! Created on: May 18, 2014
//!     Author: Lazan
//!

#ifndef DATABASE_H_
#define DATABASE_H_


#include "sql/database.h"

namespace casual
{

   namespace broker
   {

      struct Database
      {

         Database();




         sql::database::Connection m_connection;
      };

   } // broker


} // casual

#endif // DATABASE_H_
