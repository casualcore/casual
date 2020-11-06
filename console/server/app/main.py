from flask import Flask, Blueprint, render_template
import logging
import json



logger = logging.getLogger(__name__)

from app.cas_api import Casual

logger.info("imports")

#app = Flask(__name__, template_folder='templates', static_folder='static')

bp = Blueprint("hello", __name__)

@bp.route("/")
def index():
  with Casual() as casual:
    logger.info("hellosan")
    response ="hello world"
    resp = casual.call(".casual/domain/state", "")
    #response = json.dumps(resp)
    return render_template("layout2.html")
