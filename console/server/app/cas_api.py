import casual.server.api as api

class Casual():
  def __init__(self):
    self.call = api.call
  def __enter__(self):
    return self
  def __exit__(self, exc_type, exc_value, exc_traceback):
    pass
    
  
  