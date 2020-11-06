import app.routes.api as api
from app.routes.open import b_open



def get_routes():
  return api.b_api, b_open


