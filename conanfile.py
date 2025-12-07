from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake
import os.path

class CppScriptConan(ConanFile):
    name = "hobbyScript"
    version = "0.1"
    license = "<Put the package license here>"
    author = "Zdeno Ash Miklas ashsidney@gmail.com"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of HobbyScript here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "CppScript/include/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires("taocpp-pegtl/3.2.8")
        self.requires("gtest/1.15.0")

    def layout(self):
        cmake_layout(self, src_folder="CppScript")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="CppScript/include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["hobbyScript"]
        if not self.options.shared:
            self.cpp_info.defines.append("_NOEXPORT")
