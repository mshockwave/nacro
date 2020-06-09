#!/usr/bin/env python3
import sys,re
import importlib

if len(sys.argv) < 2:
    sys.exit(1)

module_name = str(sys.argv[1])
the_module = importlib.import_module(module_name)

if hasattr(the_module, '__file__'):
    # has __init__.py file
    print(re.compile('/__init__.py.*').sub('',the_module.__file__))
elif hasattr(the_module, '__path__'):
    path = str(the_module.__path__)
    print(re.compile(r'(_NamespacePath|[\[\]\(\)\'])').sub('', path))
else:
    sys.exit(1)
