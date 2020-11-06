
import logging
from os import getenv


log_file = getenv('CASUAL_DOMAIN_HOME', '/tmp/casual/console') + "/console.log"


def create_logger(app):
  
  logging.basicConfig(filename=log_file, 
                      level=logging.DEBUG,
                      format='%(asctime)s|%(name)s|%(levelname)s|%(message)s')

  #handler = logging.StreamHandler()
  #handler.setFormatter(logging.Formatter(
  #     '%(asctime)s|%(name)s|%(levelname)s|%(message)s'))
  #app.logger.addHandler(handler)
  #app.logger.removeHandler(flog.default_handler)
  #app.logger.setLevel(logging.DEBUG)
  app.logger.setLevel(logging.DEBUG)
  app.logger.info("Casual Admin Console listening on port: {}".format(app.config['PORT']))



