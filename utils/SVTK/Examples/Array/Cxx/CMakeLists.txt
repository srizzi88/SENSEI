PROJECT(Array)

IF(NOT SVTK_BINARY_DIR)
FIND_PACKAGE(SVTK REQUIRED)
INCLUDE(${SVTK_USE_FILE})
ENDIF()

IF(NOT SVTK_USE_N_WAY_ARRAYS)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires SVTK_USE_N_WAY_ARRAYS.")
ENDIF()

ADD_EXECUTABLE(ArrayBasics ArrayBasics.cxx)
TARGET_LINK_LIBRARIES(ArrayBasics svtkCommon)

ADD_EXECUTABLE(IdentityMatrix IdentityMatrix.cxx)
TARGET_LINK_LIBRARIES(IdentityMatrix svtkCommon)

ADD_EXECUTABLE(ArrayIteration ArrayIteration.cxx)
TARGET_LINK_LIBRARIES(ArrayIteration svtkCommon)

IF(svtkViewsInfovis_LOADED AND SVTK_USE_INFOVIS)
  ADD_EXECUTABLE(AdjacencyMatrix AdjacencyMatrix.cxx)
  TARGET_LINK_LIBRARIES(AdjacencyMatrix svtkInfovis svtkViews)
ENDIF()
