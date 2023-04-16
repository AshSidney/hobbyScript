from conans import ConanFile, CMake, tools
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
    generators = "cmake"
    exports_sources = "CppScript/include/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires("nlohmann_json/3.9.1")
        self.requires("gtest/1.11.0")
        #bgfxDir = 'build/bgfx'
        #if not os.path.exists(bgfxDir):
        #    git = tools.Git(folder=bgfxDir)
        #    git.clone('https://github.com/firefalcom/conan-bgfx.git')
        #self.run('conan install .', cwd=bgfxDir)
        #self.requires("bgfx/7188")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="CppScript")
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
