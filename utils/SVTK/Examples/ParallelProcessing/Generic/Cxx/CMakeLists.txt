cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(ParallelProcessingGeneric)

find_package(SVTK
  COMPONENTS
    IOImage
    ImagingCore
    ImagingGeneral
    FiltersSources
    ParallelMPI
    RenderingCore
    RenderingParallel
    TestingCore
    TestingRendering
    mpi)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(ParallelIso ParallelIso.cxx)
target_link_libraries(ParallelIso
  PRIVATE
    ${SVTK_LIBRARIES})

set(TaskPara_SRCS
  TaskParallelism.cxx
  task1.cxx
  task2.cxx)

add_executable(TaskParallelism
  ${TaskPara_SRCS})
target_link_libraries(TaskParallelism
  PRIVATE
    ${SVTK_LIBRARIES})

svtk_module_autoinit(
  TARGETS ParallelIso
          TaskParallelism
  MODULES ${SVTK_LIBRARIES})
