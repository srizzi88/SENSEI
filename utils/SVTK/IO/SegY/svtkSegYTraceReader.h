/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSegYTraceReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkSegYTraceReader_h
#define svtkSegYTraceReader_h

#include <fstream>
#include <vector>

#include "svtkSegYTraceHeaderBytesPositions.h"

/*
 * Single Seg-Y trace
 */
class svtkSegYTrace
{
public:
  int XCoordinate;
  int YCoordinate;
  short CoordinateMultiplier;
  std::vector<float> Data;
  int InlineNumber;
  int CrosslineNumber;
  short SampleInterval;
};

/*
 * Single Seg-Y trace reader
 */
class svtkSegYTraceReader
{
private:
  svtkSegYTraceHeaderBytesPositions traceHeaderBytesPos;

  int XCoordinate;
  int YCoordinate;

public:
  svtkSegYTraceReader();

  void SetXYCoordBytePositions(int x, int y);
  void PrintTraceHeader(std::istream& in, int startPos);
  void ReadTrace(std::streamoff& startPos, std::istream& in, int formatCode, svtkSegYTrace* trace);
  void ReadInlineCrossline(std::streamoff& startPos, std::istream& in, int formatCode,
    int* inlineNumber, int* crosslineNumber, int* xCoord, int* yCoord, short* coordMultiplier);

  int GetTraceSize(int numSamples, int formatCode);
};

#endif // svtkSegYTraceReader_h
// SVTK-HeaderTest-Exclude: svtkSegYTraceReader.h
