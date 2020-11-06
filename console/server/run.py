from app import create_app
from waitress import serve

from os import getenv
import sys
PORT=getenv("CASUAL_CONSOLE_BACKEND_PORT", 8001)
import signal


listen = "*:{}".format(PORT)

def stop_handler(signum, stack_frame):
  sys.exit(0)

def serve_app():
  serve(create_app(port=PORT),listen=listen, _quiet=True)



if __name__ == '__main__':
  signal.signal(signal.SIGINT, stop_handler)
  signal.signal(signal.SIGTERM, stop_handler)
  serve_app()