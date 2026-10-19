from conan import ConanFile
from conan.tools.files import get
from conan.tools.layout import basic_layout
import os

class NginxConan(ConanFile):
    __doc__ = """
    Use conan source path_to_file
    """
    name = "nginx"
    version = "1.30.4"
    description = "Robust, small and high-performance HTTP server and reverse proxy"
    license = "2-clause BSD-like license"
    topics = ("server", "http", "proxy")
    
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_ssl": [True, False],
        "with_http_v2": [True, False]
    }
    default_options = {
        "with_ssl": True,
        "with_http_v2": True
    }

    def layout(self):
        thirdparty = os.getenv('CASUAL_THIRDPARTY') if os.getenv('CASUAL_THIRDPARTY') else os.getenv('CASUAL_MAKE_SOURCE_ROOT') + "/../casual-thirdparty"
        basic_layout(self, src_folder=thirdparty + "/src")

    def source(self):
        # Laddar ner källkoden direkt från Nginx officiella webbplats
        get(self, f"https://nginx.org/download/nginx-{self.version}.tar.gz")

        target = f"nginx-{self.version}"
        link_name = "nginx"
        
        # Ta bort länken om den redan finns sedan tidigare
        if os.path.lexists(link_name):
            os.unlink(link_name)
            
        # Skapa symlink (target_is_directory=True krävs för mappar på Windows)
        os.symlink(target, link_name, target_is_directory=True)
