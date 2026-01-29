from setuptools import setup
from Cython.Build import cythonize
import os

script_files = [os.path.join("scripts", f) for f in os.listdir("scripts") if f.endswith(".py")]
setup(ext_modules=cythonize(script_files, language_level="3"))