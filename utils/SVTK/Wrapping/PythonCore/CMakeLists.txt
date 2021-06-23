set(classes
  svtkPythonArgs
  svtkPythonCommand
  svtkPythonOverload
  svtkPythonUtil
  svtkSmartPyObject)

set(headers
  PySVTKEnum.h
  PySVTKMethodDescriptor.h
  PySVTKNamespace.h
  PySVTKObject.h
  PySVTKReference.h
  PySVTKSpecialObject.h
  PySVTKTemplate.h
  svtkPythonCompatibility.h)

set(sources
  PySVTKEnum.cxx
  PySVTKExtras.cxx
  PySVTKMethodDescriptor.cxx
  PySVTKNamespace.cxx
  PySVTKObject.cxx
  PySVTKReference.cxx
  PySVTKSpecialObject.cxx
  PySVTKTemplate.cxx)

set(private_headers
  PySVTKExtras.h
)

svtk_module_add_module(SVTK::WrappingPythonCore
  CLASSES ${classes}
  SOURCES ${sources}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  LIBRARY_NAME_SUFFIX "${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}")
