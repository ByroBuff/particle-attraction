# setup.py
from setuptools import setup, Extension

cpartsim_module = Extension(
    'cpartsim',
    sources=['cpartsim.c'],
    extra_compile_args=['-O3'],   # optimize aggressively
)

setup(
    name='cpartsim',
    version='0.1',
    ext_modules=[cpartsim_module],
)
