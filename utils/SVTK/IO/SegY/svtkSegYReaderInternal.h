/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSegYReaderInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkSegYReaderInternal_h
#define svtkSegYReaderInternal_h
#ifndef __SVTK_WRAP__

#include <fstream>
#include <string>
#include <vector>
#include <svtksys/FStream.hxx>

// Forward declarations
class svtkStructuredGrid;
class svtkImageData;
class svtkSegYTraceReader;
class svtkSegYTrace;
class svtkSegYBinaryHeaderBytesPositions;

class svtkSegYReaderInternal
{
public:
  svtkSegYReaderInternal();
  svtkSegYReaderInternal(const svtkSegYReaderInternal& other) = delete;
  svtkSegYReaderInternal& operator=(const svtkSegYReaderInternal& other) = delete;
  ~svtkSegYReaderInternal();

public:
  bool Is3DComputeParameters(
    int* extent, double origin[3], double spacing[3][3], int* spacingSign, bool force2D);
  void LoadTraces(int* extent);

  void ExportData(
    svtkImageData*, int* extent, double origin[3], double spacing[3][3], int* spacingSign);
  void ExportData(svtkStructuredGrid*, int* extent, double origin[3], double spacing[3][3]);

  void SetXYCoordBytePositions(int x, int y);
  void SetVerticalCRS(int);

  svtksys::ifstream In;

protected:
  bool ReadHeader();

private:
  std::vector<svtkSegYTrace*> Traces;
  svtkSegYBinaryHeaderBytesPositions* BinaryHeaderBytesPos;
  svtkSegYTraceReader* TraceReader;
  int VerticalCRS;
  // Binary Header
  short SampleInterval;
  int FormatCode;
  int SampleCountPerTrace;
};

#endif
#endif // svtkSegYReaderInternal_h
// SVTK-HeaderTest-Exclude: svtkSegYReaderInternal.h
