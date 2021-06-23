/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSegYTraceReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSegYTraceReader.h"
#include "svtkSegYIOUtils.h"

#include <iostream>

//-----------------------------------------------------------------------------
svtkSegYTraceReader::svtkSegYTraceReader()
{
  this->XCoordinate = 72;
  this->YCoordinate = 76;
}

//-----------------------------------------------------------------------------
void svtkSegYTraceReader::SetXYCoordBytePositions(int x, int y)
{
  this->XCoordinate = x;
  this->YCoordinate = y;
}

//-----------------------------------------------------------------------------
void svtkSegYTraceReader::PrintTraceHeader(std::istream& in, int startPos)
{
  int traceSequenceNumberInLine =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + traceHeaderBytesPos.TraceNumber, in);
  std::cout << "Trace sequence number in line : " << traceSequenceNumberInLine << std::endl;

  int traceSequenceNumberInFile = svtkSegYIOUtils::Instance()->readLongInteger(in);
  std::cout << "Trace sequence number in file : " << traceSequenceNumberInFile << std::endl;

  // Get number_of_samples from trace header position 115-116
  int numSamples =
    svtkSegYIOUtils::Instance()->readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
  std::cout << "number of samples: " << numSamples << std::endl;

  short sampleInterval =
    svtkSegYIOUtils::Instance()->readShortInteger(startPos + traceHeaderBytesPos.SampleInterval, in);
  std::cout << "sample interval: " << sampleInterval << std::endl;

  // Get inline number from trace header position 189-192
  int inlineNum =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
  std::cout << "Field record number (inline number) : " << inlineNum << std::endl;

  int crosslineNum =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
  std::cout << "cross-line number (ensemble number) : " << crosslineNum << std::endl;

  int traceNumberWithinEnsemble = svtkSegYIOUtils::Instance()->readLongInteger(
    startPos + traceHeaderBytesPos.TraceNumberWithinEnsemble, in);
  std::cout << "trace number within ensemble : " << traceNumberWithinEnsemble << std::endl;

  short coordinateMultiplier = svtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.CoordinateMultiplier, in);
  std::cout << "coordinate multiplier : " << coordinateMultiplier << std::endl;

  int xCoordinate = svtkSegYIOUtils::Instance()->readLongInteger(startPos + this->XCoordinate, in);
  std::cout << "X coordinate for ensemble position of the trace : " << xCoordinate << std::endl;

  int yCoordinate = svtkSegYIOUtils::Instance()->readLongInteger(startPos + this->YCoordinate, in);
  std::cout << "Y coordinate for ensemble position of the trace : " << yCoordinate << std::endl;

  short coordinateUnits = svtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.CoordinateUnits, in);
  std::cout << "coordinateUnits: " << coordinateUnits << std::endl;
}

//-----------------------------------------------------------------------------
void svtkSegYTraceReader::ReadTrace(
  std::streamoff& startPos, std::istream& in, int formatCode, svtkSegYTrace* trace)
{
  trace->InlineNumber =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
  trace->CrosslineNumber =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
  int numSamples =
    svtkSegYIOUtils::Instance()->readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
  trace->CoordinateMultiplier = svtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.CoordinateMultiplier, in);
  trace->XCoordinate =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + this->XCoordinate, in);
  trace->YCoordinate =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + this->YCoordinate, in);
  trace->SampleInterval =
    svtkSegYIOUtils::Instance()->readShortInteger(startPos + traceHeaderBytesPos.SampleInterval, in);

  in.seekg(startPos + 240, in.beg);
  float value;
  switch (formatCode)
  {
    case 1:
      for (int i = 0; i < numSamples; i++)
      {
        value = svtkSegYIOUtils::Instance()->readIBMFloat(in);
        trace->Data.push_back(value);
      }
      break;
    case 3:
      for (int i = 0; i < numSamples; i++)
      {
        value = svtkSegYIOUtils::Instance()->readShortInteger(in);
        trace->Data.push_back(value);
      }
      break;
    case 5:
      for (int i = 0; i < numSamples; i++)
      {
        value = svtkSegYIOUtils::Instance()->readFloat(in);
        trace->Data.push_back(value);
      }
      break;
    case 8:
      for (int i = 0; i < numSamples; i++)
      {
        value = svtkSegYIOUtils::Instance()->readChar(in);
        trace->Data.push_back(value);
      }
      break;
    default:
      std::cerr << "Data sample format code " << formatCode << " not supported." << std::endl;
      value = 0;
  }

  startPos += 240 + this->GetTraceSize(numSamples, formatCode);
}

//-----------------------------------------------------------------------------
void svtkSegYTraceReader::ReadInlineCrossline(std::streamoff& startPos, std::istream& in,
  int formatCode, int* inlineNumber, int* crosslineNumber, int* xCoord, int* yCoord,
  short* coordMultiplier)
{
  *inlineNumber =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
  *crosslineNumber =
    svtkSegYIOUtils::Instance()->readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
  int numSamples =
    svtkSegYIOUtils::Instance()->readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);

  *xCoord = svtkSegYIOUtils::Instance()->readLongInteger(startPos + this->XCoordinate, in);
  *yCoord = svtkSegYIOUtils::Instance()->readLongInteger(startPos + this->YCoordinate, in);
  *coordMultiplier = svtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.CoordinateMultiplier, in);
  startPos += 240 + this->GetTraceSize(numSamples, formatCode);
}

//-----------------------------------------------------------------------------
int svtkSegYTraceReader::GetTraceSize(int numSamples, int formatCode)
{
  if (formatCode == 1 || formatCode == 2 || formatCode == 4 || formatCode == 5)
  {
    return 4 * numSamples;
  }
  if (formatCode == 3)
  {
    return 2 * numSamples;
  }
  if (formatCode == 8)
  {
    return numSamples;
  }
  std::cerr << "Unsupported data format code : " << formatCode << std::endl;
  return -1;
}
