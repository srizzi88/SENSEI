#--////////////////////////////////////////////////////////////////////////////
#--  Copyright (c) 2010-2019, Michael A. Jackson. BlueQuartz Software
#--  All rights reserved.
#--  BSD License: https://www.opensource.org/licenses/bsd-license.html
#--////////////////////////////////////////////////////////////////////////////
cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)

project(SimpleCocoaSVTK)

if(NOT SVTK_BINARY_DIR)
  find_package(SVTK)
  if(NOT SVTK_DIR)
    message(FATAL_ERROR "Please set SVTK_DIR.")
  endif()
  include(${SVTK_USE_FILE})
endif()

# This is needed in case we are building this project separate from the SVTK build
if (NOT EXECUTABLE_OUTPUT_PATH)
  set (EXECUTABLE_OUTPUT_PATH ${SimpleCocoaSVTK_BINARY_DIR})
endif()

# The source files - Note because the files have both C++ and Objective-C in them
# the file extension is .mm
set (SimpleCocoaSVTK_SRCS
  ${SimpleCocoaSVTK_SOURCE_DIR}/main.mm
  ${SimpleCocoaSVTK_SOURCE_DIR}/BasicSVTKView.mm
  ${SimpleCocoaSVTK_SOURCE_DIR}/CustomLayer.mm
  ${SimpleCocoaSVTK_SOURCE_DIR}/CustomView.mm
  ${SimpleCocoaSVTK_SOURCE_DIR}/MyDocument.mm
  ${SimpleCocoaSVTK_SOURCE_DIR}/MyWindowController.mm
)

# Build all Objective-C as ARC.
set_source_files_properties(main.mm
                            BasicSVTKView.mm
                            CustomLayer.mm
                            CustomView.mm
                            MyDocument.mm
                            MyWindowController.mm
                            PROPERTIES COMPILE_FLAGS "-fobjc-arc")

# The Headers
set (SimpleCocoaSVTK_HDRS
  ${SimpleCocoaSVTK_SOURCE_DIR}/BasicSVTKView.h
  ${SimpleCocoaSVTK_SOURCE_DIR}/CustomLayer.h
  ${SimpleCocoaSVTK_SOURCE_DIR}/CustomView.h
  ${SimpleCocoaSVTK_SOURCE_DIR}/MyDocument.h
  ${SimpleCocoaSVTK_SOURCE_DIR}/MyWindowController.h
)

# these are the OS X Interface Builder Files
set (SimpleCocoaSVTK_XIBS
  MainMenu
  MyWindow
)

# Set the OS X Bundle specific CMake variables which will be used to populate the plist for
# the application bundle
set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.svtk.SimpleCocoaSVTK")
set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})

# These variables are specific to our plist and are NOT standard CMake variables
set(MACOSX_BUNDLE_NSMAIN_NIB_FILE "MainMenu")
set(MACOSX_BUNDLE_NSPRINCIPAL_CLASS "NSApplication")

# Add our Executable
add_executable(SimpleCocoaSVTK MACOSX_BUNDLE ${SimpleCocoaSVTK_SRCS} ${SimpleCocoaSVTK_HDRS})

# Probably a better way to set the framework link libraries.
target_link_libraries(SimpleCocoaSVTK "-framework Cocoa -framework OpenGL"
                      svtkRenderingOpenGL2 svtkInteractionStyle svtkRenderingFreeType svtkFiltersSources svtkRenderingGL2PSOpenGL2 )

# Set a custom plist file for the app bundle
set_target_properties(SimpleCocoaSVTK PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${SimpleCocoaSVTK_SOURCE_DIR}/Info-CMake.plist)

# Make sure we can find the 'ibtool' program. If we can NOT find it we
# skip generation of this project
find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
  message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
endif()

# Make sure the 'Resources' Directory is correctly created before we build
add_custom_command (TARGET SimpleCocoaSVTK PRE_BUILD
                      COMMAND mkdir -p ${EXECUTABLE_OUTPUT_PATH}/\${CONFIGURATION}/SimpleCocoaSVTK.app/Contents/Resources)

# Compile the .xib files using the 'ibtool' program with the destination being the app package
foreach(xib ${SimpleCocoaSVTK_XIBS})
  add_custom_command (TARGET SimpleCocoaSVTK POST_BUILD
                      COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text --compile ${EXECUTABLE_OUTPUT_PATH}/\${CONFIGURATION}/SimpleCocoaSVTK.app/Contents/Resources/${xib}.nib ${SimpleCocoaSVTK_SOURCE_DIR}/${xib}.xib
                      COMMENT "Compiling ${SimpleCocoaSVTK_SOURCE_DIR}/${xib}.xib")

endforeach()
