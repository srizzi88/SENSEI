cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(Medical)

find_package(SVTK
  COMPONENTS
    CommonCore
    CommonDataModel
    CommonTransforms
    FiltersCore
    FiltersGeneral
    FiltersGeometry
    FiltersModeling
    IOImage
    IOXML
    ImagingCore
    ImagingStatistics
    InteractionStyle
    RenderingCore
    RenderingVolume
    RenderingOpenGL2
    RenderingVolumeOpenGL2
  OPTIONAL_COMPONENTS
    TestingCore
    TestingRendering)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(Medical1 MACOSX_BUNDLE Medical1.cxx)
add_executable(Medical2 MACOSX_BUNDLE Medical2.cxx)
add_executable(Medical3 MACOSX_BUNDLE Medical3.cxx)
add_executable(Medical4 MACOSX_BUNDLE Medical4.cxx)

target_link_libraries(Medical1 PRIVATE ${SVTK_LIBRARIES})
target_link_libraries(Medical2 PRIVATE ${SVTK_LIBRARIES})
target_link_libraries(Medical3 PRIVATE ${SVTK_LIBRARIES})
target_link_libraries(Medical4 PRIVATE ${SVTK_LIBRARIES})

add_executable(GenerateModelsFromLabels MACOSX_BUNDLE GenerateModelsFromLabels.cxx)
add_executable(GenerateCubesFromLabels MACOSX_BUNDLE  GenerateCubesFromLabels.cxx)
target_link_libraries(GenerateModelsFromLabels PRIVATE ${SVTK_LIBRARIES})
target_link_libraries(GenerateCubesFromLabels PRIVATE ${SVTK_LIBRARIES})

svtk_module_autoinit(
  TARGETS Medical1
          Medical2
          Medical3
          Medical4
          GenerateModelsFromLabels
          GenerateCubesFromLabels
  MODULES ${SVTK_LIBRARIES})

if (BUILD_TESTING)
  include(CTest)
  if (TARGET SVTK::TestingRendering)

    ######## Regression Testing ########
    set(ExternalData_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/ExternalData)

    list(APPEND ExternalData_URL_TEMPLATES
      # Data published by Girder
      "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"

      # Data published by developers using git-gitlab-push.
      "https://www.svtk.org/files/ExternalData/%(algo)/%(hash)"
    )
    include(ExternalData)

    ExternalData_Expand_Arguments(MedicalData _
      DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/headsq/,REGEX:.*})

    foreach(test Medical1 Medical2 Medical3 Medical4)
      ExternalData_add_test(MedicalData
        NAME    "MedicalExampleCxx-${test}"
        COMMAND ${test}
          ${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing/Data/headsq/quarter
          -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/Baseline/${test}.png}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()

    ExternalData_Add_Target(MedicalData)
  endif ()
endif ()
