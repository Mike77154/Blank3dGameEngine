from pathlib import Path

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class BmpConan(ConanFile):
    name = "bmp"
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": True, "fPIC": True}
    exports_sources = (
        'CMakeLists.txt',
'cmake/*',
'exports/*',
'bmp/*',
'include/*',
'VERSION',
'PUBLIC_API.md',
'PACKAGING_ECOSYSTEM.md',
    )

    def set_version(self):
        self.version = Path(self.recipe_folder, "VERSION").read_text(encoding="utf-8").strip()

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.variables["BMP_BUILD_TESTS"] = False
        toolchain.variables["BMP_BUILD_TOOLS"] = False
        toolchain.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["bmp"]
        self.cpp_info.set_property("cmake_file_name", "bmp")
        self.cpp_info.set_property("cmake_target_name", "bmp::bmp")
