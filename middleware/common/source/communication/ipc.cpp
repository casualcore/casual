//!
//! casual
//!

#include "common/communication/ipc.h"
#include "common/communication/log.h"
#include "common/environment.h"
#include "common/error.h"
#include "common/trace.h"
#include "common/domain.h"


#include <fstream>

#include <sys/msg.h>

namespace casual
{
   namespace common
   {

      namespace communication
      {
         namespace ipc
         {

            namespace native
            {
               bool send( handle_type id, const message::Transport& transport, common::Flags< Flag> flags)
               {
                  //
                  // before we might block we check signals.
                  //
                  common::signal::handle();

                  auto result = ::msgsnd( id, &const_cast< message::Transport&>( transport).message, transport.size(), flags.underlaying());

                  if( result == -1)
                  {
                     auto code = errno;

                     switch( code)
                     {
                        case EAGAIN:
                        {
                           return false;
                        }
                        case EINTR:
                        {
                           log << "ipc::native::send - signal received\n";
                           common::signal::handle();

                           //
                           // we got a signal we don't have a handle for
                           // We continue
                           //
                           return send( id, transport, flags);
                        }
                        case EIDRM:
                        {
                           throw exception::queue::Unavailable{ "queue unavailable - id: " + std::to_string( id) + " - " + common::error::string()};
                        }
                        case ENOMEM:
                        {
                           throw exception::limit::Memory{ "id: " + std::to_string( id) + " - " + common::error::string()};
                        }
                        case EINVAL:
                        {
                           //if( /* message.size() < MSGMAX  && */ transport.message.header.type != common::message::Type::absent_message)
                           if( cast::underlying( transport.type()) > 0)
                           {
                              //
                              // The problem is with queue-id. We guess that it has been removed.
                              //
                              throw exception::queue::Unavailable{ "queue unavailable - id: " + std::to_string( id) + " - " + common::error::string()};
                           }
                           // we let it fall through to default
                        }
                        // no break
                        case EFAULT:
                        default:
                        {
                           throw common::exception::invalid::Argument( "invalid queue arguments - id: " + std::to_string( id) + " - " + common::error::string());
                        }
                     }
                  }

                  log << "---> [" << id << "] send transport: " << transport << " - flags: " << flags << '\n';

                  return true;
               }
               bool receive( handle_type id, message::Transport& transport, common::Flags< Flag> flags)
               {
                  //
                  // before we might block we check signals.
                  //
                  common::signal::handle();

                  auto result = msgrcv( id, &transport.message, transport.max_message_size(), 0, flags.underlaying());

                  if( result == -1)
                  {
                     auto code = errno;

                     switch( code)
                     {
                        case EINTR:
                        {
                           log << "ipc::native::receive - signal received\n";

                           common::signal::handle();

                           //
                           // we got a signal we don't have a handle for
                           // We continue
                           //
                           return receive( id, transport, flags);
                        }
                        case ENOMSG:
                        case EAGAIN:
                        {
                           return false;
                        }
                        case EIDRM:
                        {
                           throw exception::queue::Unavailable{ "queue removed - id: " + std::to_string( id) + " - " + common::error::string()};
                        }
                        default:
                        {
                           std::ostringstream msg;
                           msg << "ipc < [" << id << "] receive failed - transport: " << transport << " - flags: " << flags << " - " << common::error::string();
                           log << msg.str() << std::endl;
                           throw exception::invalid::Argument( msg.str(), __FILE__, __LINE__);
                        }
                     }
                  }

                  log << "<--- [" << id << "] receive transport: " << transport << " - flags: " << flags << '\n';

                  return true;

               }

            } // native

            namespace inbound
            {

               Connector::Connector()
                : m_id( msgget( IPC_PRIVATE, IPC_CREAT | 0660))
               {
                  if( m_id  == cInvalid)
                  {
                     throw exception::invalid::Argument( "ipc queue create failed - " + common::error::string(), __FILE__, __LINE__);
                  }
                  log << "queue id: " << m_id << " created\n";
               }

               Connector::~Connector()
               {
                  remove( m_id);
               }

               Connector::Connector( Connector&& rhs) noexcept
               {
                  swap( *this, rhs);
               }
               Connector& Connector::operator = ( Connector&& rhs) noexcept
               {
                  Connector temp{ std::move( rhs)};
                  swap( *this, temp);
                  return *this;
               }

               handle_type Connector::id() const { return m_id;}

               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ id: " << rhs.m_id << '}';
               }


               void swap( Connector& lhs, Connector& rhs)
               {
                  using std::swap;
                  swap( lhs.m_id, rhs.m_id);
               }


               Device& device()
               {
                  static Device singlton;
                  return singlton;
               }

               handle_type id()
               {
                  return device().connector().id();
               }
            } // inbound

            namespace outbound
            {
               Connector::Connector( handle_type id) : m_id{ id}
               {}

               Connector::operator handle_type() const { return m_id;}

               Connector::handle_type Connector::id() const { return m_id;}

               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ id: " << rhs.m_id << '}';
               }


               namespace instance
               {
                  namespace local
                  {
                     namespace
                     {

                        process::Handle fetch(
                              const Uuid& identity,
                              const std::string& environment,
                              process::instance::fetch::Directive directive)
                        {
                           Trace trace{ "ipc::outbound::instance::local::fetch"};

                           if( common::environment::variable::exists( environment))
                           {
                              auto process = environment::variable::process::get( environment);

                              if( communication::ipc::exists( process.queue))
                              {
                                 return process;
                              }
                           }

                           try
                           {
                              auto process = process::instance::fetch::handle( identity, directive);

                              if( ! environment.empty())
                              {
                                 environment::variable::process::set( environment, process);
                              }
                              return process;
                           }
                           catch( const exception::communication::Unavailable&)
                           {
                              log << "failed to fetch instance with identity: " << identity << '\n';
                              return {};
                           }
                        }

                     } // <unnamed>
                  } // local


                  template< process::instance::fetch::Directive directive>
                  Connector< directive>::Connector( const Uuid& identity, std::string environment)
                     : m_process( local::fetch( identity, environment, directive)),
                       m_identity{ identity}, m_environment{ std::move( environment)}
                  {

                  }


                  template< process::instance::fetch::Directive directive>
                  void Connector< directive>::reconnect()
                  {
                     Trace trace{ "ipc::outbound::instance::Connector::reconnect"};

                     m_process = local::fetch( m_identity, m_environment, directive);

                     if( ! communication::ipc::exists( m_process.queue))
                     {
                        throw exception::communication::Unavailable{ "failed to fetch ipc-queue for instance", CASUAL_NIP( m_identity), CASUAL_NIP( m_environment), CASUAL_NIP( m_process)};
                     }
                  }

                  template< process::instance::fetch::Directive directive>
                  std::ostream& operator << ( std::ostream& out, const Connector< directive>& rhs)
                  {
                     return out << "{ process: " << rhs.m_process
                           << ", identity:" << rhs.m_identity
                           << ", environment:" << rhs.m_environment
                           << '}';
                  }

                  template struct Connector< process::instance::fetch::Directive::direct>;
                  template struct Connector< process::instance::fetch::Directive::wait>;

               } // instance

               namespace domain
               {
                  namespace local
                  {
                     namespace
                     {

                        platform::ipc::id::type reconnect()
                        {
                           Trace trace{ "common::communication::ipc::outbound::domain::local::reconnect"};

                           auto from_environment = []()
                                 {
                                    if( environment::variable::exists( environment::variable::name::ipc::domain::manager()))
                                    {
                                       return environment::variable::process::get( environment::variable::name::ipc::domain::manager());
                                    }
                                    return process::Handle{};
                                 };

                           auto process = from_environment();


                           if( ipc::exists( process.queue))
                           {
                              return process.queue;
                           }

                           log << "failed to locate domain manager via " << environment::variable::name::ipc::domain::manager() << " - trying 'singleton file'\n";

                           auto from_singleton_file = []()
                                 {
                                    std::ifstream file{ common::environment::domain::singleton::file()};

                                    struct
                                    {
                                       process::Handle process;
                                       struct
                                       {
                                          std::string name;
                                          std::string id;
                                       } identity;
                                    } domain_info;

                                    if( file)
                                    {
                                       file >> domain_info.process.queue;
                                       file >> domain_info.process.pid;
                                       file >> domain_info.identity.name;
                                       file >> domain_info.identity.id;

                                       environment::variable::process::set( environment::variable::name::ipc::domain::manager(), domain_info.process);
                                       common::domain::identity( { domain_info.identity.id, domain_info.identity.name});

                                       log << "domain information - id: " << common::domain::identity() << ", process: " << domain_info.process << '\n';
                                    }
                                    return domain_info.process;
                                 };

                           process = from_singleton_file();

                           if( ! ipc::exists( process.queue))
                           {
                              throw exception::communication::Unavailable{ "failed to locate domain manager"};
                           }

                           return process.queue;
                        }

                     } // <unnamed>
                  } // local

                  Connector::Connector() : outbound::Connector{ local::reconnect()}
                  {

                  }

                  void Connector::reconnect()
                  {
                     Trace trace{ "ipc::outbound::domain::Connector::reconnect"};

                     m_id = local::reconnect();
                  }
               } // domain

            } // outbound




            namespace broker
            {
               outbound::instance::Device& device()
               {
                  static outbound::instance::Device singelton{
                     process::instance::identity::broker(),
                     common::environment::variable::name::ipc::broker()};

                  return singelton;
               }

            } // broker

            namespace transaction
            {
               namespace manager
               {
                  outbound::instance::Device& device()
                  {
                     static outbound::instance::Device singelton{
                        process::instance::identity::transaction::manager(),
                        common::environment::variable::name::ipc::transaction::manager()};
                     return singelton;
                  }

               } // manager
            } // transaction

            namespace gateway
            {
               namespace manager
               {
                  outbound::instance::Device& device()
                  {
                     static outbound::instance::Device singelton{
                        process::instance::identity::gateway::manager(),
                        environment::variable::name::ipc::gateway::manager()};

                     return singelton;
                  }

                  namespace optional
                  {
                     outbound::instance::optional::Device& device()
                     {
                        static outbound::instance::optional::Device singelton{
                           process::instance::identity::gateway::manager(),
                           environment::variable::name::ipc::gateway::manager()};

                        return singelton;
                     }
                  } // optional

               } // manager
            } // gateway

            namespace queue
            {
               namespace broker
               {
                  outbound::instance::Device& device()
                  {
                     static outbound::instance::Device singelton{
                        process::instance::identity::queue::broker(),
                        environment::variable::name::ipc::queue::broker()};

                     return singelton;
                  }

                  namespace optional
                  {
                     outbound::instance::optional::Device& device()
                     {
                        static outbound::instance::optional::Device singelton{
                           process::instance::identity::queue::broker(),
                           environment::variable::name::ipc::queue::broker()};

                        return singelton;
                     }
                  } // optional
               } // broker
            } // queue


            namespace domain
            {
               namespace manager
               {
                  outbound::domain::Device& device()
                  {
                     static outbound::domain::Device singelton;
                     return singelton;
                  }

               } // manager
            } // domain




            bool exists( handle_type id)
            {
               struct msqid_ds info;

               return msgctl( id, IPC_STAT, &info) == 0;
            }

            bool remove( handle_type id)
            {
               if( id != -1)
               {
                  if( msgctl( id, IPC_RMID, nullptr) == 0)
                  {
                     log << "queue id: " << id << " removed\n";
                     return true;
                  }
                  else
                  {
                     log::error << "failed to remove ipc-queue with id: " << id << " - " << common::error::string() << "\n";
                  }
               }
               return false;
            }

            bool remove( const process::Handle& owner)
            {
               struct msqid_ds info;

               if( msgctl( owner.queue, IPC_STAT, &info) != 0)
               {
                  return false;
               }
               if( info.msg_lrpid == owner.pid)
               {
                  return remove( owner.queue);
               }
               return false;
            }

         } // ipc

      } // communication
   } // common
} // casual
