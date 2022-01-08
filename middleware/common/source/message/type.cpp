//!
//! Copyright (c) 2021
//!
//! This software is licensed under the MIT license
//!

#include "common/message/type.h"

namespace casual
{
   namespace common::message
   {
      std::ostream& operator << ( std::ostream& out, Type value)
      {
         switch( value)
         {
            case Type::absent_message: return out << "absent_message";
            case Type::flush_ipc: return out << "flush_ipc";
            case Type::poke: return out << "poke";
            case Type::shutdown_request: return out << "shutdown_request";
            case Type::shutdown_reply: return out << "shutdown_reply";
            case Type::delay_message: return out << "delay_message";
            case Type::inbound_ipc_connect: return out << "inbound_ipc_connect";
            case Type::process_lookup_request: return out << "process_lookup_request";
            case Type::process_lookup_reply: return out << "process_lookup_reply";
            case Type::domain_process_connect_request: return out << "domain_process_connect_request";
            case Type::domain_process_connect_reply: return out << "domain_process_connect_reply";
            case Type::domain_process_prepare_shutdown_request: return out << "domain_process_prepare_shutdown_request";
            case Type::domain_process_prepare_shutdown_reply: return out << "domain_process_prepare_shutdown_reply";
            case Type::domain_manager_shutdown_request: return out << "domain_manager_shutdown_request";
            case Type::domain_manager_shutdown_reply: return out << "domain_manager_shutdown_reply";
            case Type::domain_process_lookup_request: return out << "domain_process_lookup_request";
            case Type::domain_process_lookup_reply: return out << "domain_process_lookup_reply";
            case Type::domain_configuration_request: return out << "domain_configuration_request";
            case Type::domain_configuration_reply: return out << "domain_configuration_reply";
            case Type::domain_server_configuration_request: return out << "domain_server_configuration_request";
            case Type::domain_server_configuration_reply: return out << "domain_server_configuration_reply";
            case Type::domain_pending_message_connect_request: return out << "domain_pending_message_connect_request";
            case Type::domain_pending_message_connect_reply: return out << "domain_pending_message_connect_reply";
            case Type::domain_pending_message_send_request: return out << "domain_pending_message_send_request";
            case Type::domain_instance_global_state_request: return out << "domain_instance_global_state_request";
            case Type::domain_instance_global_state_reply: return out << "domain_instance_global_state_reply";
            case Type::domain_discovery_internal_registration_request: return out << "domain_discovery_internal_registration_request";
            case Type::domain_discovery_internal_registration_reply: return out << "domain_discovery_internal_registration_reply";
            case Type::domain_discovery_external_registration_request: return out << "domain_discovery_external_registration_request";
            case Type::domain_discovery_external_registration_reply: return out << "domain_discovery_external_registration_reply";
            case Type::domain_discovery_external_request: return out << "domain_discovery_external_request";
            case Type::domain_discovery_external_reply: return out << "domain_discovery_external_reply";
            case Type::domain_discovery_external_advertised_request: return out << "domain_discovery_external_advertised_request";
            case Type::domain_discovery_external_advertised_reply: return out << "domain_discovery_external_advertised_reply";
            case Type::domain_discovery_external_rediscovery_request: return out << "domain_discovery_external_rediscovery_request";
            case Type::domain_discovery_external_rediscovery_reply: return out << "domain_discovery_external_rediscovery_reply";
            case Type::server_connect_request: return out << "server_connect_request";
            case Type::server_connect_reply: return out << "server_connect_reply";
            case Type::server_disconnect: return out << "server_disconnect";
            case Type::server_ping_request: return out << "server_ping_request";
            case Type::server_ping_reply: return out << "server_ping_reply";
            case Type::service_advertise: return out << "service_advertise";
            case Type::service_name_lookup_request: return out << "service_name_lookup_request";
            case Type::service_name_lookup_reply: return out << "service_name_lookup_reply";
            case Type::service_name_lookup_discard_request: return out << "service_name_lookup_discard_request";
            case Type::service_name_lookup_discard_reply: return out << "service_name_lookup_discard_reply";
            case Type::service_call: return out << "service_call";
            case Type::service_reply: return out << "service_reply";
            case Type::service_acknowledge: return out << "service_acknowledge";
            case Type::service_concurrent_advertise: return out << "service_concurrent_advertise";
            case Type::conversation_connect_request: return out << "conversation_connect_request";
            case Type::conversation_connect_reply: return out << "conversation_connect_reply";
            case Type::conversation_send: return out << "conversation_send";
            case Type::conversation_disconnect: return out << "conversation_disconnect";
            case Type::event_subscription_begin: return out << "event_subscription_begin";
            case Type::event_subscription_end: return out << "event_subscription_end";
            case Type::event_idle: return out << "event_idle";
            case Type::event_task: return out << "event_task";
            case Type::event_sub_task: return out << "event_sub_task";
            case Type::event_error: return out << "event_error";
            case Type::event_notification: return out << "event_notification";
            case Type::event_process_spawn: return out << "event_process_spawn";
            case Type::event_process_exit: return out << "event_process_exit";
            case Type::event_process_configured: return out << "event_process_configured";
            case Type::event_process_assassination_contract: return out << "event_process_assassination_contract";
            case Type::event_discoverable_avaliable: return out << "event_discoverable_avaliable";
            case Type::event_domain_information: return out << "event_domain_information";
            case Type::event_service_call: return out << "event_service_call";
            case Type::event_service_calls: return out << "event_service_calls";
            case Type::transaction_client_connect_request: return out << "transaction_client_connect_request";
            case Type::transaction_client_connect_reply: return out << "transaction_client_connect_reply";
            case Type::transaction_manager_connect_request: return out << "transaction_manager_connect_request";
            case Type::transaction_manager_connect_reply: return out << "transaction_manager_connect_reply";
            case Type::transaction_manager_configuration: return out << "transaction_manager_configuration";
            case Type::transaction_manager_ready: return out << "transaction_manager_ready";
            case Type::transaction_resource_proxy_configuration_request: return out << "transaction_resource_proxy_configuration_request";
            case Type::transaction_resource_proxy_configuration_reply: return out << "transaction_resource_proxy_configuration_reply";
            case Type::transaction_resource_proxy_ready: return out << "transaction_resource_proxy_ready";
            case Type::transaction_configuration_alias_request: return out << "transaction_configuration_alias_request";
            case Type::transaction_configuration_alias_reply: return out << "transaction_configuration_alias_reply";
            case Type::transaction_begin_request: return out << "transaction_begin_request";
            case Type::transaction_begin_reply: return out << "transaction_begin_reply";
            case Type::transaction_commit_request: return out << "transaction_commit_request";
            case Type::transaction_commit_reply: return out << "transaction_commit_reply";
            case Type::transaction_rollback_request: return out << "transaction_rollback_request";
            case Type::transaction_rollback_reply: return out << "transaction_rollback_reply";
            case Type::transaction_generic_reply: return out << "transaction_generic_reply";
            case Type::transaction_resource_prepare_request: return out << "transaction_resource_prepare_request";
            case Type::transaction_resource_prepare_reply: return out << "transaction_resource_prepare_reply";
            case Type::transaction_resource_commit_request: return out << "transaction_resource_commit_request";
            case Type::transaction_resource_commit_reply: return out << "transaction_resource_commit_reply";
            case Type::transaction_resource_rollback_request: return out << "transaction_resource_rollback_request";
            case Type::transaction_resource_rollback_reply: return out << "transaction_resource_rollback_reply";
            case Type::transaction_resource_lookup_request: return out << "transaction_resource_lookup_request";
            case Type::transaction_resource_lookup_reply: return out << "transaction_resource_lookup_reply";
            case Type::transaction_resource_involved_request: return out << "transaction_resource_involved_request";
            case Type::transaction_resource_involved_reply: return out << "transaction_resource_involved_reply";
            case Type::transaction_external_resource_involved: return out << "transaction_external_resource_involved";
            case Type::transaction_resource_id_request: return out << "transaction_resource_id_request";
            case Type::transaction_resource_id_reply: return out << "transaction_resource_id_reply";
            case Type::queue_manager_queue_advertise: return out << "queue_manager_queue_advertise";
            case Type::queue_manager_queue_lookup_request: return out << "queue_manager_queue_lookup_request";
            case Type::queue_manager_queue_lookup_reply: return out << "queue_manager_queue_lookup_reply";
            case Type::queue_manager_queue_lookup_discard_request: return out << "queue_manager_queue_lookup_discard_request";
            case Type::queue_manager_queue_lookup_discard_reply: return out << "queue_manager_queue_lookup_discard_reply";
            case Type::queue_group_enqueue_request: return out << "queue_group_enqueue_request";
            case Type::queue_group_enqueue_reply: return out << "queue_group_enqueue_reply";
            case Type::queue_group_dequeue_request: return out << "queue_group_dequeue_request";
            case Type::queue_group_dequeue_reply: return out << "queue_group_dequeue_reply";
            case Type::queue_group_dequeue_forget_request: return out << "queue_group_dequeue_forget_request";
            case Type::queue_group_dequeue_forget_reply: return out << "queue_group_dequeue_forget_reply";
            case Type::queue_group_connect: return out << "queue_group_connect";
            case Type::queue_group_configuration_update_request: return out << "queue_group_configuration_update_request";
            case Type::queue_group_configuration_update_reply: return out << "queue_group_configuration_update_reply";
            case Type::queue_group_state_request: return out << "queue_group_state_request";
            case Type::queue_group_state_reply: return out << "queue_group_state_reply";
            case Type::queue_group_message_meta_request: return out << "queue_group_message_meta_request";
            case Type::queue_group_message_meta_reply: return out << "queue_group_message_meta_reply";
            case Type::queue_group_message_meta_peek_request: return out << "queue_group_message_meta_peek_request";
            case Type::queue_group_message_meta_peek_reply: return out << "queue_group_message_meta_peek_reply";
            case Type::queue_group_message_peek_request: return out << "queue_group_message_peek_request";
            case Type::queue_group_message_peek_reply: return out << "queue_group_message_peek_reply";
            case Type::queue_group_message_remove_request: return out << "queue_group_message_remove_request";
            case Type::queue_group_message_remove_reply: return out << "queue_group_message_remove_reply";
            case Type::queue_group_queue_restore_request: return out << "queue_group_queue_restore_request";
            case Type::queue_group_queue_restore_reply: return out << "queue_group_queue_restore_reply";
            case Type::queue_group_queue_clear_request: return out << "queue_group_queue_clear_request";
            case Type::queue_group_queue_clear_reply: return out << "queue_group_queue_clear_reply";
            case Type::queue_group_metric_reset_request: return out << "queue_group_metric_reset_request";
            case Type::queue_group_metric_reset_reply: return out << "queue_group_metric_reset_reply";
            case Type::queue_forward_group_connect: return out << "queue_forward_group_connect";
            case Type::queue_forward_group_configuration_update_request: return out << "queue_forward_group_configuration_update_request";
            case Type::queue_forward_group_configuration_update_reply: return out << "queue_forward_group_configuration_update_reply";
            case Type::queue_forward_group_state_request: return out << "queue_forward_group_state_request";
            case Type::queue_forward_group_state_reply: return out << "queue_forward_group_state_reply";
            case Type::gateway_inbound_connect: return out << "gateway_inbound_connect";
            case Type::gateway_inbound_configuration_update_request: return out << "gateway_inbound_configuration_update_request";
            case Type::gateway_inbound_configuration_update_reply: return out << "gateway_inbound_configuration_update_reply";
            case Type::gateway_inbound_state_request: return out << "gateway_inbound_state_request";
            case Type::gateway_inbound_state_reply: return out << "gateway_inbound_state_reply";
            case Type::gateway_reverse_inbound_state_request: return out << "gateway_reverse_inbound_state_request";
            case Type::gateway_reverse_inbound_state_reply: return out << "gateway_reverse_inbound_state_reply";
            case Type::gateway_outbound_connect: return out << "gateway_outbound_connect";
            case Type::gateway_outbound_configuration_update_request: return out << "gateway_outbound_configuration_update_request";
            case Type::gateway_outbound_configuration_update_reply: return out << "gateway_outbound_configuration_update_reply";
            case Type::gateway_outbound_state_request: return out << "gateway_outbound_state_request";
            case Type::gateway_outbound_state_reply: return out << "gateway_outbound_state_reply";
            case Type::gateway_reverse_outbound_state_request: return out << "gateway_reverse_outbound_state_request";
            case Type::gateway_reverse_outbound_state_reply: return out << "gateway_reverse_outbound_state_reply";
            case Type::gateway_domain_connect_request: return out << "gateway_domain_connect_request";
            case Type::gateway_domain_connect_reply: return out << "gateway_domain_connect_reply";
            case Type::gateway_domain_disconnect_request: return out << "gateway_domain_disconnect_request";
            case Type::gateway_domain_disconnect_reply: return out << "gateway_domain_disconnect_reply";
            case Type::gateway_domain_connected: return out << "gateway_domain_connected";
            case Type::domain_discovery_request: return out << "domain_discovery_request";
            case Type::domain_discovery_reply: return out << "domain_discovery_reply";
            case Type::configuration_request: return out << "configuration_request";
            case Type::configuration_reply: return out << "configuration_reply";
            case Type::configuration_supplier_registration: return out << "configuration_supplier_registration";
            case Type::configuration_update_request: return out << "configuration_update_request";
            case Type::configuration_update_reply: return out << "configuration_update_reply";
            case Type::configuration_put_request: return out << "configuration_put_request";
            case Type::configuration_put_reply: return out << "configuration_put_reply";
            case Type::signal_timeout: return out << "signal_timeout";
            case Type::signal_hangup: return out << "signal_hangup";
            case Type::cli_pipe_done: return out << "cli_pipe_done";
            case Type::cli_payload: return out << "cli_payload";
            case Type::cli_queue_message_id: return out << "cli_queue_message_id";
            case Type::cli_pipe_error_fatal: return out << "cli_pipe_error_fatal";
            case Type::cli_transaction_directive: return out << "cli_transaction_directive";
            case Type::cli_transaction_directive_terminated: return out << "cli_transaction_directive_terminated";
            case Type::cli_transaction_associated: return out << "cli_transaction_associated";
            case Type::cli_transaction_finalize: return out << "cli_transaction_finalize";
            case Type::cli_transaction_propagate: return out << "cli_transaction_propagate";
            case Type::internal_dump_state: return out << "internal_dump_state";
            case Type::unittest_message: return out << "unittest_message";

            // end markers...
            case Type::EVENT_DOMAIN_BASE_END: return out << "EVENT_DOMAIN_BASE_END";
            case Type::EVENT_SERVICE_BASE_END: return out << "EVENT_SERVICE_BASE_END";
         }
         return out << "<unknown>";

      }
   }
}