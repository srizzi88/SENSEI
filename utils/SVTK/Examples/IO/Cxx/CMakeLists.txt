cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(IO)

find_package(SVTK
  OPTIONAL_COMPONENTS
    IOExport
    IOImport
    TestingRendering)
find_package(SVTK
  COMPONENTS
    CommonCore
    IOGeometry
    IOLegacy
    IOXML
    InteractionStyle)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(DumpXMLFile MACOSX_BUNDLE
  DumpXMLFile.cxx)
target_link_libraries(DumpXMLFile
  PRIVATE
    ${SVTK_LIBRARIES})

add_executable(ParticleReader MACOSX_BUNDLE
  ParticleReader.cxx)
target_link_libraries(ParticleReader
  PRIVATE
    ${SVTK_LIBRARIES})

svtk_module_autoinit(
  TARGETS DumpXMLFile
          ParticleReader
  MODULES ${SVTK_LIBRARIES})

if (TARGET SVTK::IOExport AND TARGET SVTK::IOImport)
  add_executable(objtovtp
    objtovtp.cxx)
  target_link_libraries(objtovtp
    PRIVATE
      ${SVTK_LIBRARIES}
      SVTK::IOExport
      SVTK::IOImport)

  svtk_module_autoinit(
    TARGETS objtovtp
    MODULES ${SVTK_LIBRARIES}
            SVTK::IOExport
            SVTK::IOImport)
endif ()

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

    ExternalData_add_test(IOData
      NAME    "IOExampleCxx-TestDumpXMLFile"
      COMMAND DumpXMLFile
              DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/cow.vtp}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    ExternalData_add_test(IOData
      NAME    "IOExampleCxx-TestParticleReader"
      COMMAND ParticleReader
              DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/golf.csv}
              ${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary/TestParticleReader.vtp
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    ExternalData_Add_Target(IOData)
  endif ()
endif ()
