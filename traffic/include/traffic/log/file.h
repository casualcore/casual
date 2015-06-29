/*
 * monitordb.h
 *
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */

#ifndef TRAFFIC_LOG_FILE_H_
#define TRAFFIC_LOG_FILE_H_

#include "common/message/monitor.h"
#include <string>
#include <vector>


namespace casual
{
namespace traffic
{
namespace log
{
   struct Transaction; // Forward

   class File
   {
   public:

      typedef Transaction transaction_type;

      File( std::ofstream& stream);

      ~File();

      void store( const common::message::traffic::monitor::Notify& message);

   private:
      std::ofstream& m_stream;
   };

   struct Transaction
   {
      Transaction( File& file);
      ~Transaction();
   private:
      File& m_file;
   };

}
}
}

void operator>>( const casual::common::message::traffic::monitor::Notify& message, casual::traffic::log::File& file);


#endif /* TRAFFIC_LOG_FILE_H_ */
