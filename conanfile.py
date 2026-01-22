import os
import sys
import re
from pathlib import Path
from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, cmake_layout, CMakeToolchain, CMakeDeps
from conan.tools.files import copy, rmdir

required_conan_version = ">=2.4.0"

class NovatelEdieConan(ConanFile):
    name = "novatel_edie"
    description = (
        "EDIE (Encode Decode Interface Engine) is a C++ SDK that can encode and decode messages "
        "from NovAtel's OEM7 receivers from one format into another.")
    url = "https://github.com/novatel/novatel_edie"
    license = "MIT"
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "tests": [True, False],
        "examples": [True, False],
        "benchmarks": [True, False],
        "python": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "tests": False,
        "examples": False,
        "benchmarks": False,
        "python": False,
        "fPIC": True,
    }

    exports_sources = [
        "cmake/*", "database/*", "include/*", "src/*", "python/*",
        "LICENSE", "!doc", "!test", "CMakeLists.txt"]

    def _get_python(self) -> str:
        """Retrieves the path to the python executable to be used in the build."""
        # Prefer user-provided conf
        user_python = self.conf.get("user.python:exe", default=None)
        if user_python:
            return user_python

        # Fallback to the current interpreter
        if sys.executable:
            return sys.executable
        raise ConanInvalidConfiguration(
            "Python executable not found. Set it via -c user.python:exe=/path/to/python"
        )

    def _deploy_shared_libs(self):
        """Copies shared library binaries of dependencies into a known location."""
        # Both Debug and Release folders will be created when using a cmake driven build
        third_party_dep_path = (
            Path(self.build_folder)
            / "third_party_libs"
            / str(self.settings.build_type) # Debug or Release
        )
        for dep in self.dependencies.values():
            search_dirs = dep.cpp_info.libdirs + dep.cpp_info.bindirs
            for search_dir in search_dirs:
                copy(self, "*.dll", search_dir, third_party_dep_path)
                copy(self, "*.dylib", search_dir, third_party_dep_path)
                copy(self, "*.so", search_dir, third_party_dep_path)

    def set_version(self):
        cmakelists_content = Path(self.recipe_folder, "CMakeLists.txt").read_text()
        self.version = re.search(r"set\(RELEASE_VERSION ([\d.]+)\)", cmakelists_content).group(1)
        print(f"Detected version: {self.version}")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        print(f"Configuring conan with the following options: {self.options}")
        if self.options.shared:
            self.options.rm_safe("fPIC")
            self.options["spdlog"].shared = True

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("simdjson/3.10.1", transitive_headers=True, transitive_libs=True, force=True)
        self.requires("nlohmann_json/[>=3.11 <3.12]", transitive_headers=True)
        self.requires("spdlog/[>=1.15 <1.16]", transitive_headers=True, transitive_libs=True, force=True)
        self.requires("gegles-spdlog_setup/[>=1.1 <2]", transitive_headers=True)
        self.requires("fmt/[>=12.0.0 <13]", force=True)

    def build_requirements(self):
        self.test_requires("gtest/[>=1.14 <1.15]")
        self.test_requires("benchmark/[>=1.8 <2]")
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.tool_requires("patchelf/[*]")

    def validate(self):
        check_min_cppstd(self, 17)

        if self.options.shared and not self.dependencies["spdlog"].options.shared:
            # Statically linking against spdlog causes its singleton registry to be
            # re-instantiated in each shared library and executable that links against it.
            raise ConanInvalidConfiguration("spdlog must be dynamically linked when building novatel_edie as a shared library")


    def generate(self):
        """Deploys pre-requistes to the build folder."""
        # Determine whether conan is being invoked as part of a cmake configuration
        cmake_driven = self.conf.get("user.novatel_edie:cmake_driven", default=False)

        # Deploy CMake information for dependencies
        deps = CMakeDeps(self)
        deps.generate()

        # Deploy any shared libraries
        self._deploy_shared_libs()

        if not cmake_driven:
            # Configure a CMakeToolchain if conan is not invoked by an existing one
            tc = CMakeToolchain(self)
            tc.user_presets_path = "ConanPresets.json"
            tc.cache_variables["CONAN_BUILD_DIR"] = str(self.build_folder)
            tc.cache_variables["BUILD_BENCHMARKS"] = self.options.benchmarks
            tc.cache_variables["BUILD_EXAMPLES"] = self.options.examples
            tc.cache_variables["BUILD_TESTS"] = self.options.tests
            tc.cache_variables["CMAKE_INSTALL_DATADIR"] = "res"
            if self.options.python:
                tc.cache_variables["BUILD_PYTHON"] = True
                python_exe = self._get_python()
                tc.cache_variables["Python_EXECUTABLE"] = python_exe
                tc.cache_variables["PYTHON_INSTALL_DIR"] = "python_package/novatel_edie"
                print(f"Using python at: {python_exe}")
            tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        rmdir(self, os.path.join(self.source_folder, "lib", "cmake"))

    def package_info(self):
        """Provides information to consumers."""
        self.cpp_info.set_property("cmake_file_name", "novatel_edie")
        self.cpp_info.set_property("cmake_target_name", "novatel_edie::novatel_edie")
        self.cpp_info.resdirs = ["res"]
        self.cpp_info.libs = [
            # Note: the order of the listed libs matters when linking statically.
            "edie_oem_decoder",
            "edie_decoders_common",
            "edie_common",
        ]
        db_path = os.path.join(self.package_folder, "res", "novatel_edie", "database.json")
        self.runenv_info.define_path("EDIE_DATABASE_FILE", db_path)
