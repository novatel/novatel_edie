import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.files import load


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"
    test_type = "explicit"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            bin_path = os.path.join(self.cpp.build.bindir, "CommandEncoding")
            database_json = self.dependencies["novatel_edie"].runenv_info.vars(self).get("EDIE_DATABASE_FILE")
            format = "ASCII"
            command = "RTKTIMEOUT 30"
            self.run(f"{bin_path} {database_json} {format} {command}", env="conanrun")
            print(load(self, os.path.join(self.build_folder, "COMMAND.ASCII")).strip())
