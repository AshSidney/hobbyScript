import os

from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.build.cross_building import cross_building

class CppScriptTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def build(self):
        cmake = cmake_layout(self)
        # Current dir is "test_package/build/<build_id>" and CMakeLists.txt is
        # in "test_package"
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
        self.copy('*.so*', dst='bin', src='lib')

    def test(self):
        if not cross_building(self):
            os.chdir("bin")
            self.run(".%sexample" % os.sep)
