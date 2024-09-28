from conan import ConanFile

class NovatelEdiePackage(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "nlohmann_json/[>=3.11 <3.12]",
        "gegles-spdlog_setup/[>=1.1 <2]",
        "spdlog/[>=1.13 <2]"
    ]
    default_options = {
        # Statically linking against spdlog causes its singleton registry to be
        # re-instantiated in each shared library or executable that links against it.
        "spdlog/*:shared": True,
        "fmt/*:shared": True
    }
    generators = "CMakeDeps", "CMakeToolchain"
    cmake_layout = "cmake_layout"

    def build_requirements(self):
        self.test_requires("gtest/[>=1.14 <1.15]")
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.tool_requires("patchelf/0.18")
