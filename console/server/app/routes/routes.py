import app.routes.api as api
import app.routes.admin as admin
#from app.routes.open import b_open



def get_routes():
  return [api.b_api, admin.b_admin]


