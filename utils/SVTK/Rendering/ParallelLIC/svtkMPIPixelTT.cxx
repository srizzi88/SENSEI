/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPIPixelTT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMPIPixelTT.h"

#define svtkMPIPixelTTMacro2(_ctype, _mpiEnum, _svtkEnum)                                            \
  MPI_Datatype svtkMPIPixelTT<_ctype>::MPIType = _mpiEnum;                                          \
  int svtkMPIPixelTT<_ctype>::SVTKType = _svtkEnum

svtkMPIPixelTTMacro2(void, MPI_BYTE, SVTK_VOID);
svtkMPIPixelTTMacro2(char, MPI_CHAR, SVTK_CHAR);
svtkMPIPixelTTMacro2(signed char, MPI_CHAR, SVTK_SIGNED_CHAR);
svtkMPIPixelTTMacro2(unsigned char, MPI_UNSIGNED_CHAR, SVTK_UNSIGNED_CHAR);
svtkMPIPixelTTMacro2(short, MPI_SHORT, SVTK_SHORT);
svtkMPIPixelTTMacro2(unsigned short, MPI_UNSIGNED_SHORT, SVTK_UNSIGNED_SHORT);
svtkMPIPixelTTMacro2(int, MPI_INT, SVTK_INT);
svtkMPIPixelTTMacro2(unsigned int, MPI_UNSIGNED, SVTK_UNSIGNED_INT);
svtkMPIPixelTTMacro2(long, MPI_LONG, SVTK_LONG);
svtkMPIPixelTTMacro2(unsigned long, MPI_UNSIGNED_LONG, SVTK_UNSIGNED_LONG);
svtkMPIPixelTTMacro2(float, MPI_FLOAT, SVTK_FLOAT);
svtkMPIPixelTTMacro2(double, MPI_DOUBLE, SVTK_DOUBLE);
// svtkMPIPixelTTMacro2(svtkIdType, MPI_LONG_LONG, SVTK_IDTYPE);
svtkMPIPixelTTMacro2(long long, MPI_LONG_LONG, SVTK_LONG_LONG);
svtkMPIPixelTTMacro2(unsigned long long, MPI_UNSIGNED_LONG_LONG, SVTK_UNSIGNED_LONG_LONG);
