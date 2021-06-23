"""This is the svtk module."""
import sys

if sys.version_info < (3,5):
    # imp is deprecated in 3.4
    import imp, importlib

    # import svtkmodules package.
    svtkmodules_m = importlib.import_module('svtkmodules')

    # import svtkmodules.all
    all_m = importlib.import_module('svtkmodules.all')

    # create a clone of the `svtkmodules.all` module.
    svtk_m = imp.new_module(__name__)
    for key in dir(all_m):
        if not hasattr(svtk_m, key):
            setattr(svtk_m, key, getattr(all_m, key))

    # make the clone of `svtkmodules.all` act as a package at the same location
    # as svtkmodules. This ensures that importing modules from within the svtkmodules package
    # continues to work.
    svtk_m.__path__ = svtkmodules_m.__path__
    # replace old `svtk` module with this new package.
    sys.modules[__name__] = svtk_m

else:
    import importlib
    # import svtkmodules.all
    all_m = importlib.import_module('svtkmodules.all')

    # import svtkmodules
    svtkmodules_m = importlib.import_module('svtkmodules')

    # make svtkmodules.all act as the svtkmodules package to support importing
    # other modules from svtkmodules package via `svtk`.
    all_m.__path__ = svtkmodules_m.__path__

    # replace old `svtk` module with the `all` package.
    sys.modules[__name__] = all_m
