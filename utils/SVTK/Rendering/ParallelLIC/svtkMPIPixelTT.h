/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPIPixelTT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkMPIPixelTT_h
#define svtkMPIPixelTT_h

#include "svtkMPI.h"
#include "svtkType.h" // for svtk types

// Description:
// Traits class for converting from svtk data type enum
// to the appropriate C or MPI datatype.
template <typename T>
class svtkMPIPixelTT;

#define svtkMPIPixelTTMacro1(_ctype)                                                                \
  template <>                                                                                      \
  class svtkMPIPixelTT<_ctype>                                                                      \
  {                                                                                                \
  public:                                                                                          \
    static MPI_Datatype MPIType;                                                                   \
    static int SVTKType;                                                                            \
  }

svtkMPIPixelTTMacro1(void);
svtkMPIPixelTTMacro1(char);
svtkMPIPixelTTMacro1(signed char);
svtkMPIPixelTTMacro1(unsigned char);
svtkMPIPixelTTMacro1(short);
svtkMPIPixelTTMacro1(unsigned short);
svtkMPIPixelTTMacro1(int);
svtkMPIPixelTTMacro1(unsigned int);
svtkMPIPixelTTMacro1(long);
svtkMPIPixelTTMacro1(unsigned long);
svtkMPIPixelTTMacro1(float);
svtkMPIPixelTTMacro1(double);
// svtkMPIPixelTTMacro1(svtkIdType);
svtkMPIPixelTTMacro1(long long);
svtkMPIPixelTTMacro1(unsigned long long);

#endif
// SVTK-HeaderTest-Exclude: svtkMPIPixelTT.h
