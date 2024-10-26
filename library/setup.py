# Requirements:     pip install pybind11
# Location:         cd library
# Build:            python setup.py build_ext --inplace
# For local tests:  pip install -e .

from setuptools import setup, Extension
import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext
import sys

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "fairbenchtiny",
        ["bindings.cpp"],
        extra_compile_args=["-O2"],
    ),
]

# Run setup
setup(
    name="fairbenchtiny",
    version="0.1",
    description="A c++ implementation for fairbech",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
