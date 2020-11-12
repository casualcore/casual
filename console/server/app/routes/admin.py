from flask import request, Blueprint, make_response, jsonify, session
from flask import current_app as app

from app.services import CasualService, CasualInformationService

b_admin = Blueprint("b_admin", __name__)

@b_admin.route("/api/v1/service/metric/reset", methods=["POST"])
def reset_metrics():
    try:
        content = request.get_json()
        services = content.get('services')
        casual = CasualService()
        reset_service = casual.reset_metrics(services)
        return make_response(jsonify(reset_service), 200)
    except Exception as e:
        app.logger.exception("An error occurred resetting service metrics", extra={'stack':True})
        return make_response({"message": str(e)}), 500

    