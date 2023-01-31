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
      std::string_view description( Type value) noexcept
      {
         switch( value)
         {
            case Type::absent_message: return "absent_message";
            case Type::flush_ipc: return "flush_ipc";
            case Type::poke: return "poke";
            case Type::shutdown_request: return "shutdown_request";
            case Type::shutdown_reply: return "shutdown_reply";
            case Type::delay_message: return "delay_message";
            case Type::inbound_ipc_connect: return "inbound_ipc_connect";
            case Type::process_lookup_request: return "process_lookup_request";
            case Type::process_lookup_reply: return "process_lookup_reply";
            case Type::domain_process_connect_request: return "domain_process_connect_request";
            case Type::domain_process_connect_reply: return "domain_process_connect_reply";
            case Type::domain_process_prepare_shutdown_request: return "domain_process_prepare_shutdown_request";
            case Type::domain_process_prepare_shutdown_reply: return "domain_process_prepare_shutdown_reply";
            case Type::domain_manager_shutdown_request: return "domain_manager_shutdown_request";
            case Type::domain_manager_shutdown_reply: return "domain_manager_shutdown_reply";
            case Type::domain_process_lookup_request: return "domain_process_lookup_request";
            case Type::domain_process_lookup_reply: return "domain_process_lookup_reply";
            case Type::domain_process_information_request: return "domain_process_information_request";
            case Type::domain_process_information_reply: return "domain_process_information_reply";
            case Type::domain_configuration_request: return "domain_configuration_request";
            case Type::domain_configuration_reply: return "domain_configuration_reply";
            case Type::domain_server_configuration_request: return "domain_server_configuration_request";
            case Type::domain_server_configuration_reply: return "domain_server_configuration_reply";
            case Type::domain_pending_message_connect_request: return "domain_pending_message_connect_request";
            case Type::domain_pending_message_connect_reply: return "domain_pending_message_connect_reply";
            case Type::domain_pending_message_send_request: return "domain_pending_message_send_request";
            case Type::domain_instance_global_state_request: return "domain_instance_global_state_request";
            case Type::domain_instance_global_state_reply: return "domain_instance_global_state_reply";
            case Type::domain_discovery_api_provider_registration_request: return "domain_discovery_api_provider_registration_request";
            case Type::domain_discovery_api_provider_registration_reply: return "domain_discovery_api_provider_registration_reply";
            case Type::domain_discovery_api_request: return "domain_discovery_api_request";
            case Type::domain_discovery_api_reply: return "domain_discovery_api_reply";
            case Type::domain_discovery_api_rediscovery_request: return "domain_discovery_api_rediscovery_request";
            case Type::domain_discovery_api_rediscovery_reply: return "domain_discovery_api_rediscovery_reply";
            case Type::domain_discovery_needs_request: return "domain_discovery_needs_request";
            case Type::domain_discovery_needs_reply: return "domain_discovery_needs_reply";
            case Type::domain_discovery_known_request: return "domain_discovery_known_request";
            case Type::domain_discovery_known_reply: return "domain_discovery_known_reply";
            case Type::server_connect_request: return "server_connect_request";
            case Type::server_connect_reply: return "server_connect_reply";
            case Type::server_disconnect: return "server_disconnect";
            case Type::server_ping_request: return "server_ping_request";
            case Type::server_ping_reply: return "server_ping_reply";
            case Type::service_advertise: return "service_advertise";
            case Type::service_name_lookup_request: return "service_name_lookup_request";
            case Type::service_name_lookup_reply: return "service_name_lookup_reply";
            case Type::service_name_lookup_discard_request: return "service_name_lookup_discard_request";
            case Type::service_name_lookup_discard_reply: return "service_name_lookup_discard_reply";
            case Type::service_call: return "service_call";
            case Type::service_reply: return "service_reply";
            case Type::service_acknowledge: return "service_acknowledge";
            case Type::service_concurrent_advertise: return "service_concurrent_advertise";
            case Type::conversation_connect_request: return "conversation_connect_request";
            case Type::conversation_connect_reply: return "conversation_connect_reply";
            case Type::conversation_send: return "conversation_send";
            case Type::conversation_disconnect: return "conversation_disconnect";
            case Type::event_subscription_begin: return "event_subscription_begin";
            case Type::event_subscription_end: return "event_subscription_end";
            case Type::event_idle: return "event_idle";
            case Type::event_task: return "event_task";
            case Type::event_sub_task: return "event_sub_task";
            case Type::event_error: return "event_error";
            case Type::event_notification: return "event_notification";
            case Type::event_process_spawn: return "event_process_spawn";
            case Type::event_process_exit: return "event_process_exit";
            case Type::event_process_configured: return "event_process_configured";
            case Type::event_process_assassination_contract: return "event_process_assassination_contract";
            case Type::event_domain_information: return "event_domain_information";
            case Type::event_service_call: return "event_service_call";
            case Type::event_service_calls: return "event_service_calls";
            case Type::transaction_client_connect_request: return "transaction_client_connect_request";
            case Type::transaction_client_connect_reply: return "transaction_client_connect_reply";
            case Type::transaction_manager_connect_request: return "transaction_manager_connect_request";
            case Type::transaction_manager_connect_reply: return "transaction_manager_connect_reply";
            case Type::transaction_manager_configuration: return "transaction_manager_configuration";
            case Type::transaction_manager_ready: return "transaction_manager_ready";
            case Type::transaction_resource_proxy_configuration_request: return "transaction_resource_proxy_configuration_request";
            case Type::transaction_resource_proxy_configuration_reply: return "transaction_resource_proxy_configuration_reply";
            case Type::transaction_resource_proxy_ready: return "transaction_resource_proxy_ready";
            case Type::transaction_configuration_alias_request: return "transaction_configuration_alias_request";
            case Type::transaction_configuration_alias_reply: return "transaction_configuration_alias_reply";
            case Type::transaction_begin_request: return "transaction_begin_request";
            case Type::transaction_begin_reply: return "transaction_begin_reply";
            case Type::transaction_commit_request: return "transaction_commit_request";
            case Type::transaction_commit_reply: return "transaction_commit_reply";
            case Type::transaction_rollback_request: return "transaction_rollback_request";
            case Type::transaction_rollback_reply: return "transaction_rollback_reply";
            case Type::transaction_generic_reply: return "transaction_generic_reply";
            case Type::transaction_resource_prepare_request: return "transaction_resource_prepare_request";
            case Type::transaction_resource_prepare_reply: return "transaction_resource_prepare_reply";
            case Type::transaction_resource_commit_request: return "transaction_resource_commit_request";
            case Type::transaction_resource_commit_reply: return "transaction_resource_commit_reply";
            case Type::transaction_resource_rollback_request: return "transaction_resource_rollback_request";
            case Type::transaction_resource_rollback_reply: return "transaction_resource_rollback_reply";
            case Type::transaction_resource_lookup_request: return "transaction_resource_lookup_request";
            case Type::transaction_resource_lookup_reply: return "transaction_resource_lookup_reply";
            case Type::transaction_resource_involved_request: return "transaction_resource_involved_request";
            case Type::transaction_resource_involved_reply: return "transaction_resource_involved_reply";
            case Type::transaction_external_resource_involved: return "transaction_external_resource_involved";
            case Type::transaction_resource_id_request: return "transaction_resource_id_request";
            case Type::transaction_resource_id_reply: return "transaction_resource_id_reply";
            case Type::queue_manager_queue_advertise: return "queue_manager_queue_advertise";
            case Type::queue_manager_queue_lookup_request: return "queue_manager_queue_lookup_request";
            case Type::queue_manager_queue_lookup_reply: return "queue_manager_queue_lookup_reply";
            case Type::queue_manager_queue_lookup_discard_request: return "queue_manager_queue_lookup_discard_request";
            case Type::queue_manager_queue_lookup_discard_reply: return "queue_manager_queue_lookup_discard_reply";
            case Type::queue_group_enqueue_request: return "queue_group_enqueue_request";
            case Type::queue_group_enqueue_reply: return "queue_group_enqueue_reply";
            case Type::queue_group_dequeue_request: return "queue_group_dequeue_request";
            case Type::queue_group_dequeue_reply: return "queue_group_dequeue_reply";
            case Type::queue_group_dequeue_forget_request: return "queue_group_dequeue_forget_request";
            case Type::queue_group_dequeue_forget_reply: return "queue_group_dequeue_forget_reply";
            case Type::queue_group_connect: return "queue_group_connect";
            case Type::queue_group_configuration_update_request: return "queue_group_configuration_update_request";
            case Type::queue_group_configuration_update_reply: return "queue_group_configuration_update_reply";
            case Type::queue_group_state_request: return "queue_group_state_request";
            case Type::queue_group_state_reply: return "queue_group_state_reply";
            case Type::queue_group_message_meta_request: return "queue_group_message_meta_request";
            case Type::queue_group_message_meta_reply: return "queue_group_message_meta_reply";
            case Type::queue_group_message_meta_peek_request: return "queue_group_message_meta_peek_request";
            case Type::queue_group_message_meta_peek_reply: return "queue_group_message_meta_peek_reply";
            case Type::queue_group_message_peek_request: return "queue_group_message_peek_request";
            case Type::queue_group_message_peek_reply: return "queue_group_message_peek_reply";
            case Type::queue_group_message_browse_request: return "queue_group_message_browse_request";
            case Type::queue_group_message_browse_reply: return "queue_group_message_browse_reply";
            case Type::queue_group_message_remove_request: return "queue_group_message_remove_request";
            case Type::queue_group_message_remove_reply: return "queue_group_message_remove_reply";
            case Type::queue_group_message_recovery_request: return "queue_group_message_recovery_request";
            case Type::queue_group_message_recovery_reply: return "queue_group_message_recovery_reply";
            case Type::queue_group_queue_restore_request: return "queue_group_queue_restore_request";
            case Type::queue_group_queue_restore_reply: return "queue_group_queue_restore_reply";
            case Type::queue_group_queue_clear_request: return "queue_group_queue_clear_request";
            case Type::queue_group_queue_clear_reply: return "queue_group_queue_clear_reply";
            case Type::queue_group_metric_reset_request: return "queue_group_metric_reset_request";
            case Type::queue_group_metric_reset_reply: return "queue_group_metric_reset_reply";
            case Type::queue_forward_group_connect: return "queue_forward_group_connect";
            case Type::queue_forward_group_configuration_update_request: return "queue_forward_group_configuration_update_request";
            case Type::queue_forward_group_configuration_update_reply: return "queue_forward_group_configuration_update_reply";
            case Type::queue_forward_group_state_request: return "queue_forward_group_state_request";
            case Type::queue_forward_group_state_reply: return "queue_forward_group_state_reply";
            case Type::gateway_inbound_connect: return "gateway_inbound_connect";
            case Type::gateway_inbound_configuration_update_request: return "gateway_inbound_configuration_update_request";
            case Type::gateway_inbound_configuration_update_reply: return "gateway_inbound_configuration_update_reply";
            case Type::gateway_inbound_state_request: return "gateway_inbound_state_request";
            case Type::gateway_inbound_state_reply: return "gateway_inbound_state_reply";
            case Type::gateway_inbound_connection_lost: return "gateway_inbound_connection_lost";
            case Type::gateway_reverse_inbound_state_request: return "gateway_reverse_inbound_state_request";
            case Type::gateway_reverse_inbound_state_reply: return "gateway_reverse_inbound_state_reply";
            case Type::gateway_outbound_connect: return "gateway_outbound_connect";
            case Type::gateway_outbound_configuration_update_request: return "gateway_outbound_configuration_update_request";
            case Type::gateway_outbound_configuration_update_reply: return "gateway_outbound_configuration_update_reply";
            case Type::gateway_outbound_state_request: return "gateway_outbound_state_request";
            case Type::gateway_outbound_state_reply: return "gateway_outbound_state_reply";
            case Type::gateway_outbound_connection_lost: return "gateway_outbound_connection_lost";
            case Type::gateway_reverse_outbound_state_request: return "gateway_reverse_outbound_state_request";
            case Type::gateway_reverse_outbound_state_reply: return "gateway_reverse_outbound_state_reply";
            case Type::gateway_domain_connect_request: return "gateway_domain_connect_request";
            case Type::gateway_domain_connect_reply: return "gateway_domain_connect_reply";
            case Type::gateway_domain_disconnect_request: return "gateway_domain_disconnect_request";
            case Type::gateway_domain_disconnect_reply: return "gateway_domain_disconnect_reply";
            case Type::gateway_domain_connected: return "gateway_domain_connected";
            case Type::domain_discovery_request: return "domain_discovery_request";
            case Type::domain_discovery_reply: return "domain_discovery_reply";
            case Type::domain_discovery_internal_request: return "domain_discovery_internal_request";
            case Type::domain_discovery_internal_reply: return "domain_discovery_internal_reply";
            case Type::domain_discovery_topology_implicit_update: return "domain_discovery_topology_implicit_update";
            case Type::domain_discovery_topology_direct_update: return "domain_discovery_topology_direct_update";
            case Type::domain_discovery_topology_direct_explore: return "domain_discovery_topology_direct_explore";
            case Type::configuration_request: return "configuration_request";
            case Type::configuration_reply: return "configuration_reply";
            case Type::configuration_supplier_registration: return "configuration_supplier_registration";
            case Type::configuration_update_request: return "configuration_update_request";
            case Type::configuration_update_reply: return "configuration_update_reply";
            case Type::configuration_put_request: return "configuration_put_request";
            case Type::configuration_put_reply: return "configuration_put_reply";
            case Type::signal_timeout: return "signal_timeout";
            case Type::signal_hangup: return "signal_hangup";
            case Type::cli_pipe_done: return "cli_pipe_done";
            case Type::cli_payload: return "cli_payload";
            case Type::cli_queue_message: return "cli_queue_message";
            case Type::cli_queue_message_id: return "cli_queue_message_id";
            case Type::cli_pipe_error_fatal: return "cli_pipe_error_fatal";
            case Type::cli_transaction_directive: return "cli_transaction_directive";
            case Type::cli_transaction_directive_terminated: return "cli_transaction_directive_terminated";
            case Type::cli_transaction_associated: return "cli_transaction_associated";
            case Type::cli_transaction_finalize: return "cli_transaction_finalize";
            case Type::cli_transaction_propagate: return "cli_transaction_propagate";
            case Type::internal_dump_state: return "internal_dump_state";
            case Type::internal_configure_log: return "internal_configure_log";
            case Type::unittest_message: return "unittest_message";

            // end markers...
            case Type::EVENT_DOMAIN_BASE_END: return "EVENT_DOMAIN_BASE_END";
            case Type::EVENT_SERVICE_BASE_END: return "EVENT_SERVICE_BASE_END";
         }
         return "<unknown>";

      }
   }
}