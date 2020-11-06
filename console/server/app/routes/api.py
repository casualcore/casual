from flask import request, Blueprint, make_response, jsonify, session
from flask import current_app as app

from app.services.casualservice import CasualService

b_api = Blueprint("b_api", __name__)

@b_api.route("/api/v1/casual/info", methods=["GET"])
def casual_information():
  try:
      casual = CasualService()
      information = casual.get_casual_information()
  except Exception as e:
      app.logger.exception("An error occurred fetching groups", extra={'stack':True})
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


@b_api.route("/api/v1/executables/get", methods=["GET"])
def executables():
    try:

        casual = CasualService()
        executables = casual.get_executables()

        return make_response(jsonify(executables), 200)
    except Exception as e:

        app.logger.exception("An error occurred fetching executables", extra={'stack':True})
        return make_response({"message": str(e)}), 500


@b_api.route("/api/v1/services/get")
def services():
    try:

        casual = CasualService()
        services = casual.get_services()

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
