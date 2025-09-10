import os
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
    description = ("EDIE (Encode Decode Interface Engine) is a C++ SDK that can encode and decode messages "
                   "from NovAtel's OEM7 receivers from one format into another.")
    url = "https://github.com/novatel/novatel_edie"
    license = "MIT"
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    exports_sources = ["cmake/*", "database/*", "include/*", "src/*", "LICENSE", "!doc", "!test", "CMakeLists.txt"]

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
        self.requires("spdlog/[>=1.13 <2]", transitive_headers=True, transitive_libs=True, force=True)
        self.requires("gegles-spdlog_setup/[>=1.1 <2]", transitive_headers=True)
        # fmt/11.1.1 is currently not compatible with spdlog https://github.com/gabime/spdlog/issues/3302
        self.requires("fmt/11.0.2", force=True)

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
        tc = CMakeToolchain(self)
        tc.user_presets_path = False
        tc.cache_variables["BUILD_BENCHMARKS"] = False
        tc.cache_variables["BUILD_EXAMPLES"] = False
        tc.cache_variables["BUILD_TESTS"] = False
        tc.cache_variables["CMAKE_INSTALL_DATADIR"] = "res"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

        # A workaround for conan_provider.cmake not loading conan_toolchain.cmake
        # TODO: replace with conan_cmakedeps_paths.cmake from the new CMakeDeps generator:
        # https://github.com/conan-io/conan/issues/7240#issuecomment-2443768898
        toolchain_lines = Path(self.generators_folder, "conan_toolchain.cmake").read_text().splitlines()
        runtime_paths = "\n".join(l for l in toolchain_lines if ("CONAN_RUNTIME_LIB_DIRS" in l or "CMAKE_PROGRAM_PATH" in l))
        Path(self.generators_folder, "conan_runtime_paths.cmake").write_text(runtime_paths)

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
