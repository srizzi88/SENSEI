svtk_module_find_package(
  PACKAGE ADIOS2
  VERSION 2.4)
if (SVTK_USE_MPI AND NOT ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "SVTK built with MPI requires ADIOS2 built with MPI")
elseif(NOT SVTK_USE_MPI AND ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "SVTK built without MPI requires ADIOS2 built without MPI")
endif()

# Initial empty set of files
set(classes_core)
set(private_classes_core)
set(private_headers_core)
set(private_templates_core)

# ADIOS2 Core Image Reader
set(classes_core svtkADIOS2CoreImageReader)
set(private_classes_core Core/svtkADIOS2CoreArraySelection)
set(private_headers_core Core/svtkADIOS2CoreTypeTraits.h)
set(private_templates_core)
set(svtk_io_adios2_core_enabled TRUE CACHE INTERNAL "" FORCE)

if (svtk_io_adios2_core_enabled)
  list(APPEND classes   ${classes_core})
  list(APPEND private_classes   ${private_classes_core})
  list(APPEND private_headers   ${private_headers_core})
  list(APPEND private_templates ${private_templates_core})
endif()

set(classes_vtx svtkADIOS2VTXReader)
set(private_classes_vtx
  VTX/VTXSchemaManager
  VTX/common/VTXDataArray
  VTX/common/VTXHelper
  VTX/schema/VTXSchema
  VTX/schema/svtk/VTXsvtkBase
  VTX/schema/svtk/VTXsvtkVTI
  VTX/schema/svtk/VTXsvtkVTU)
set(private_headers_vtx VTX/common/VTXTypes.h)
set(private_templates_vtx
  VTX/common/VTXHelper.txx
  VTX/schema/VTXSchema.txx
  VTX/schema/svtk/VTXsvtkVTI.txx
  VTX/schema/svtk/VTXsvtkVTU.txx)

if (SVTK_USE_MPI)
  set(svtk_io_adios2_vtx_enabled TRUE CACHE INTERNAL "" FORCE)
else ()
  set(svtk_io_adios2_vtx_enabled FALSE CACHE INTERNAL "" FORCE)
endif()

if (svtk_io_adios2_vtx_enabled)
  list(APPEND classes   ${classes_vtx})
  list(APPEND private_classes   ${private_classes_vtx})
  list(APPEND private_headers   ${private_headers_vtx})
  list(APPEND private_templates ${private_templates_vtx})
endif ()

svtk_module_add_module(SVTK::IOADIOS2
  CLASSES   ${classes}
  PRIVATE_CLASSES   ${private_classes}
  PRIVATE_HEADERS   ${private_headers}
  PRIVATE_TEMPLATES ${private_templates})
svtk_module_link(SVTK::IOADIOS2 PRIVATE adios2::adios2)

if (ADIOS2_HAVE_MPI)
  svtk_module_definitions(SVTK::IOADIOS2 PRIVATE IOADIOS2_HAVE_MPI)
endif ()
