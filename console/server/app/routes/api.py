from flask import request, Blueprint, make_response, jsonify, session
from flask import current_app as app

from app.services import CasualService, CasualInformationService

b_api = Blueprint("b_api", __name__)

@b_api.route("/api/v1/casual/info", methods=["GET"])
def casual_information():
  try:
      casual = CasualInformationService()
      information = casual.information()
      return make_response(jsonify(information), 200)

  except Exception as e:
      app.logger.exception("An error occurred fetchin Casual Information", extra={'stack':True})
      return make_response({"message": str(e)}), 500

@b_api.route("/api/v1/groups/get", methods=["GET"])
def groups():
    try:

        casual = CasualService()
        groups = casual.get_groups()
        return make_response(jsonify(groups), 200)
    except Exception as e:
        app.logger.exception("An error occurred fetching groups", extra={'stack':True})
        return make_response({"message": str(e)}), 500

@b_api.route("/api/v1/servers/get", methods=["GET"])
def servers():
    try:

        casual = CasualService()
        servers = casual.get_servers()

        return make_response(jsonify(servers), 200)
    except Exception as e:
        app.logger.exception("An error occurred fetching servers", extra={'stack':True})
        return make_response({"message": str(e)}), 500

@b_api.route("/api/v1/servers/get/server", methods=["GET"])
def get_server_by_alias():
    try:
        alias = request.args.get('alias')
        if alias is None:
            raise "query missing"
        casual = CasualService()
        server = casual.get_server_by_alias(alias)
        return make_response(jsonify(server), 200)
    except Exception as e:
        app.logger.exception("An error occurred fetching server", extra={'stack':True})
        return make_response({"message": str(e)}), 500


@b_api.route("/api/v1/executables/get", methods=["GET"])
def executables():
    try:

        casual = CasualService()
        executables = casual.get_executables()

        return make_response(jsonify(executables), 200)
    except Exception as e:

        app.logger.exception("An error occurred fetching executables", extra={'stack':True})
        return make_response({"message": str(e)}), 500
@b_api.route("/api/v1/executables/get/executable", methods=["GET"])
def get_executable_by_alias():
    try:
        alias = request.args.get('alias')
        if alias is None:
            raise "query missing"
        casual = CasualService()
        executable = casual.get_executable_by_alias(alias)
        return make_response(jsonify(executable), 200)
    except Exception as e:
        app.logger.exception("An error occurred fetching executable", extra={'stack':True})
        return make_response({"message": str(e)}), 500


@b_api.route("/api/v1/services/get")
def get_service_by_name():
    try:

        casual = CasualService()
        services = casual.get_services()

        return make_response(jsonify(services), 200)
    except Exception as e:
        app.logger.exception("An error occurred fetching services", extra={'stack':True})
        return make_response({"message": str(e)}), 500

@b_api.route("/api/v1/services/get/service")
def services():
    try:
        name = request.args.get('name')
        if name is None:
            raise "Service not found"
        casual = CasualService()
        services = casual.get_service_by_name(name)

        return make_response(jsonify(services), 200)
    except Exception as e:
        app.logger.exception("An error occurred fetching services", extra={'stack':True})
        return make_response({"message": str(e)}), 500

@b_api.route("/api/v1/gateways/get")
def gateways():
    try:

        casual = CasualService()
        gateways = casual.get_gateways()

        return make_response(jsonify(gateways), 200)
    except Exception as e:
        app.logger.exception("An error occurred fetching gateways", extra={'stack':True})
        return make_response({"message": str(e)}), 500

@b_api.route("/api/v1/transactions/get")
def transactions():
    try:

        casual = CasualService()
        transactions = casual.get_transactions()

        return make_response(jsonify(transactions), 200)
    except Exception as e:
        return make_response({"message": str(e)}), 500
