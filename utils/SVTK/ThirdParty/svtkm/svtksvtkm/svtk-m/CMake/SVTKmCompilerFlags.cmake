##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
   CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  set(SVTKM_COMPILER_IS_MSVC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  set(SVTKM_COMPILER_IS_PGI 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(SVTKM_COMPILER_IS_ICC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(SVTKM_COMPILER_IS_CLANG 1)
  set(SVTKM_COMPILER_IS_APPLECLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(SVTKM_COMPILER_IS_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(SVTKM_COMPILER_IS_GNU 1)
endif()

#-----------------------------------------------------------------------------
# svtkm_compiler_flags is used by all the svtkm targets and consumers of SVTK-m
# The flags on svtkm_compiler_flags are needed when using/building svtk-m
add_library(svtkm_compiler_flags INTERFACE)

# When building libraries/tests that are part of the SVTK-m repository
# inherit the properties from svtkm_vectorization_flags.
# The flags are intended only for SVTK-m itself and are not needed by consumers.
# We will export svtkm_vectorization_flags in general
# so consumer can either enable vectorization or use SVTK-m's build flags if
# they so desire
target_link_libraries(svtkm_compiler_flags
  INTERFACE $<BUILD_INTERFACE:svtkm_vectorization_flags>)

# setup that we need C++11 support
target_compile_features(svtkm_compiler_flags INTERFACE cxx_std_11)

# setup our static libraries so that a separate ELF section
# is generated for each function. This allows for the linker to
# remove unused sections. This allows for programs that use SVTK-m
# to have the smallest binary impact as they can drop any SVTK-m symbol
# they don't use.
if(SVTKM_COMPILER_IS_MSVC)
  target_compile_options(svtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/Gy>)
  if(TARGET svtkm::cuda)
    target_compile_options(svtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler="/Gy">)
  endif()
elseif(NOT SVTKM_COMPILER_IS_PGI) #can't find an equivalant PGI flag
  target_compile_options(svtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>)
  if(TARGET svtkm::cuda)
    target_compile_options(svtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-ffunction-sections>)
  endif()
endif()

# Enable large object support so we can have 2^32 addressable sections
if(SVTKM_COMPILER_IS_MSVC)
  target_compile_options(svtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/bigobj>)
  if(TARGET svtkm::cuda)
    target_compile_options(svtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler="/bigobj">)
  endif()
endif()

# Setup the include directories that are needed for svtkm
target_include_directories(svtkm_compiler_flags INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${SVTKm_INSTALL_INCLUDE_DIR}>
  )

#-----------------------------------------------------------------------------
# svtkm_developer_flags is used ONLY BY libraries that are built as part of this
# repository
add_library(svtkm_developer_flags INTERFACE)

# Additional warnings just for Clang 3.5+, and AppleClang 7+
# about failures to vectorize.
if (SVTKM_COMPILER_IS_CLANG AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.4)
  target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Wno-pass-failed>)
elseif(SVTKM_COMPILER_IS_APPLECLANG AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.99)
  target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Wno-pass-failed>)
endif()

if(SVTKM_COMPILER_IS_MSVC)
  target_compile_definitions(svtkm_developer_flags INTERFACE "_SCL_SECURE_NO_WARNINGS"
                                                            "_CRT_SECURE_NO_WARNINGS")

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
    set(cxx_flags "-W3")
    set(cuda_flags "-Xcompiler=-W3")
  endif()
  list(APPEND cxx_flags -wd4702 -wd4505)
  list(APPEND cuda_flags "-Xcompiler=-wd4702,-wd4505")

  #Setup MSVC warnings with CUDA and CXX
  target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>)
  if(TARGET svtkm::cuda)
    target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${cuda_flags}  -Xcudafe=--diag_suppress=1394,--diag_suppress=766>)
  endif()

  if(MSVC_VERSION LESS 1900)
    # In VS2013 the C4127 warning has a bug in the implementation and
    # generates false positive warnings for lots of template code
    target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-wd4127>)
    if(TARGET svtkm::cuda)
      target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-wd4127>)
    endif()
  endif()

elseif(SVTKM_COMPILER_IS_ICC)
  #Intel compiler offers header level suppression in the form of
  # #pragma warning(disable : 1478), but for warning 1478 it seems to not
  #work. Instead we add it as a definition
  # Likewise to suppress failures about being unable to apply vectorization
  # to loops, the #pragma warning(disable seems to not work so we add a
  # a compile define.
  target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-wd1478 -wd13379>)

elseif(SVTKM_COMPILER_IS_GNU OR SVTKM_COMPILER_IS_CLANG)
  set(cxx_flags -Wall -Wcast-align -Wchar-subscripts -Wextra -Wpointer-arith -Wformat -Wformat-security -Wshadow -Wunused -fno-common)
  set(cuda_flags -Xcompiler=-Wall,-Wno-unknown-pragmas,-Wno-unused-local-typedefs,-Wno-unused-local-typedefs,-Wno-unused-function,-Wcast-align,-Wchar-subscripts,-Wpointer-arith,-Wformat,-Wformat-security,-Wshadow,-Wunused,-fno-common)

  #Only add float-conversion warnings for gcc as the integer warnigns in GCC
  #include the implicit casting of all types smaller than int to ints.
  if (SVTKM_COMPILER_IS_GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.99)
    list(APPEND cxx_flags -Wfloat-conversion)
    set(cuda_flags "${cuda_flags},-Wfloat-conversion")
  elseif (SVTKM_COMPILER_IS_CLANG)
    list(APPEND cxx_flags -Wconversion)
    set(cuda_flags "${cuda_flags},-Wconversion")
  endif()

  #Add in the -Wodr warning for GCC versions 5.2+
  if (SVTKM_COMPILER_IS_GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.1)
    list(APPEND cxx_flags -Wodr)
    set(cuda_flags "${cuda_flags},-Wodr")
  elseif (SVTKM_COMPILER_IS_CLANG)
    list(APPEND cxx_flags -Wodr)
    set(cuda_flags "${cuda_flags},-Wodr")
  endif()

  #GCC 5, 6 don't properly handle strict-overflow suppression through pragma's.
  #Instead of suppressing around the location of the strict-overflow you
  #have to suppress around the entry point, or in svtk-m case the worklet
  #invocation site. This is incredibly tedious and has been fixed in gcc 7
  #
  if(SVTKM_COMPILER_IS_GNU AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.99) AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.99) )
    list(APPEND cxx_flags -Wno-strict-overflow)
  endif()

  target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>)
  if(TARGET svtkm::cuda)
    target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${cuda_flags}>)
  endif()
endif()

#common warnings for all platforms when building cuda
if(TARGET svtkm::cuda)
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
    #nvcc 9 introduced specific controls to disable the stack size warning
    #otherwise we let the warning occur. We have to set this in CMAKE_CUDA_FLAGS
    #as it is passed to the device link step, unlike compile_options
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xnvlink=--suppress-stack-size-warning")
  endif()

  set(display_error_nums -Xcudafe=--display_error_number)
  target_compile_options(svtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${display_error_nums}>)
endif()

if(NOT SVTKm_INSTALL_ONLY_LIBRARIES)
  install(TARGETS svtkm_compiler_flags svtkm_developer_flags EXPORT ${SVTKm_EXPORT_NAME})
endif()
