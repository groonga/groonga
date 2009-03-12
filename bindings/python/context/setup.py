#!/usr/bin/python
from distutils.core import setup, Extension

cflags = ['-Wall', '-fPIC', '-g', '-O0']
#cflags = ['-Wall', '-fPIC', '-O3']

ext = Extension('groongactx',
                libraries = ['groonga'],
                sources = ['groongactx.c'],
                extra_compile_args = cflags)

setup(name = 'groongactx',
      version = '1.0',
      description = 'python GQTP',
      long_description = '''
      This is a GQTP Python API package.
      ''',
      license='GNU LESSER GENERAL PUBLIC LICENSE',
      author = 'Brazil',
      author_email = 'groonga at razil.jp',
      ext_modules = [ext]
     )
