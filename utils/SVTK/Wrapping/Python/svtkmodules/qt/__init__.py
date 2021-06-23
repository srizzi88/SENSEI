"""Qt module for SVTK/Python.

Example usage:

    import sys
    import PyQt5
    from PyQt5.QtWidgets import QApplication
    from svtkmodules.qt.QSVTKRenderWindowInteractor import QSVTKRenderWindowInteractor

    app = QApplication(sys.argv)

    widget = QSVTKRenderWindowInteractor()
    widget.Initialize()
    widget.Start()

    renwin = widget.GetRenderWindow()

For more information, see QSVTKRenderWidgetConeExample() in the file
QSVTKRenderWindowInteractor.py.
"""

import sys

# PyQtImpl can be set by the user
PyQtImpl = None

# Has an implementation has been imported yet?
for impl in ["PyQt5", "PySide2", "PyQt4", "PySide"]:
    if impl in sys.modules:
        PyQtImpl = impl
        break

# QSVTKRWIBase, base class for QSVTKRenderWindowInteractor,
# can be altered by the user to "QGLWidget" in case
# of rendering errors (e.g. depth check problems, readGLBuffer
# warnings...)
QSVTKRWIBase = "QWidget"

__all__ = ['QSVTKRenderWindowInteractor']
