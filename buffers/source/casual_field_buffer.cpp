//
// casual_field_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include "casual_field_buffer.h"

namespace
{
   namespace internal
   {
      namespace explore
      {
         int type( const long id, int& result)
         {
            result = id / 100000;

         #ifdef __bool_true_false_are_defined
            if( result < CASUAL_FIELD_BOOL)
         #else
            if( result < CASUAL_FIELD_CHAR)
         #endif
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            if( result > CASUAL_FIELD_BINARY)
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            //
            // TODO: check if existing in repository
            //

            return CASUAL_FIELD_SUCCESS;
         }

         int name( const long id, const char*& result)
         {
            result = "TODO";
            return CASUAL_FIELD_SUCCESS;
         }

         int id( const char* const name, long& result)
         {
            // TODO
            result = 0;
            return CASUAL_FIELD_SUCCESS;
         }


      }
   }
}


const char* CasualFieldDescription( const int code)
{
   switch( code)
   {
   case CASUAL_FIELD_SUCCESS:
      return "Success";
   default:
      return "TODO";
   }
}

int CasualFieldAddBool( char* const buffer, const long id, const bool value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldAddChar( char* const buffer, const long id, const char value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldAddShort( char* const buffer, const long id, const short value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldAddLong( char* const buffer, const long id, const long value)
{
   return CASUAL_FIELD_SUCCESS;
}
int CasualFieldAddFloat( char* const buffer, const long id, const float value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldAddDouble( char* const buffer, const long id, const double value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldAddString( char* const buffer, const long id, const char* value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldAddBinary( char* const buffer, const long id, const char* value, const long size)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldAddData( char* const buffer, const long id, const void* value, const long size)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetBool( const char* const buffer, const long id, const long index, bool* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetChar( const char* const buffer, const long id, const long index, char* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetShort( const char* const buffer, const long id, const long index, short* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetLong( const char* const buffer, const long id, const long index, long* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetFloat( const char* const buffer, const long id, const long index, float* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetDouble( const char* const buffer, const long id, const long index, double* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetString( const char* const buffer, const long id, const long index, char** value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetBinary( const char* const buffer, const long id, const long index, char** value, long* const size)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetData( const char* const buffer, const long id, const long index, void** value, long* const size)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldExploreBuffer( const char* buffer, long* const size, long* const used)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldExploreId( const char* const name, long* const id)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldExploreName( const long id, const char** name)
{
   return internal::explore::name( id, *name);
}

int CasualFieldExploreType( const long id, int* const type)
{
   return internal::explore::type( id, *type);
}

int CasualFieldRemoveAll( char* const buffer)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldRemoveId( char* const buffer, const long id)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldRemoveOccurrence( char* const buffer, const long id, const long occurrence)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldCopyBuffer( char* const target, const char* const source)
{
   return CASUAL_FIELD_SUCCESS;
}

