r"""
Currently, this package is experimental and may change in the future.
"""
from __future__ import absolute_import


#------------------------------------------------------------------------------
# this little trick is for static builds of SVTK. In such builds, if
# the user imports this Python package in a non-statically linked Python
# interpreter i.e. not of the of the SVTK-python executables, then we import the
# static components importer module.
try:
    from . import svtkCommonCore
except ImportError:
    import _svtkmodules_static
#------------------------------------------------------------------------------
