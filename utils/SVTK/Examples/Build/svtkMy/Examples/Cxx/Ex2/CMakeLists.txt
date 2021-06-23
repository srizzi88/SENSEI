#
# Add the executable
#

add_executable(svtkmyEx2 svtkmyEx2.cxx)
target_link_libraries(svtkmyEx2
  SVTKMY::Unsorted
  SVTK::FiltersSources SVTK::RenderingCore SVTK::RenderingOpenGL2)

#
# Because SVTK rendering uses factory classes, it is necessary to
# declare that we want to use classes in RenderingOpenGL2 to
# implement the factory classes in RenderingCore
#

svtk_module_autoinit(
  TARGETS svtkmyEx2
  MODULES SVTK::RenderingCore SVTK::RenderingOpenGL2)
