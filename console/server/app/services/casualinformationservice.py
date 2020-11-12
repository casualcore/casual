from . import CasualService

class CasualInformationService(CasualService):
    def __init__(self):
        super(CasualInformationService, self).__init__()

    def information(self):
        
        return self.get_servers()