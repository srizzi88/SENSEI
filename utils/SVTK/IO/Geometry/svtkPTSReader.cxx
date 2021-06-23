/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPTSReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPTSReader.h"

#include "svtkCellArray.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkUnsignedCharArray.h"
#include "svtksys/FStream.hxx"

svtkStandardNewMacro(svtkPTSReader);

//----------------------------------------------------------------------------
svtkPTSReader::svtkPTSReader()
  : FileName(nullptr)
  , OutputDataTypeIsDouble(false)
  , LimitReadToBounds(false)
  , LimitToMaxNumberOfPoints(false)
  , MaxNumberOfPoints(1000000)
{
  this->SetNumberOfInputPorts(0);
  this->ReadBounds[0] = this->ReadBounds[2] = this->ReadBounds[4] = SVTK_DOUBLE_MAX;
  this->ReadBounds[1] = this->ReadBounds[3] = this->ReadBounds[5] = SVTK_DOUBLE_MIN;

  this->CreateCells = true;
  this->IncludeColorAndLuminance = true;
}

//----------------------------------------------------------------------------
svtkPTSReader::~svtkPTSReader()
{
  if (this->FileName)
  {
    delete[] this->FileName;
    this->FileName = nullptr;
  }
}

//-----------------------------------------------------------------------------
// svtkSetStringMacro except we clear some variables if we update the value
void svtkPTSReader::SetFileName(const char* filename)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting FileName to " << filename);
  if (this->FileName == nullptr && filename == nullptr)
  {
    return;
  }
  if (this->FileName && filename && !strcmp(this->FileName, filename))
  {
    return;
  }
  delete[] this->FileName;
  if (filename)
  {
    size_t n = strlen(filename) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (filename);
    this->FileName = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    this->FileName = nullptr;
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkPTSReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    svtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkPTSReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "OutputDataType = " << (this->OutputDataTypeIsDouble ? "double" : "float")
     << "\n";

  os << indent << "CreateCells = " << (this->CreateCells ? "yes" : "no") << "\n";

  os << indent << "IncludeColorAndLuminance = " << (this->IncludeColorAndLuminance ? "yes" : "no")
     << "\n";

  if (this->LimitReadToBounds)
  {
    os << indent << "LimitReadToBounds = true\n";
    os << indent << "ReadBounds = [" << this->ReadBounds[0] << "," << this->ReadBounds[1] << ","
       << this->ReadBounds[2] << this->ReadBounds[3] << "," << this->ReadBounds[4] << ","
       << this->ReadBounds[5] << "]\n";
  }
  else
  {
    os << indent << "LimitReadToBounds = false\n";
  }

  if (this->LimitToMaxNumberOfPoints)
  {
    os << indent << "LimitToMaxNumberOfPoints = true\n";
    os << indent << "MaxNumberOfPoints" << MaxNumberOfPoints << "\n";
  }
  else
  {
    os << indent << "LimitToMaxNumberOfPoints = false\n";
  }
}

//-----------------------------------------------------------------------------
int svtkPTSReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // See if we can open in the file
  if (!this->FileName)
  {
    svtkErrorMacro(<< "FileName must be specified.");
    return 0;
  }

  // Open the new file.
  svtkDebugMacro(<< "Opening file " << this->FileName);
  svtksys::ifstream file(this->FileName, ios::in | ios::binary);
  if (!file || file.fail())
  {
    svtkErrorMacro(<< "Could not open file " << this->FileName);
    return 0;
  }

  this->UpdateProgress(0);

  // Determine the number of points to be read in which should be
  // a single int at the top of the file
  const unsigned int bufferSize = 2048;
  std::string buffer;
  char junk[bufferSize];
  svtkTypeInt32 numPts = -1, tempNumPts;
  for (numPts = -1; !file.eof();)
  {
    getline(file, buffer);
    // Scanf should match the integer part but not the string
    int numArgs = sscanf(buffer.c_str(), "%d%s", &tempNumPts, junk);
    if (numArgs == 1)
    {
      numPts = static_cast<svtkTypeInt32>(tempNumPts);
      break;
    }
    if (numArgs != -1)
    {
      // We have a file that doesn't have a number of points line
      // Instead we need to count the number of lines in the file
      // Remember we already read in the first line hence numPts starts
      // at 1
      for (numPts = 1; getline(file, buffer); ++numPts)
      {
        if (numPts % 1000000 == 0)
        {
          this->UpdateProgress(0.1);
          if (this->GetAbortExecute())
          {
            return 0;
          }
        }
      }
      file.clear();
      file.seekg(0);
      break;
    }
  }

  // Next determine the format the point info. Is it x y z,
  // x y z intensity or
  // or x y z intensity r g b?
  int numValuesPerLine;
  double irgb[4], pt[3];

  if (numPts == -1)
  {
    svtkErrorMacro(<< "Could not process file " << this->FileName << " - Unknown Format");
    return 0;
  }
  else if (numPts == 0)
  {
    // Trivial case of no points - lets set it to 3
    svtkErrorMacro(<< "Could not process file " << this->FileName << " - No points specified");
    return 0;
  }
  else
  {
    getline(file, buffer);
    numValuesPerLine = sscanf(buffer.c_str(), "%lf %lf %lf %lf %lf %lf %lf", pt, pt + 1, pt + 2,
      irgb, irgb + 1, irgb + 2, irgb + 3);
  }
  if (!((numValuesPerLine == 3) || (numValuesPerLine == 4) || (numValuesPerLine == 6) ||
        (numValuesPerLine == 7)))
  {
    // Unsupported line format!
    svtkErrorMacro(<< "Invalid Pts Format (point info has " << numValuesPerLine
                  << ") in the file:" << this->FileName);
    return 0;
  }

  // Lets setup the SVTK Arrays and Points
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // If we are trying to limit the max number of points calculate the
  // onRatio - else set it to 1
  double onRatio = 1.0;
  svtkTypeInt32 targetNumPts = numPts;
  if (this->LimitToMaxNumberOfPoints)
  {
    onRatio = static_cast<double>(this->MaxNumberOfPoints) / numPts;
    targetNumPts = numPts * onRatio + 1;
  }

  svtkNew<svtkPoints> newPts;
  if (this->OutputDataTypeIsDouble)
  {
    newPts->SetDataTypeToDouble();
  }
  else
  {
    newPts->SetDataTypeToFloat();
  }
  newPts->Allocate(targetNumPts);

  svtkNew<svtkUnsignedCharArray> colors;
  svtkNew<svtkFloatArray> intensities;
  output->SetPoints(newPts);

  svtkNew<svtkCellArray> newVerts;
  if (this->CreateCells)
  {
    output->SetVerts(newVerts);
  }

  bool wantIntensities = ((numValuesPerLine == 4) || (numValuesPerLine == 7));
  if (numValuesPerLine > 4)
  {
    colors->SetNumberOfComponents(3);
    colors->SetName("Color");
    colors->Allocate(targetNumPts * 3);
    output->GetPointData()->SetScalars(colors);
    if (!this->IncludeColorAndLuminance)
    {
      wantIntensities = false;
    }
  }

  if (wantIntensities)
  {
    intensities->SetName("Intensities");
    intensities->SetNumberOfComponents(1);
    intensities->Allocate(targetNumPts);
    output->GetPointData()->AddArray(intensities);
  }

  if (numPts == 0)
  {
    // we are done
    return 1;
  }

  this->UpdateProgress(0.2);
  if (this->GetAbortExecute())
  {
    this->UpdateProgress(1.0);
    return 1;
  }

  // setup the ReadBBox, IF we're limiting the read to specified ReadBounds
  if (this->LimitReadToBounds)
  {
    this->ReadBBox.Reset();
    this->ReadBBox.SetMinPoint(this->ReadBounds[0], this->ReadBounds[2], this->ReadBounds[4]);
    this->ReadBBox.SetMaxPoint(this->ReadBounds[1], this->ReadBounds[3], this->ReadBounds[5]);
    // the ReadBBox is guaranteed to be "valid", regardless of the whether
    // ReadBounds is valid.  If any of the MonPoint values are greater than
    // the corresponding MaxPoint, the MinPoint component will be set to be
    // the same as the MaxPoint during the SetMaxPoint fn call.
  }

  // Lets Process the points!  Remember that we have already loaded in
  // the first line of points in the buffer
  svtkIdType* pids = nullptr;
  svtkIdType pid;
  if (this->CreateCells)
  {
    pids = new svtkIdType[targetNumPts];
  }
  long lastCount = 0;
  for (long i = 0; i < numPts; i++)
  {
    // Should we process this point?  Meaning that we skipped the appropriate number of points
    // based on the Max Number of points (onRatio) or the filtering by the read bounding box
    // OK to process based on Max Number of Points
    if (floor(i * onRatio) > lastCount)
    {
      lastCount++;
      sscanf(buffer.c_str(), "%lf %lf %lf %lf %lf %lf %lf", pt, pt + 1, pt + 2, irgb, irgb + 1,
        irgb + 2, irgb + 3);
      // OK to process based on bounding box
      if ((!this->LimitReadToBounds) || this->ReadBBox.ContainsPoint(pt))
      {
        pid = newPts->InsertNextPoint(pt);
        // std::cerr << "Point " << i << " : " << pt[0] << " " << pt[1] << " " << pt[2] << "\n";
        if (this->CreateCells)
        {
          pids[pid] = pid;
        }
        if (wantIntensities)
        {
          intensities->InsertNextValue(irgb[0]);
        }
        if (numValuesPerLine > 4)
        {
          // if we have intensity then the color info starts with the second value in the array
          // else it starts with the first
          if (wantIntensities)
          {
            colors->InsertNextTuple(irgb + 1);
          }
          else
          {
            colors->InsertNextTuple(irgb);
          }
        }
      }
    }
    if (file.eof())
    {
      break;
    }
    if (i % 1000000 == 0)
    {
      this->UpdateProgress(0.2 + (0.75 * i) / numPts);
      if (this->GetAbortExecute())
      {
        return 0;
      }
    }
    getline(file, buffer);
  }

  // Do we have to squeeze any of the arrays?
  if (newPts->GetNumberOfPoints() < targetNumPts)
  {
    newPts->Squeeze();
    if (wantIntensities)
    {
      intensities->Squeeze();
    }
    if (numValuesPerLine > 4)
    {
      colors->Squeeze();
    }
  }

  if (this->CreateCells)
  {
    newVerts->InsertNextCell(newPts->GetNumberOfPoints(), pids);
    delete[] pids;
  }

  this->UpdateProgress(1.0);
  return 1;
}
