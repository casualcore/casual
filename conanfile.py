from conan import ConanFile
from conan.tools.files import get
from conan.tools.layout import basic_layout
import os
import shutil

class CasualConan(ConanFile):
    name = "casual"
    settings = "os", "compiler", "build_type", "arch"

    # Stäng av automatiska miljöfiler
    virtualbuildenv = False
    virtualrunenv = False

    def requirements(self):
        self.requires("yaml-cpp/0.8.0")
        self.requires("pugixml/1.15")
        self.requires("gtest/1.16.0")
        self.requires("cppcodec/0.2")
        self.requires("tomlplusplus/3.4.0")
        self.requires("rapidjson/1.1.0")        

    def generate(self):
        # Skapa en lokal mapp i ditt projekt för externa bibliotek
        thirdparty = os.getenv('CASUAL_THIRDPARTY') if os.getenv('CASUAL_THIRDPARTY') else os.getenv('CASUAL_MAKE_SOURCE_ROOT') + "/../casual-thirdparty"
        local_include_dir = os.path.join(thirdparty, "include")
        local_lib_dir = os.path.join(thirdparty, "lib")
        os.makedirs(local_include_dir, exist_ok=True)
        os.makedirs(local_lib_dir, exist_ok=True)

        for dep_name, dep in self.dependencies.items():
            dep_cpp = dep.cpp_info.aggregated_components()
            
            # Kopiera header-filer
            for inc_path in dep_cpp.includedirs:
                abs_inc = os.path.join(dep.package_folder, inc_path)
                if os.path.exists(abs_inc):
                    # Kopiera mappen eller dess innehåll till din lokala sökväg
                    shutil.copytree(abs_inc, local_include_dir, dirs_exist_ok=True)

            # Kopiera lib-filer
            for lib_path in dep_cpp.libdirs:
                abs_lib = os.path.join(dep.package_folder, lib_path)
                if os.path.exists(abs_lib):
                    for file in os.listdir(abs_lib):
                        if file.endswith((".a", ".lib", ".so", ".dylib")):
                            shutil.copy(os.path.join(abs_lib, file), local_lib_dir)