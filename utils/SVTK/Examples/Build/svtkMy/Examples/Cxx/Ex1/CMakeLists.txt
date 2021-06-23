#
# Add the executable
#

add_executable(svtkmyEx1 svtkmyEx1.cxx)
target_link_libraries(svtkmyEx1 SVTKMY::Unsorted SVTKMY::Common SVTKMY::Imaging)
