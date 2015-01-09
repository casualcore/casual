//
// casual_field_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include "buffer/field.h"

#include "common/environment.h"
#include "common/network_byteorder.h"
#include "common/buffer/pool.h"
#include "common/log.h"

#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"

#include <cstring>

#include <string>
#include <map>
#include <vector>

#include <algorithm>

#include <type_traits>


namespace casual
{
   namespace buffer
   {
      namespace field
      {

         /*
          * This implementation contain a C-interface that offers functionality
          * for a replacement to FML32 (non-standard compared to XATMI)
          *
          * The implementation is quite cumbersome since the user-handle is the
          * actual underlying buffer which is a std::vector<char>::data() so we
          * cannot just use append data which might imply reallocation since
          * this is a "stateless" interface and thus search from start is done
          * all the time
          *
          * The main idea is to keep the buffer "ready to go" without the need
          * for extra marshalling when transported and thus data is stored in
          * network byteorder etc from start
          *
          * One doubtful decision is to (redundantly) store the allocated size
          * in the buffer as well that indeed simplifies a few things
          *
          * The buffer layout is |size|used|values ...|
          *
          * The values layout is |id|size|data...|
          *
          * Some things that might be explained that perhaps is not so obvious
          * with FML32; The type (FLD_SHORT/CASUAL_FIELD_SHORT etc) can be
          * deduced from the id, i.e. every id for 'short' must be between 0x0
          * and 0x1FFFFFF and every id for 'long' must be between 0x2000000 and
          * 0x3FFFFFF etc. In this implementation no validation of whether the
          * id exists in the repository-table occurs but just from the type.
          * For now there's no proper error-handling while handling the table-
          * repository and that has to improve ...
          *
          * ... and many other things can be improved as well. There's a lot of
          * semi-redundant functions for finding a certain occurrence which is
          * made by searching from start every time since we do not have any
          * index or such but some (perhaps naive) benchmarks shows that that
          * doesn't give isn't so slow after all ... future will show
          *
          * Removal of occurrences doesn't actually remove them but just make
          * them invisible by nullify it and thus the buffer doesn't collapse
          *
          * The repository-implementation is a bit comme ci comme ça and to
          * provide something like 'mkfldhdr32' the functionality has to be
          * accessible from there (or vice versa)
          */



         namespace
         {
            typedef common::platform::const_raw_buffer_type const_data_type;
            typedef common::platform::raw_buffer_type data_type;
            typedef common::platform::raw_buffer_size size_type;

            // A thing to make stuff less bloaty ... not so good though
            constexpr auto size_size = common::network::bytes<size_type>();


            //! Transform a value in place from network to host
            template<typename T>
            void parse( const_data_type where, T& value)
            {
               // Cast to network version
               const auto encoded = *reinterpret_cast< const common::network::type<T>*>( where);
               // Decode to host version
               value = common::network::byteorder<T>::decode( encoded);
            }

            // TODO: We should remove this
            void parse( data_type where, data_type& value)
            {
               value = where;
            }

            //! Transform a value in place from host to network
            template<typename T>
            void write( data_type where, const T value)
            {
               // Encode to network version
               const auto encoded = common::network::byteorder<T>::encode( value);
               // Write it to buffer
               std::memcpy( where, &encoded, sizeof( encoded));
            }

            // TODO: Perhaps useless
            void write( data_type where, const_data_type value, const size_type count)
            {
               std::memcpy( where, value, count);
            }


            //!
            //! Helper-class to manage the buffer
            //!
            class Buffer
            {
            public:

               template<typename type, size_type offset>
               class Data
               {
               public:

                  Data( data_type where) : m_where( where ? where + offset : nullptr) {}

                  //! @return Whether this is valid or not
                  explicit operator bool () const
                  {return m_where != nullptr;}

                  type operator() () const
                  {
                     type result;
                     parse( m_where, result);
                     return result;
                  }

                  void operator() ( const type value)
                  {
                     write( m_where, value);
                  }

               private:
                  data_type m_where;
               };

               class Value
               {
               public:

                  Value( data_type buffer) :
                     id( buffer),
                     size( buffer),
                     data( buffer)
                  {}


                  // TODO: Make this more generic
                  static constexpr size_type header()
                  {return size_size + size_size;}

                  Data<size_type, size_size * 0> id;
                  Data<size_type, size_size * 1> size;
                  Data<data_type, size_size * 2> data;

                  bool operator == ( const Value& other) const
                  {return this->data() == other.data();}

                  bool operator != ( const Value& other) const
                  {return this->data() != other.data();}

                  //! @return A 'handle' to next 'iterator'
                  Value next() const
                  {
                     return Value( data ? data() + size() : nullptr);
                  }

               };

               Buffer( data_type buffer) :
                  size( buffer),
                  used( buffer),
                  data( buffer)
               {}

               // TODO: Can we make this more generic ?
               Data<size_type, size_size * 0> size;
               Data<size_type, size_size * 1> used;
               Data<data_type, size_size * 2> data;

               // TODO: Make this more generic
               static constexpr size_type header()
               {return size_size + size_size;}

               //! @return Whether this buffer is valid or not
               explicit operator bool () const
               {return size ? true : false;}

               //! @return A 'handle' to "begin"
               Value first() const
               {
                  return Value( data ? data() : nullptr);
               }

               //! @return A 'handle' to "end"
               Value beyond() const
               {
                  return Value( data ? data() + used() - header() : nullptr);
               }
            };

            // TODO: Doesn't have to inherit from anything, but has to implement a set of functions (like pool::basic)
            class Allocator : public common::buffer::pool::default_pool
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{{ CASUAL_FIELD, "" }};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const common::buffer::Type& type, const std::size_t size)
               {
                  constexpr auto header = Buffer::header();

                  const auto actual = size < header ? header : size;

                  m_pool.emplace_back( type, actual);

                  auto data = m_pool.back().payload.memory.data();

                  Buffer buffer{ data};
                  buffer.size( actual);
                  buffer.used( header);

                  return data;
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const std::size_t size)
               {
                  const auto result = find( handle);

                  if( result == std::end( m_pool))
                  {
                     // TODO: shouldn't this be an error (exception) ?
                     return nullptr;
                  }

                  const auto used = Buffer( result->payload.memory.data()).used();

                  const auto actual = size < used ? used : size;

                  //
                  // User may shrink a buffer, but not smaller than what's used
                  //
                  result->payload.memory.resize( actual);

                  Buffer( result->payload.memory.data()).size( actual);

                  return result->payload.memory.data();
               }
            };

         } //

      } // field

   } // buffer

   //
   // Register and define the type that can be used to get the custom pool
   //
   template class common::buffer::pool::Registration< buffer::field::Allocator>;


   namespace buffer
   {
      namespace field
      {

         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {

            Buffer find_buffer( const char* const handle)
            {
               try
               {
                  auto& buffer = pool_type::pool.get( handle);
                  return Buffer( buffer.payload.memory.data());
               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated field-logging ?
                  //
                  common::error::handler();
                  return Buffer( nullptr);
               }
            }

            int validate_id( const long id, const int type)
            {
               //
               // TODO: Shall we validate with repository/table as well ?
               //

               if( id > CASUAL_FIELD_NO_ID)
               {
                  if( type == id / CASUAL_FIELD_TYPE_BASE)
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }
               }

               return CASUAL_FIELD_INVALID_ID;
            }

            int remove( const char* const handle, const long id)
            {
               if( !(id > CASUAL_FIELD_NO_ID))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               int result = CASUAL_FIELD_NO_OCCURRENCE;

               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     value.id( CASUAL_FIELD_NO_ID);
                     // We found at least one
                     result = CASUAL_FIELD_SUCCESS;
                  }

                  value = value.next();

               }

               return result;

            }


            int remove( const char* const handle, const long id, long index)
            {
               if( !(id > CASUAL_FIELD_NO_ID))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     if( !index--)
                     {
                        value.id( CASUAL_FIELD_NO_ID);
                        return CASUAL_FIELD_SUCCESS;
                     }

                  }

                  value = value.next();

               }

               return CASUAL_FIELD_NO_OCCURRENCE;

            }


            int add( const char* const handle, const long id, const int type, const char* const data, const long size)
            {
               if( const auto result = validate_id( id, type))
               {
                  return result;
               }

               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               if( !data)
               {
                  return CASUAL_FIELD_INVALID_ARGUMENT;
               }


               const auto total = buffer.used() + Buffer::Value::header() + size;

               if( total > buffer.size())
               {
                  return CASUAL_FIELD_NO_SPACE;
               }

               Buffer::Value value = buffer.beyond();

               value.id( id);
               value.size( size);
               write( value.data(), data, size);

               buffer.used( total);

               return CASUAL_FIELD_SUCCESS;
            }

            template<typename T>
            int add( const char* const handle, const long id, const int type, const T data)
            {
               const auto encoded = common::network::byteorder<T>::encode( data);
               return add( handle, id, type, reinterpret_cast<const char*>( &encoded), sizeof encoded);
            }

            int get( const char* const handle, const long id, long index, const int type, const char** data, long* size)
            {
               if( const auto result = validate_id( id, type))
               {
                  return result;
               }


               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               //if( !data || !size)
               //{
               //   return CASUAL_FIELD_INVALID_ARGUMENT;
               //}


               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     if( !index--)
                     {
                        if( data) *data = value.data();
                        if( size) *size = value.size();
                        return CASUAL_FIELD_SUCCESS;
                     }

                  }

                  value = value.next();

               }

               return CASUAL_FIELD_NO_OCCURRENCE;

            }

            template<typename T>
            int get( const char* const handle, const long id, const long index, const int type, T* const value)
            {
               const char* data = nullptr;

               if( const auto result = get( handle, id, index, type, &data, nullptr))
               {
                  return result;
               }

               if( value) parse( data, *value);

               return CASUAL_FIELD_SUCCESS;
            }


            int first( const char* const handle, long& id, long& index)
            {
               const Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Buffer::Value value = buffer.first();
               const Buffer::Value beyond = buffer.beyond();

               if( value != beyond)
               {
                  id = value.id();
                  index = 0;
               }
               else
               {
                  return CASUAL_FIELD_NO_OCCURRENCE;
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int next( const char* const handle, long& id, long& index)
            {
               //
               // This may be inefficient for large buffers
               //
               // Perhaps we shall store the occurrence as well, but then
               // insertions and removals becomes a bit more cumbersome
               //
               // An other option is to make this and double-linked-list-like
               //

               const Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               //
               // Store all found id's to estimate index later
               //
               std::vector<decltype(Buffer::Value::id())> values;

               const Buffer::Value beyond = buffer.beyond();
               Buffer::Value value = buffer.first();

               while( value != beyond)
               {
                  values.push_back( value.id());

                  value = value.next();

                  if( values.back() == id)
                  {
                     if( !index--)
                     {
                        if( value != beyond)
                        {
                           id = value.id();
                           index = std::count( values.begin(), values.end(), id);

                           return CASUAL_FIELD_SUCCESS;
                        }
                        else
                        {
                           return CASUAL_FIELD_NO_OCCURRENCE;
                        }
                     }

                  }

               }

               //
               // We couldn't even find the previous one
               //
               return CASUAL_FIELD_INVALID_ID;

            }


            int reset( const char* const handle)
            {
               if( Buffer buffer = find_buffer( handle))
               {
                  buffer.used( Buffer::header());
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int explore( const char* const handle, long* const size, long* const used)
            {
               if( const Buffer buffer = find_buffer( handle))
               {
                  if( size) *size = buffer.size();
                  if( used) *used = buffer.used();
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }

         } //


      } // field

   } // buffer

} // casual


const char* CasualFieldDescription( const int code)
{
   switch( code)
   {
      case CASUAL_FIELD_SUCCESS:
         return "Success";
      case CASUAL_FIELD_NO_SPACE:
         return "No space";
      case CASUAL_FIELD_NO_OCCURRENCE:
         return "No occurrence";
      case CASUAL_FIELD_UNKNOWN_ID:
         return "Unknown id";
      case CASUAL_FIELD_INVALID_BUFFER:
         return "Invalid buffer";
      case CASUAL_FIELD_INVALID_ID:
         return "Invalid id";
      case CASUAL_FIELD_INVALID_TYPE:
         return "Invalid type";
      case CASUAL_FIELD_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_FIELD_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int CasualFieldExploreBuffer( const char* buffer, long* const size, long* const used)
{
   return casual::buffer::field::explore( buffer, size, used);
}

int CasualFieldAddChar( char* const buffer, const long id, const char value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_CHAR, value);
}

int CasualFieldAddShort( char* const buffer, const long id, const short value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_SHORT, value);
}

int CasualFieldAddLong( char* const buffer, const long id, const long value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_LONG, value);
}
int CasualFieldAddFloat( char* const buffer, const long id, const float value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_FLOAT, value);
}

int CasualFieldAddDouble( char* const buffer, const long id, const double value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_DOUBLE, value);
}

int CasualFieldAddString( char* const buffer, const long id, const char* const value)
{
   const auto count = std::strlen( value) + 1;
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_STRING, value, count);
}

int CasualFieldAddBinary( char* const buffer, const long id, const char* const value, const long count)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_BINARY, value, count);
}

int CasualFieldGetChar( const char* const buffer, const long id, const long index, char* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_CHAR, value);
}

int CasualFieldGetShort( const char* const buffer, const long id, const long index, short* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_SHORT, value);
}

int CasualFieldGetLong( const char* const buffer, const long id, const long index, long* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_LONG, value);
}

int CasualFieldGetFloat( const char* const buffer, const long id, const long index, float* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_FLOAT, value);
}

int CasualFieldGetDouble( const char* const buffer, const long id, const long index, double* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_DOUBLE, value);
}

int CasualFieldGetString( const char* const buffer, const long id, const long index, const char** value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_STRING, value, nullptr);
}

int CasualFieldGetBinary( const char* const buffer, const long id, const long index, const char** value, long* const count)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_BINARY, value, count);
}


namespace casual
{
   namespace buffer
   {
      namespace field
      {
         namespace
         {
            //
            // TODO: Much better
            //
            // This functionality might be used by some tooling as well
            //

            namespace repository
            {
               std::map<std::string,int> name_to_type()
               {
                  return decltype(name_to_type())
                  {
                     {"short",   CASUAL_FIELD_SHORT},
                     {"long",    CASUAL_FIELD_LONG},
                     {"char",    CASUAL_FIELD_CHAR},
                     {"float",   CASUAL_FIELD_FLOAT},
                     {"double",  CASUAL_FIELD_DOUBLE},
                     {"string",  CASUAL_FIELD_STRING},
                     {"binary",  CASUAL_FIELD_BINARY},
                  };
               }


               std::map<int,std::string> type_to_name()
               {
                  return decltype(type_to_name())
                  {
                     {CASUAL_FIELD_SHORT,    "short"},
                     {CASUAL_FIELD_LONG,     "long"},
                     {CASUAL_FIELD_CHAR,     "char"},
                     {CASUAL_FIELD_FLOAT,    "float"},
                     {CASUAL_FIELD_DOUBLE,   "double"},
                     {CASUAL_FIELD_STRING,   "string"},
                     {CASUAL_FIELD_BINARY,   "binary"},
                  };
               }

               struct field
               {
                  long id; // relative id
                  std::string name;
                  std::string type;

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( type);
                  }
               };

               struct group
               {
                  long base;
                  std::vector< field> fields;

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( base);
                     archive & CASUAL_MAKE_NVP( fields);
                  }
               };

               struct mapping
               {
                  long id;
                  std::string name;
               };

               std::vector<group> fetch_groups()
               {
                  decltype(fetch_groups()) groups;

                  try
                  {
                     const auto file = common::environment::variable::get( "CASUAL_FIELD_TABLE");

                     auto archive = sf::archive::reader::makeFromFile( file);

                     archive >> CASUAL_MAKE_NVP( groups);
                  }
                  catch( ...)
                  {
                     // Make sure this is logged once
                     common::error::handler();
                  }

                  return groups;

               }

               std::vector<field> fetch_fields()
               {
                  const auto groups = fetch_groups();

                  decltype(fetch_fields()) fields;

                  for( const auto& group : groups)
                  {
                     for( const auto& field : group.fields)
                     {
                        try
                        {
                           const auto id = name_to_type().at( field.type) * CASUAL_FIELD_TYPE_BASE + group.base + field.id;

                           if( id > CASUAL_FIELD_NO_ID)
                           {
                              fields.push_back( field);
                              fields.back().id = id;
                           }
                           else
                           {
                              // TODO: Much better
                              common::log::error << "id for " << field.name << " is invalid" << std::endl;
                           }
                        }
                        catch( const std::out_of_range&)
                        {
                           // TODO: Much better
                           common::log::error << "type for " << field.name << " is invalid" << std::endl;
                        }
                     }
                  }

                  return fields;

               }

               std::map<std::string,long> name_to_id()
               {
                  const auto fields = fetch_fields();

                  decltype( name_to_id()) result;

                  for( const auto& field : fields)
                  {
                     if( !result.emplace( field.name, field.id).second)
                     {
                        // TODO: Much better
                        common::log::error << "name for " << field.name << " is not unique" << std::endl;
                     }
                  }

                  return result;
               }

               std::map<long,std::string> id_to_name()
               {
                  const auto fields = fetch_fields();

                  decltype( id_to_name()) result;

                  for( const auto& field : fields)
                  {
                     if( !result.emplace( field.id, field.name).second)
                     {
                        // TODO: Much better
                        common::log::error << "id for " << field.name << " is not unique" << std::endl;
                     }
                  }

                  return result;

               }

            } // repository
         } //
      } // field
   } // buffer
} // casual


int CasualFieldNameOfId( const long id, const char** name)
{
   static const auto mapping = casual::buffer::field::repository::id_to_name();

   if( id > CASUAL_FIELD_NO_ID)
   {
      try
      {
         const auto& result = mapping.at( id);
         if( name) *name = result.c_str();
      }
      catch( const std::out_of_range&)
      {
         return CASUAL_FIELD_UNKNOWN_ID;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldIdOfName( const char* const name, long* const id)
{
   static const auto mapping = casual::buffer::field::repository::name_to_id();

   if( name)
   {
      try
      {
         const auto& result = mapping.at( name);
         if( id) *id = result;
      }
      catch( const std::out_of_range&)
      {
         return CASUAL_FIELD_UNKNOWN_ID;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldTypeOfId( const long id, int* const type)
{
   //
   // CASUAL_FIELD_SHORT is 0 'cause FLD_SHORT is 0 and thus we need to
   // represent invalid id with something else than 0
   //

   if( id > CASUAL_FIELD_NO_ID)
   {
      const int result = id / CASUAL_FIELD_TYPE_BASE;

      switch( result)
      {
         case CASUAL_FIELD_SHORT:
         case CASUAL_FIELD_LONG:
         case CASUAL_FIELD_CHAR:
         case CASUAL_FIELD_FLOAT:
         case CASUAL_FIELD_DOUBLE:
         case CASUAL_FIELD_STRING:
         case CASUAL_FIELD_BINARY:
            break;
         default:
            return CASUAL_FIELD_INVALID_ID;
      }

      if( type) *type = result;

   }
   else
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldNameOfType( const int type, const char** name)
{
   static const auto mapping = casual::buffer::field::repository::type_to_name();

   try
   {
      const auto& result = mapping.at( type);
      if( name) *name = result.c_str();
   }
   catch( const std::out_of_range&)
   {
      return CASUAL_FIELD_INVALID_TYPE;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldTypeOfName( const char* const name, int* const type)
{
   static const auto mapping = casual::buffer::field::repository::name_to_type();

   try
   {
      const auto& result = mapping.at( name);
      if( type) *type = result;
   }
   catch( const std::exception&)
   {
      return CASUAL_FIELD_INVALID_TYPE;
   }

   return CASUAL_FIELD_SUCCESS;
}


int CasualFieldExist( const char* const buffer, const long id, const long index)
{
   //
   // Perhaps we can use casual::buffer::field::next, but not yet
   //
   const int type = id / CASUAL_FIELD_TYPE_BASE;
   return casual::buffer::field::get( buffer, id, index, type, nullptr, nullptr);
}

int CasualFieldRemoveAll( char* const buffer)
{
   return casual::buffer::field::reset( buffer);
}

int CasualFieldRemoveId( char* buffer, const long id)
{
   return casual::buffer::field::remove( buffer, id);
}

int CasualFieldRemoveOccurrence( char* const buffer, const long id, long index)
{
   return casual::buffer::field::remove( buffer, id, index);
}

int CasualFieldCopyBuffer( char* target, const char* source)
{
   long used;
   if( const auto result = casual::buffer::field::explore( source, nullptr, &used))
   {
      return result;
   }

   long size;
   if( const auto result = casual::buffer::field::explore( target, &size, nullptr))
   {
      return result;
   }

   if( size < used)
   {
      return CASUAL_FIELD_NO_SPACE;
   }

   //
   // TODO: This is a little bit too much knowledge 'bout internal stuff
   //
   // This is to make sure the size-portion is not blown away
   //
   target += casual::buffer::field::size_size;
   source += casual::buffer::field::size_size;

   casual::buffer::field::write( target, source, used);

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldNext( const char* const buffer, long* const id, long* const index)
{
   if( !id || !index)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   if( *id == CASUAL_FIELD_NO_ID)
   {
      return casual::buffer::field::first( buffer, *id, *index);
   }
   else
   {
      return casual::buffer::field::next( buffer, *id, *index);
   }

}
