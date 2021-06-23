cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project(SVTKStatExample)

if(NOT SVTK_BINARY_DIR)
  find_package(SVTK REQUIRED)
  include(${SVTK_USE_FILE})
endif()

link_directories(${SVTK_LIBRARY_DIRS})

# Add example use of svtkDescriptiveStatistics
add_executable(ExampleDescriptiveStatistics ExampleDescriptiveStatistics.cxx)
target_link_libraries(ExampleDescriptiveStatistics svtkCommonCore svtkCommonSystem svtkCommonDataModel svtkFiltersStatistics)

# Add example use of svtkKMeansStatistics
add_executable(ExampleKMeansStatistics ExampleKMeansStatistics.cxx)
target_link_libraries(ExampleKMeansStatistics svtkCommonCore svtkCommonSystem svtkCommonDataModel svtkFiltersStatistics)
