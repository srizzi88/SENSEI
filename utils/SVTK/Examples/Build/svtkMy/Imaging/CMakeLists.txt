#
# Source files
#
# Here is where you can add the name of your local imaging classes.
#
set(classes
  svtkImageFoo
)

#
# Create the svtkmyImaging C++ library.
#
svtk_module_add_module(SVTKMY::Imaging
  CLASSES ${classes})
