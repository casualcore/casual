import os
import shutil

import subprocess
from pathlib import Path

from conan import ConanFile
from conan.tools.layout import basic_layout

class CasualConan(ConanFile):
    name = "casual"
    settings = "os", "compiler", "build_type", "arch"

    # Stäng av automatiska miljöfiler
    virtualbuildenv = False
    virtualrunenv = False

    def requirements(self):
        self.requires("pugixml/1.15")
        self.requires("gtest/1.16.0")
        self.requires("cppcodec/0.2")

