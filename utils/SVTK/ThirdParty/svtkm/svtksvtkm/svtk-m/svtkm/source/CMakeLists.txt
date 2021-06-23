##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  Oscillator.h
  Source.h
  Tangle.h
  Wavelet.h
  )

set(device_sources
  Oscillator.cxx
  Source.cxx
  Tangle.cxx
  Wavelet.cxx
  )

svtkm_library(NAME svtkm_source
  DEVICE_SOURCES ${device_sources}
  HEADERS ${headers}
  )

target_link_libraries(svtkm_source PUBLIC svtkm_cont)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
