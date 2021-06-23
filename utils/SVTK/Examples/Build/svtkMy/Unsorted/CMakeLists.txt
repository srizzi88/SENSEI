#
# Source files
#
# Here is where you can add the name of your local unsorted classes.
#
set(classes
  svtkBar2
)

#
# Create the svtkmyUnsorted C++ library.
#
svtk_module_add_module(SVTKMY::Unsorted
  CLASSES ${classes})
