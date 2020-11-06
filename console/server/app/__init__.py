from flask import Flask
#import logging
from app.tools.log.logger import create_logger
from os import getenv






def create_app(port=8001):
  app = Flask(__name__, template_folder='templates', static_folder='static')
  
  from flask_cors import CORS 
  CORS(app)

  config_setup = getenv("CONSOLE_CONFIG", None)
  
  if config_setup is not None:
    app.config.from_object(config_setup)
  else:
    app.config.from_object("app.config.config.ProductionConfig")

  #from flask_cors import CORS
  #from flask_assets import Environment, Bundle
  #from app.assets.assets import bundles

  import app.routes.routes as routes

  for route in routes.get_routes():
    app.register_blueprint(route)

  #assets = Environment(app)
  #assets.register(bundles)

  create_logger(app)


  #from . import main
  #app.register_blueprint(main.bp)


  #@app.before_request
  #def access_log():
  #    app.logger.info(request)

  return app



