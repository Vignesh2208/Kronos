from distutils.core import setup, Extension
setup(name='kronos_functions', version='1.2',  \
      ext_modules=[Extension('kronos_functions',
                  library_dirs = ['/usr/local/lib','/usr/lib/i386-linux-gnu'],
                  sources = ['py_extensions.c', 'Kronos_functions.c', 'kronos_utility_functions.c'])])
