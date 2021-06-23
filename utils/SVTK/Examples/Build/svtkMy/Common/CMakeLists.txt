#
# Source files
#
# Here is where you can add the name of your local common classes.
#
set(classes
  svtkBar
)

#
# Create the svtkmyCommon C++ library.
#
svtk_module_add_module(SVTKMY::Common
  CLASSES ${classes})
