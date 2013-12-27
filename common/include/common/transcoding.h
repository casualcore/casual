//
// transcoding.h
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#ifndef TRANSCODING_H_
#define TRANSCODING_H_

#include <string>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace transcoding
      {
         namespace Base64
         {
            std::string encode( const std::vector<char>& data);
            std::vector<char> decode( const std::string& data);
         }
      }
   }
}


#endif /* TRANSCODING_H_ */
