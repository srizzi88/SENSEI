#
# Mangled VTK
#
add_library(sSVTK INTERFACE)

if (FALSE)

find_package(SVTK CONFIG QUIET)
if (NOT SVTK_FOUND)
  message(FATAL_ERROR "SVTK is required for Sensei core even when not using "
    "any infrastructures. Please set `SVTK_DIR` to point to a directory "
    "containing `SVTKConfig.cmake` or `vtk-config.cmake`.")
endif()

set(SENSEI_SVTK_COMPONENTS CommonDataModel)

find_package(SVTK CONFIG QUIET COMPONENTS ${SENSEI_SVTK_COMPONENTS})
if (NOT SVTK_FOUND)
  message(FATAL_ERROR "SVTK (${SENSEI_SVTK_COMPONENTS}) modules are required for "
    "Sensei core even when not using any infrastructures. Please set "
    "`SVTK_DIR` to point to a directory containing `SVTKConfig.cmake` or "
    "`vtk-config.cmake`.")
endif()

else()
set(SVTK_LIBRARIES SVTK::CommonDataModel)
endif()

target_link_libraries(sSVTK INTERFACE ${SVTK_LIBRARIES})

install(TARGETS sSVTK EXPORT sSVTK)
install(EXPORT sSVTK DESTINATION lib/cmake EXPORT_LINK_INTERFACE_LIBRARIES)
