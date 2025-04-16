from setuptools import setup, Extension

module = Extension(
    'fwlib',
    sources=['examples/python-c-extension/fwlib.c'],
    include_dirs=['.'],  # Look in current directory for fwlib32.h
    library_dirs=['.'],  # Look in current directory for the library
    libraries=['fwlib32-linux-x64'],  # Name without 'lib' prefix and .so suffix
    runtime_library_dirs=['.']  # Add this line
)

setup(
    name='fwlib',
    version='0.1',
    description='Python wrapper for FANUC fwlib32 library',
    ext_modules=[module]
)