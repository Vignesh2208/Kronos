from distutils.core import setup, Extension
setup(name='kronos_functions', version='1.0',  \
      ext_modules=[Extension('kronos_functions', include_dirs=['/usr/local/include','/usr/local/include/python3.2'], library_dirs = ['/usr/local/lib','/usr/lib/i386-linux-gnu'], sources = ['py_extensions.c', 'Kronos_functions.c', 'utility_functions.c'])])
