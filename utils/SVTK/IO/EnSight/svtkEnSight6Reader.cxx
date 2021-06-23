/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnSight6Reader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEnSight6Reader.h"

#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnstructuredGrid.h"
#include "svtksys/FStream.hxx"

#include <cassert>
#include <cctype>
#include <string>

svtkStandardNewMacro(svtkEnSight6Reader);

//----------------------------------------------------------------------------
svtkEnSight6Reader::svtkEnSight6Reader()
{
  this->NumberOfUnstructuredPoints = 0;
  this->UnstructuredPoints = svtkPoints::New();
  this->UnstructuredNodeIds = nullptr;
}

//----------------------------------------------------------------------------
svtkEnSight6Reader::~svtkEnSight6Reader()
{
  if (this->UnstructuredNodeIds)
  {
    this->UnstructuredNodeIds->Delete();
    this->UnstructuredNodeIds = nullptr;
  }
  this->UnstructuredPoints->Delete();
  this->UnstructuredPoints = nullptr;
}

//----------------------------------------------------------------------------
static void svtkEnSight6ReaderRead1(
  const char* line, const char*, int* pointId, float* point1, float* point2, float* point3)
{
#ifdef __CYGWIN__
  // most cygwins are busted in sscanf, this is a work around
  int numEntries = 0;
  char* dup = strdup(line);
  dup[8] = '\0';
  numEntries += sscanf(dup, "%8d", pointId);
  dup[8] = line[8];
  dup[20] = '\0';
  numEntries += sscanf(dup + 8, "%12e", point1);
  dup[20] = line[20];
  dup[32] = '\0';
  numEntries += sscanf(dup + 20, "%12e", point2);
  dup[32] = line[32];
  numEntries += sscanf(dup + 32, "%12e", point3);
  free(dup);
#else
#ifndef NDEBUG
  int numEntries =
#endif
    sscanf(line, " %8d %12e %12e %12e", pointId, point1, point2, point3);
#endif
  assert("post: all_items_match" && numEntries == 4);
}

static void svtkEnSight6ReaderRead2(
  const char* line, const char*, float* point1, float* point2, float* point3)
{
#ifdef __CYGWIN__
  // most cygwins are busted in sscanf, this is a work around
  int numEntries = 0;
  char* dup = strdup(line);
  dup[12] = '\0';
  numEntries += sscanf(dup, "%12e", point1);
  dup[12] = line[12];
  dup[24] = '\0';
  numEntries += sscanf(dup + 12, "%12e", point2);
  dup[24] = line[24];
  numEntries += sscanf(dup + 24, "%12e", point3);
  free(dup);
#else

#ifndef NDEBUG
  int numEntries =
#endif
    sscanf(line, " %12e %12e %12e", point1, point2, point3);
#endif
  assert("post: all_items_match" && numEntries == 3);
}

static void svtkEnSight6ReaderRead3(const char* line, const char*, float* point1, float* point2,
  float* point3, float* point4, float* point5, float* point6)
{
#ifdef __CYGWIN__
  // most cygwins are busted in sscanf, this is a work around
  int numEntries = 0;
  char* dup = strdup(line);
  dup[12] = '\0';
  numEntries += sscanf(dup, "%12e", point1);
  dup[12] = line[12];
  dup[24] = '\0';
  numEntries += sscanf(dup + 12, "%12e", point2);
  dup[24] = line[24];
  dup[36] = '\0';
  numEntries += sscanf(dup + 24, "%12e", point3);
  dup[36] = line[36];
  dup[48] = '\0';
  numEntries += sscanf(dup + 36, "%12e", point4);
  dup[48] = line[48];
  dup[60] = '\0';
  numEntries += sscanf(dup + 48, "%12e", point5);
  dup[60] = line[60];
  numEntries += sscanf(dup + 60, "%12e", point6);
  free(dup);
#else
#ifndef NDEBUG
  int numEntries =
#endif
    sscanf(line, " %12e %12e %12e %12e %12e %12e", point1, point2, point3, point4, point5, point6);
#endif
  assert("post: all_items_match" && numEntries == 6);
}

static void svtkEnSight6ReaderRead4(const char* line, float* point1)
{
#ifdef __CYGWIN__
  // most cygwins are busted in sscanf, this is a work around
  int numEntries = 0;
  char* dup = strdup(line);
  dup[12] = '\0';
  numEntries += sscanf(dup, "%12e", point1);
  free(dup);
#else

#ifndef NDEBUG
  int numEntries =
#endif
    sscanf(line, "%12e", point1);

#endif
  assert("post: all_items_match" && numEntries == 1);
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadGeometryFile(
  const char* fileName, int timeStep, svtkMultiBlockDataSet* output)
{
  char line[256], subLine[256];
  int partId;
  int lineRead;
  int pointId;
  float point[3];
  int i, j;
  int pointIdsListed;

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("A GeometryFileName must be specified in the case file.");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to geometry file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  this->ReadLine(line);

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strcmp(subLine, "Binary") == 0)
    {
      svtkErrorMacro("This is a binary data set. Try "
        << "svtkEnSight6BinaryReader.");
      return 0;
    }
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
      this->ReadLine(line);
    }

    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadNextDataLine(line);
      this->RemoveLeadingBlanks(line);
    }
    this->ReadLine(line);
  }

  // Skip description line.  Using ReadLine instead of
  // ReadNextDataLine because the description line could be blank.
  this->ReadLine(line);

  // Read the node id and element id lines.
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0)
  {
    this->UnstructuredNodeIds = svtkIdTypeArray::New();
    pointIdsListed = 1;
  }
  else if (strcmp(subLine, "ignore") == 0)
  {
    pointIdsListed = 1;
  }
  else
  {
    pointIdsListed = 0;
  }

  this->ReadNextDataLine(line);

  this->ReadNextDataLine(line); // "coordinates"
  this->ReadNextDataLine(line);
  this->NumberOfUnstructuredPoints = atoi(line);
  this->UnstructuredPoints->Allocate(this->NumberOfUnstructuredPoints);
  int* tmpIds = new int[this->NumberOfUnstructuredPoints];

  int maxId = 0;

  for (j = 0; j < this->NumberOfUnstructuredPoints; j++)
  {
    this->ReadNextDataLine(line);
    if (pointIdsListed)
    {
      // point ids listed
      svtkEnSight6ReaderRead1(
        line, " %8d %12e %12e %12e", &pointId, &point[0], &point[1], &point[2]);
      if (this->UnstructuredNodeIds)
      {
        tmpIds[j] = pointId;
        if (pointId > maxId)
        {
          maxId = pointId;
        }
      }
      this->UnstructuredPoints->InsertNextPoint(point);
    }
    else
    {
      svtkEnSight6ReaderRead2(line, "%12e%12e%12e", &point[0], &point[1], &point[2]);
      this->UnstructuredPoints->InsertNextPoint(point);
    }
  }

  if (this->UnstructuredNodeIds)
  {
    this->UnstructuredNodeIds->SetNumberOfComponents(1);
    this->UnstructuredNodeIds->SetNumberOfTuples(maxId);
    this->UnstructuredNodeIds->FillComponent(0, -1);

    for (j = 0; j < this->NumberOfUnstructuredPoints; j++)
    {
      this->UnstructuredNodeIds->InsertValue(tmpIds[j] - 1, j);
    }
  }
  delete[] tmpIds;

  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && sscanf(line, " part %d", &partId) == 1)
  {
    this->NumberOfGeometryParts++;
    partId--; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);

    this->ReadLine(line); // part description line
    char* name = strdup(line);
    this->ReadNextDataLine(line);
    this->RemoveLeadingBlanks(line);

    if (strncmp(line, "block", 5) == 0)
    {
      lineRead = this->CreateStructuredGridOutput(realId, line, name, output);
    }
    else
    {
      lineRead = this->CreateUnstructuredGridOutput(realId, line, name, output);
    }
    free(name);
  }

  delete this->IS;
  this->IS = nullptr;
  if (this->UnstructuredNodeIds)
  {
    this->UnstructuredNodeIds->Delete();
    this->UnstructuredNodeIds = nullptr;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadMeasuredGeometryFile(
  const char* fileName, int timeStep, svtkMultiBlockDataSet* output)
{
  char line[256], subLine[256];
  svtkPoints* newPoints;
  int i;
  svtkIdType id;
  int tempId;
  float coords[3];

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("A MeasuredFileName must be specified in the case file.");
    return 0;
  }

  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to measured geometry file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  this->ReadLine(line);

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strcmp(subLine, "Binary") == 0)
    {
      svtkErrorMacro("This is a binary data set. Try "
        << "svtkEnSight6BinaryReader.");
      return 0;
    }
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
      this->ReadLine(line);
    }

    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
    this->ReadLine(line);
  }

  this->ReadLine(line); // "particle coordinates"
  this->ReadLine(line);
  this->NumberOfMeasuredPoints = atoi(line);

  this->NumberOfNewOutputs++;

  if (this->GetDataSetFromBlock(output, this->NumberOfGeometryParts) == nullptr ||
    !this->GetDataSetFromBlock(output, this->NumberOfGeometryParts)->IsA("svtkPolyData"))
  {
    svtkDebugMacro("creating new measured geometry output");
    svtkPolyData* pd = svtkPolyData::New();
    this->AddToBlock(output, this->NumberOfGeometryParts, pd);
    pd->Delete();
  }

  svtkPolyData* pd =
    svtkPolyData::SafeDownCast(this->GetDataSetFromBlock(output, this->NumberOfGeometryParts));
  pd->AllocateEstimate(this->NumberOfMeasuredPoints, 1);

  newPoints = svtkPoints::New();
  newPoints->Allocate(this->NumberOfMeasuredPoints);

  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
  {
    this->ReadLine(line);
    svtkEnSight6ReaderRead1(
      line, " %8d %12e %12e %12e", &tempId, &coords[0], &coords[1], &coords[2]);
    id = this->ParticleCoordinatesByIndex ? i : tempId;
    newPoints->InsertNextPoint(coords);
    pd->InsertNextCell(SVTK_VERTEX, 1, &id);
  }

  pd->SetPoints(newPoints);

  newPoints->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadScalarsPerNode(const char* fileName, const char* description,
  int timeStep, svtkMultiBlockDataSet* compositeOutput, int measured, int numberOfComponents,
  int component)
{
  char line[256];
  char tempLine[256];
  int partId, numPts, i, j;
  svtkFloatArray* scalars;
  int numLines, moreScalars;
  float scalarsRead[6];
  svtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("nullptr ScalarPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to scalar per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
    }

    this->ReadLine(line);
    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
  }

  this->ReadLine(line); // skip the description line

  this->ReadNextDataLine(line); // 1st data line or part #
  this->RemoveLeadingBlanks(line);
  if (strncmp(line, "part", 4) != 0)
  {
    int allocatedScalars = 0;
    // There are 6 values per line, and one scalar per point.
    if (!measured)
    {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
    }
    else
    {
      numPts = static_cast<svtkDataSet*>(
        this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts))
                 ->GetNumberOfPoints();
    }
    numLines = numPts / 6;
    moreScalars = numPts % 6;
    if (component == 0)
    {
      scalars = svtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);
      allocatedScalars = 1;
    }
    else
    {
      // It does not matter which unstructured part we get the point data from
      // because it is the same for all of them.
      partId = this->UnstructuredPartIds->GetId(0);
      scalars = static_cast<svtkFloatArray*>(
        this->GetDataSetFromBlock(compositeOutput, partId)->GetPointData()->GetArray(description));
    }
    for (i = 0; i < numLines; i++)
    {
      svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
        &scalarsRead[1], &scalarsRead[2], &scalarsRead[3], &scalarsRead[4], &scalarsRead[5]);

      for (j = 0; j < 6; j++)
      {
        scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
      }
      this->ReadNextDataLine(line);
    }
    strcpy(tempLine, "");
    for (j = 0; j < moreScalars; j++)
    {
      svtkEnSight6ReaderRead4(line + j * 12, &scalarsRead[j]);
      scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
    }
    if (moreScalars != 0)
    {
      this->ReadLine(line);
    }
    if (!measured)
    {
      for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
        partId = this->UnstructuredPartIds->GetId(i);
        output = static_cast<svtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, partId));
        if (component == 0)
        {
          scalars->SetName(description);
          output->GetPointData()->AddArray(scalars);
          if (!output->GetPointData()->GetScalars())
          {
            output->GetPointData()->SetScalars(scalars);
          }
        }
        else
        {
          output->GetPointData()->AddArray(scalars);
        }
      }
    }
    else
    {
      scalars->SetName(description);
      output = static_cast<svtkDataSet*>(
        this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts));
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
      {
        output->GetPointData()->SetScalars(scalars);
      }
    }
    if (allocatedScalars)
    {
      scalars->Delete();
    }
  }

  this->RemoveLeadingBlanks(line);
  // scalars for structured parts
  while (strncmp(line, "part", 4) == 0)
  {
    int allocatedScalars = 0;
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);

    output = static_cast<svtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, realId));
    this->ReadNextDataLine(line); // block
    numPts = output->GetNumberOfPoints();
    numLines = numPts / 6;
    moreScalars = numPts % 6;
    if (component == 0)
    {
      scalars = svtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);
      allocatedScalars = 1;
    }
    else
    {
      scalars = (svtkFloatArray*)(output->GetPointData()->GetArray(description));
    }
    for (i = 0; i < numLines; i++)
    {
      this->ReadNextDataLine(line);
      svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
        &scalarsRead[1], &scalarsRead[2], &scalarsRead[3], &scalarsRead[4], &scalarsRead[5]);
      for (j = 0; j < 6; j++)
      {
        scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
      }
    }
    this->ReadNextDataLine(line);
    strcpy(tempLine, "");
    for (j = 0; j < moreScalars; j++)
    {
      svtkEnSight6ReaderRead4(line + j * 12, &scalarsRead[j]);
      scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
    }
    if (component == 0)
    {
      scalars->SetName(description);
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
      {
        output->GetPointData()->SetScalars(scalars);
      }
    }
    else
    {
      output->GetPointData()->AddArray(scalars);
    }
    this->ReadNextDataLine(line);
    if (allocatedScalars)
    {
      scalars->Delete();
    }
    this->RemoveLeadingBlanks(line);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadVectorsPerNode(const char* fileName, const char* description,
  int timeStep, svtkMultiBlockDataSet* compositeOutput, int measured)
{
  char line[256];
  char tempLine[256];
  int partId, numPts, i, j, k;
  svtkFloatArray* vectors;
  int numLines, moreVectors;
  float vector1[3], vector2[3], values[6];
  svtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("nullptr VectorPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to vector per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
    }

    this->ReadLine(line);
    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
  }

  this->ReadLine(line); // skip the description line

  this->ReadNextDataLine(line); // 1st data line or part #
  this->RemoveLeadingBlanks(line);
  if (strncmp(line, "part", 4) != 0)
  {
    // There are 6 values per line, and 3 values (or 1 vector) per point.
    if (!measured)
    {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
    }
    else
    {
      numPts = static_cast<svtkDataSet*>(
        this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts))
                 ->GetNumberOfPoints();
    }
    numLines = numPts / 2;
    moreVectors = ((numPts * 3) % 6) / 3;
    vectors = svtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts * 3);
    for (i = 0; i < numLines; i++)
    {
      svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &vector1[0], &vector1[1],
        &vector1[2], &vector2[0], &vector2[1], &vector2[2]);
      vectors->InsertTuple(i * 2, vector1);
      vectors->InsertTuple(i * 2 + 1, vector2);
      this->ReadNextDataLine(line);
    }
    strcpy(tempLine, "");
    for (j = 0; j < moreVectors; j++)
    {
      svtkEnSight6ReaderRead4(line + j * 36, &vector1[0]);
      svtkEnSight6ReaderRead4(line + j * 36 + 12, &vector1[1]);
      svtkEnSight6ReaderRead4(line + j * 36 + 24, &vector1[2]);
      vectors->InsertTuple(i * 2 + j, vector1);
    }
    if (moreVectors != 0)
    {
      this->ReadLine(line);
    }
    if (!measured)
    {
      for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
        partId = this->UnstructuredPartIds->GetId(i);
        vectors->SetName(description);
        output = static_cast<svtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, partId));
        output->GetPointData()->AddArray(vectors);
        if (!output->GetPointData()->GetVectors())
        {
          output->GetPointData()->SetVectors(vectors);
        }
      }
    }
    else
    {
      vectors->SetName(description);
      output = static_cast<svtkDataSet*>(
        this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts));
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(vectors);
      }
    }
    vectors->Delete();
  }

  // vectors for structured parts
  this->RemoveLeadingBlanks(line);
  while (strncmp(line, "part", 4) == 0)
  {
    sscanf(line, " part %d", &partId);
    partId--;
    int realId = this->InsertNewPartId(partId);

    output = static_cast<svtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, realId));
    numPts = output->GetNumberOfPoints();
    numLines = numPts / 6;
    moreVectors = numPts % 6;
    vectors = svtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts * 3);

    for (k = 0; k < 3; k++)
    {
      for (i = 0; i < numLines; i++)
      {
        this->ReadNextDataLine(line);
        svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &values[0], &values[1],
          &values[2], &values[3], &values[4], &values[5]);
        for (j = 0; j < 6; j++)
        {
          vectors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }

      if (moreVectors)
      {
        this->ReadNextDataLine(line);
        strcpy(tempLine, "");
        for (j = 0; j < moreVectors; j++)
        {
          svtkEnSight6ReaderRead4(line + j * 12, &values[j]);
          vectors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }
    }
    vectors->SetName(description);
    output->GetPointData()->AddArray(vectors);
    if (!output->GetPointData()->GetVectors())
    {
      output->GetPointData()->SetVectors(vectors);
    }
    vectors->Delete();

    this->ReadNextDataLine(line);
    this->RemoveLeadingBlanks(line);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadTensorsPerNode(const char* fileName, const char* description,
  int timeStep, svtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  char tempLine[256];
  int partId, numPts, i, j, k;
  svtkFloatArray* tensors;
  int numLines, moreTensors;
  float tensor[6], values[6];
  int lineRead;
  svtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("nullptr TensorSymmPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to tensor symm per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
    }

    this->ReadLine(line);
    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
  }

  this->ReadLine(line); // skip the description line

  lineRead = this->ReadNextDataLine(line); // 1st data line or part #
  this->RemoveLeadingBlanks(line);
  if (strncmp(line, "part", 4) != 0)
  {
    // There are 6 values per line, and 6 values (or 1 tensor) per point.
    numPts = this->UnstructuredPoints->GetNumberOfPoints();
    numLines = numPts;
    tensors = svtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts * 6);
    for (i = 0; i < numLines; i++)
    {
      svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &tensor[0], &tensor[1],
        &tensor[2], &tensor[3], &tensor[5], &tensor[4]);
      tensors->InsertTuple(i, tensor);
      lineRead = this->ReadNextDataLine(line);
    }

    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
    {
      partId = this->UnstructuredPartIds->GetId(i);
      tensors->SetName(description);
      this->GetDataSetFromBlock(compositeOutput, partId)->GetPointData()->AddArray(tensors);
    }
    tensors->Delete();
  }

  // vectors for structured parts
  this->RemoveLeadingBlanks(line);
  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    sscanf(line, " part %d", &partId);
    partId--;
    int realId = this->InsertNewPartId(partId);
    this->ReadNextDataLine(line); // block
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    numLines = numPts / 6;
    moreTensors = numPts % 6;
    tensors = svtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts * 6);

    for (k = 0; k < 6; k++)
    {
      for (i = 0; i < numLines; i++)
      {
        this->ReadNextDataLine(line);
        svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &values[0], &values[1],
          &values[2], &values[3], &values[5], &values[4]);
        for (j = 0; j < 6; j++)
        {
          tensors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }

      if (moreTensors)
      {
        this->ReadNextDataLine(line);
        strcpy(tempLine, "");
        for (j = 0; j < moreTensors; j++)
        {
          svtkEnSight6ReaderRead4(line + j * 12, &values[j]);
          tensors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }
    }
    tensors->SetName(description);
    output->GetPointData()->AddArray(tensors);
    tensors->Delete();
    lineRead = this->ReadNextDataLine(line);
    this->RemoveLeadingBlanks(line);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadScalarsPerElement(const char* fileName, const char* description,
  int timeStep, svtkMultiBlockDataSet* compositeOutput, int numberOfComponents, int component)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  svtkFloatArray* scalars;
  int lineRead, elementType;
  float scalarsRead[6];
  int numLines, moreScalars;
  svtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("nullptr ScalarPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to scalars per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);                    // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    int realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    if (component == 0)
    {
      scalars = svtkFloatArray::New();
      scalars->SetNumberOfTuples(numCells);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numCells * numberOfComponents);
    }
    else
    {
      scalars = (svtkFloatArray*)(output->GetCellData()->GetArray(description));
    }

    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
    {
      numLines = numCells / 6;
      moreScalars = numCells % 6;
      for (i = 0; i < numLines; i++)
      {
        this->ReadNextDataLine(line);
        svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
          &scalarsRead[1], &scalarsRead[2], &scalarsRead[3], &scalarsRead[4], &scalarsRead[5]);
        for (j = 0; j < 6; j++)
        {
          scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
        }
      }
      lineRead = this->ReadNextDataLine(line);

      if (moreScalars)
      {
        for (j = 0; j < moreScalars; j++)
        {
          svtkEnSight6ReaderRead4(line + j * 12, &scalarsRead[j]);
          scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
        }
      }
    }
    else
    {
      while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
      {
        elementType = this->GetElementType(line);
        if (elementType < 0)
        {
          svtkErrorMacro("invalid element type");
          delete this->IS;
          this->IS = nullptr;
          return 0;
        }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        numLines = numCellsPerElement / 6;
        moreScalars = numCellsPerElement % 6;
        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
            &scalarsRead[1], &scalarsRead[2], &scalarsRead[3], &scalarsRead[4], &scalarsRead[5]);
          for (j = 0; j < 6; j++)
          {
            scalars->InsertComponent(
              this->GetCellIds(idx, elementType)->GetId(i * 6 + j), component, scalarsRead[j]);
          }
        }
        if (moreScalars)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreScalars; j++)
          {
            svtkEnSight6ReaderRead4(line + j * 12, &scalarsRead[j]);
            scalars->InsertComponent(
              this->GetCellIds(idx, elementType)->GetId(i * 6 + j), component, scalarsRead[j]);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      } // end while
    }   // end else
    if (component == 0)
    {
      scalars->SetName(description);
      output->GetCellData()->AddArray(scalars);
      if (!output->GetCellData()->GetScalars())
      {
        output->GetCellData()->SetScalars(scalars);
      }
      scalars->Delete();
    }
    else
    {
      output->GetCellData()->AddArray(scalars);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadVectorsPerElement(const char* fileName, const char* description,
  int timeStep, svtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  svtkFloatArray* vectors;
  int lineRead, elementType;
  float values[6], vector1[3], vector2[3];
  int numLines, moreVectors;
  svtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("nullptr VectorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to vector per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);                    // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    vectors = svtkFloatArray::New();
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    int realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    vectors->SetNumberOfTuples(numCells);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numCells * 3);

    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
    {
      numLines = numCells / 6;
      moreVectors = numCells % 6;

      for (k = 0; k < 3; k++)
      {
        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &values[0], &values[1],
            &values[2], &values[3], &values[4], &values[5]);
          for (j = 0; j < 6; j++)
          {
            vectors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
        if (moreVectors)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreVectors; j++)
          {
            svtkEnSight6ReaderRead4(line + j * 12, &values[j]);
            vectors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
      }
      lineRead = this->ReadNextDataLine(line);
    }
    else
    {
      while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
      {
        elementType = this->GetElementType(line);
        if (elementType < 0)
        {
          svtkErrorMacro("invalid element type");
          delete this->IS;
          this->IS = nullptr;
          return 0;
        }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        numLines = numCellsPerElement / 2;
        moreVectors = ((numCellsPerElement * 3) % 6) / 3;

        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &vector1[0], &vector1[1],
            &vector1[2], &vector2[0], &vector2[1], &vector2[2]);

          vectors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(2 * i), vector1);
          vectors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(2 * i + 1), vector2);
        }
        if (moreVectors)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreVectors; j++)
          {
            svtkEnSight6ReaderRead4(line + j * 36, &vector1[0]);
            svtkEnSight6ReaderRead4(line + j * 36 + 12, &vector1[1]);
            svtkEnSight6ReaderRead4(line + j * 36 + 24, &vector1[2]);
            vectors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(2 * i + j), vector1);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      } // end while
    }   // end else
    vectors->SetName(description);
    output->GetCellData()->AddArray(vectors);
    if (!output->GetCellData()->GetVectors())
    {
      output->GetCellData()->SetVectors(vectors);
    }
    vectors->Delete();
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::ReadTensorsPerElement(const char* fileName, const char* description,
  int timeStep, svtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  svtkFloatArray* tensors;
  int lineRead, elementType;
  float values[6], tensor[6];
  int numLines, moreTensors;
  svtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    svtkErrorMacro("nullptr TensorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    svtkDebugMacro("full path to tensor per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);                    // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    tensors = svtkFloatArray::New();
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    int realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    tensors->SetNumberOfTuples(numCells);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numCells * 6);

    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
    {
      numLines = numCells / 6;
      moreTensors = numCells % 6;

      for (k = 0; k < 6; k++)
      {
        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &values[0], &values[1],
            &values[2], &values[3], &values[5], &values[4]);
          for (j = 0; j < 6; j++)
          {
            tensors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
        if (moreTensors)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreTensors; j++)
          {
            svtkEnSight6ReaderRead4(line + j * 12, &values[j]);
            tensors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
      }
      lineRead = this->ReadNextDataLine(line);
    }
    else
    {
      while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
      {
        elementType = this->GetElementType(line);
        if (elementType < 0)
        {
          svtkErrorMacro("invalid element type");
          delete this->IS;
          this->IS = nullptr;
          return 0;
        }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        numLines = numCellsPerElement;

        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &tensor[0], &tensor[1],
            &tensor[2], &tensor[3], &tensor[5], &tensor[4]);
          tensors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(i), tensor);
        }
        lineRead = this->ReadNextDataLine(line);
      } // end while
    }   // end else
    tensors->SetName(description);
    output->GetCellData()->AddArray(tensors);
    tensors->Delete();
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::CreateUnstructuredGridOutput(
  int partId, char line[256], const char* name, svtkMultiBlockDataSet* compositeOutput)
{
  int lineRead = 1;
  char subLine[256];
  int i, j;
  svtkIdType* nodeIds;
  int* intIds;
  int numElements;
  int idx, cellId, cellType, testId;

  this->NumberOfNewOutputs++;

  if (this->GetDataSetFromBlock(compositeOutput, partId) == nullptr ||
    !this->GetDataSetFromBlock(compositeOutput, partId)->IsA("svtkUnstructuredGrid"))
  {
    svtkDebugMacro("creating new unstructured output");
    svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, ugrid);
    ugrid->Delete();

    this->UnstructuredPartIds->InsertNextId(partId);
  }

  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(this->GetDataSetFromBlock(compositeOutput, partId));

  this->SetBlockName(compositeOutput, partId, name);

  // Clear all cell ids from the last execution, if any.
  idx = this->UnstructuredPartIds->IsId(partId);
  for (i = 0; i < svtkEnSightReader::NUMBER_OF_ELEMENT_TYPES; i++)
  {
    this->GetCellIds(idx, i)->Reset();
  }

  output->Allocate(1000);

  int tmpId;
  while (lineRead && sscanf(line, " part %d", &tmpId) != 1)
  {
    this->RemoveLeadingBlanks(line);
    if (strncmp(line, "point", 5) == 0)
    {
      svtkDebugMacro("point");

      nodeIds = new svtkIdType[1];
      this->ReadNextDataLine(line);
      numElements = atoi(line);

      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (sscanf(line, " %*s %s", subLine) == 1)
        {
          // element ids listed
          // EnSight ids start at 1
          if (this->UnstructuredNodeIds)
          {
            nodeIds[0] = this->UnstructuredNodeIds->GetValue(atoi(subLine) - 1);
          }
          else
          {
            nodeIds[0] = atoi(subLine) - 1;
          }
        }
        else
        {
          if (this->UnstructuredNodeIds)
          {
            nodeIds[0] = this->UnstructuredNodeIds->GetValue(atoi(line) - 1);
          }
          else
          {
            nodeIds[0] = atoi(line) - 1;
          }
        }
        cellId = output->InsertNextCell(SVTK_VERTEX, 1, nodeIds);
        this->GetCellIds(idx, svtkEnSightReader::POINT)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
    }
    else if (strncmp(line, "bar2", 4) == 0)
    {
      svtkDebugMacro("bar2");

      nodeIds = new svtkIdType[2];
      intIds = new int[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (sscanf(line, " %*d %d %d", &intIds[0], &intIds[1]) != 2)
        {
          sscanf(line, " %d %d", &intIds[0], &intIds[1]);
        }
        for (j = 0; j < 2; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 2; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 2; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, svtkEnSightReader::BAR2)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "bar3", 4) == 0)
    {
      svtkDebugMacro("bar3");
      svtkDebugMacro("Only vertex nodes of this element will be read.");
      nodeIds = new svtkIdType[2];
      intIds = new int[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (sscanf(line, " %*d %d %*d %d", &intIds[0], &intIds[1]) != 2)
        {
          sscanf(line, " %d %*d %d", &intIds[0], &intIds[1]);
        }
        for (j = 0; j < 2; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 2; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 2; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, svtkEnSightReader::BAR3)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "tria3", 5) == 0 || strncmp(line, "tria6", 5) == 0)
    {
      if (strncmp(line, "tria6", 5) == 0)
      {
        svtkDebugMacro("tria6");
        svtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = svtkEnSightReader::TRIA6;
      }
      else
      {
        svtkDebugMacro("tria3");
        cellType = svtkEnSightReader::TRIA3;
      }

      nodeIds = new svtkIdType[3];
      intIds = new int[3];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (!(sscanf(line, " %*d %d %d %d", &intIds[0], &intIds[1], &intIds[2]) == 3 &&
              cellType == svtkEnSightReader::TRIA3) &&
          !(sscanf(line, " %*d %d %d %d %*d %*d %d", &intIds[0], &intIds[1], &intIds[2], &testId) ==
              4 &&
            cellType == svtkEnSightReader::TRIA6))
        {
          sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]);
        }
        for (j = 0; j < 3; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 3; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 3; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_TRIANGLE, 3, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "quad4", 5) == 0 || strncmp(line, "quad8", 5) == 0)
    {
      if (strncmp(line, "quad8", 5) == 0)
      {
        svtkDebugMacro("quad8");
        svtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = svtkEnSightReader::QUAD8;
      }
      else
      {
        svtkDebugMacro("quad4");
        cellType = svtkEnSightReader::QUAD4;
      }

      nodeIds = new svtkIdType[4];
      intIds = new int[4];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (!(sscanf(line, " %*d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]) ==
                4 &&
              cellType == svtkEnSightReader::QUAD4) &&
          !(sscanf(line, " %*d %d %d %d %d %*d %*d %*d %d", &intIds[0], &intIds[1], &intIds[2],
              &intIds[3], &testId) == 5 &&
            cellType == svtkEnSightReader::QUAD8))
        {
          sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]);
        }
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 4; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 4; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_QUAD, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "tetra4", 6) == 0 || strncmp(line, "tetra10", 7) == 0)
    {
      if (strncmp(line, "tetra10", 7) == 0)
      {
        svtkDebugMacro("tetra10");
        svtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = svtkEnSightReader::TETRA10;
      }
      else
      {
        svtkDebugMacro("tetra4");
        cellType = svtkEnSightReader::TETRA4;
      }

      nodeIds = new svtkIdType[4];
      intIds = new int[4];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (!(sscanf(line, " %*d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]) ==
                4 &&
              cellType == svtkEnSightReader::TETRA4) &&
          !(sscanf(line, " %*d %d %d %d %d %*d %*d %*d %*d %*d %d", &intIds[0], &intIds[1],
              &intIds[2], &intIds[3], &testId) == 5 &&
            cellType == svtkEnSightReader::TETRA10))
        {
          sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]);
        }
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 4; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 4; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_TETRA, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "pyramid5", 8) == 0 || strncmp(line, "pyramid13", 9) == 0)
    {
      if (strncmp(line, "pyramid13", 9) == 0)
      {
        svtkDebugMacro("pyramid13");
        svtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = svtkEnSightReader::PYRAMID13;
      }
      else
      {
        svtkDebugMacro("pyramid5");
        cellType = svtkEnSightReader::PYRAMID5;
      }

      nodeIds = new svtkIdType[5];
      intIds = new int[5];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (!(sscanf(line, " %*d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
                &intIds[4]) == 5 &&
              cellType == svtkEnSightReader::PYRAMID5) &&
          !(sscanf(line, " %*d %d %d %d %d %d %*d %*d %*d %*d %*d %*d %*d %d", &intIds[0],
              &intIds[1], &intIds[2], &intIds[3], &intIds[4], &testId) == 6 &&
            cellType == svtkEnSightReader::PYRAMID13))
        {
          sscanf(
            line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4]);
        }
        for (j = 0; j < 5; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 5; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 5; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_PYRAMID, 5, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "hexa8", 5) == 0 || strncmp(line, "hexa20", 6) == 0)
    {
      if (strncmp(line, "hexa20", 6) == 0)
      {
        svtkDebugMacro("hexa20");
        svtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = svtkEnSightReader::HEXA20;
      }
      else
      {
        svtkDebugMacro("hexa8");
        cellType = svtkEnSightReader::HEXA8;
      }

      nodeIds = new svtkIdType[8];
      intIds = new int[8];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        if (!(sscanf(line, " %*d %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
                &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7]) == 8 &&
              cellType == svtkEnSightReader::HEXA8) &&
          !(sscanf(line,
              " %*d %d %d %d %d %d %d %d %d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %d",
              &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6],
              &intIds[7], &testId) == 9 &&
            cellType == svtkEnSightReader::HEXA20))
        {
          sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5], &intIds[6], &intIds[7]);
        }
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 8; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 8; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_HEXAHEDRON, 8, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "penta6", 6) == 0 || strncmp(line, "penta15", 7) == 0)
    {
      if (strncmp(line, "penta15", 7) == 0)
      {
        svtkDebugMacro("penta15");
        svtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = svtkEnSightReader::PENTA15;
      }
      else
      {
        svtkDebugMacro("penta6");
        cellType = svtkEnSightReader::PENTA6;
      }

      nodeIds = new svtkIdType[6];
      intIds = new int[6];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);

      const unsigned char penta6Map[6] = { 0, 2, 1, 3, 5, 4 };
      for (i = 0; i < numElements; i++)
      {
        if (!(sscanf(line, " %*d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
                &intIds[4], &intIds[5]) == 6 &&
              cellType == svtkEnSightReader::PENTA6) &&
          !(sscanf(line, " %*d %d %d %d %d %d %d %*d %*d %*d %*d %*d %*d %*d %*d %d", &intIds[0],
              &intIds[1], &intIds[2], &intIds[3], &intIds[4], &intIds[5], &testId) == 7 &&
            cellType == svtkEnSightReader::PENTA15))
        {
          sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5]);
        }
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 6; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 6; j++)
        {
          nodeIds[penta6Map[j]] = intIds[j];
        }
        cellId = output->InsertNextCell(SVTK_WEDGE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
    {
      break;
    }
  }

  output->SetPoints(this->UnstructuredPoints);

  return lineRead;
}

//----------------------------------------------------------------------------
int svtkEnSight6Reader::CreateStructuredGridOutput(
  int partId, char line[256], const char* name, svtkMultiBlockDataSet* compositeOutput)
{
  char subLine[256];
  char formatLine[256], tempLine[256];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i, j;
  svtkPoints* points = svtkPoints::New();
  double point[3];
  int numPts, numLines, moreCoords, moreBlanking;
  float coords[6];
  int iblanks[10];

  this->NumberOfNewOutputs++;

  if (this->GetDataSetFromBlock(compositeOutput, partId) == nullptr ||
    !this->GetDataSetFromBlock(compositeOutput, partId)->IsA("svtkStructuredGrid"))
  {
    svtkDebugMacro("creating new structured grid output");
    svtkStructuredGrid* sgrid = svtkStructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, sgrid);
    sgrid->Delete();
  }

  svtkStructuredGrid* output =
    svtkStructuredGrid::SafeDownCast(this->GetDataSetFromBlock(compositeOutput, partId));
  this->SetBlockName(compositeOutput, partId, name);

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strcmp(subLine, "iblanked") == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);
  output->SetDimensions(dimensions);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  points->Allocate(numPts);

  numLines = numPts / 6; // integer division
  moreCoords = numPts % 6;

  for (i = 0; i < numLines; i++)
  {
    this->ReadNextDataLine(line);
    svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &coords[0], &coords[1],
      &coords[2], &coords[3], &coords[4], &coords[5]);
    for (j = 0; j < 6; j++)
    {
      points->InsertNextPoint(coords[j], 0.0, 0.0);
    }
  }
  if (moreCoords != 0)
  {
    this->ReadNextDataLine(line);
    for (j = 0; j < moreCoords; j++)
    {
      svtkEnSight6ReaderRead4(line + j * 12, &coords[j]);
      points->InsertNextPoint(coords[j], 0.0, 0.0);
    }
  }
  for (i = 0; i < numLines; i++)
  {
    this->ReadNextDataLine(line);
    svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &coords[0], &coords[1],
      &coords[2], &coords[3], &coords[4], &coords[5]);
    for (j = 0; j < 6; j++)
    {
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], static_cast<double>(coords[j]), point[2]);
    }
  }
  if (moreCoords != 0)
  {
    this->ReadNextDataLine(line);
    for (j = 0; j < moreCoords; j++)
    {
      svtkEnSight6ReaderRead4(line + j * 12, &coords[j]);
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], static_cast<double>(coords[j]), point[2]);
    }
  }
  for (i = 0; i < numLines; i++)
  {
    this->ReadNextDataLine(line);
    svtkEnSight6ReaderRead3(line, " %12e %12e %12e %12e %12e %12e", &coords[0], &coords[1],
      &coords[2], &coords[3], &coords[4], &coords[5]);
    for (j = 0; j < 6; j++)
    {
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], point[1], static_cast<double>(coords[j]));
    }
  }
  if (moreCoords != 0)
  {
    this->ReadNextDataLine(line);
    for (j = 0; j < moreCoords; j++)
    {
      svtkEnSight6ReaderRead4(line + j * 12, &coords[j]);
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], point[1], static_cast<double>(coords[j]));
    }
  }

  numLines = numPts / 10;
  moreBlanking = numPts % 10;
  output->SetPoints(points);
  if (iblanked)
  {
    for (i = 0; i < numLines; i++)
    {
      this->ReadNextDataLine(line);
      sscanf(line, " %d %d %d %d %d %d %d %d %d %d", &iblanks[0], &iblanks[1], &iblanks[2],
        &iblanks[3], &iblanks[4], &iblanks[5], &iblanks[6], &iblanks[7], &iblanks[8], &iblanks[9]);
      for (j = 0; j < 10; j++)
      {
        if (!iblanks[j])
        {
          output->BlankPoint(i * numLines + j);
        }
      }
    }
    if (moreBlanking != 0)
    {
      this->ReadNextDataLine(line);
      strcpy(formatLine, "");
      strcpy(tempLine, "");
      for (j = 0; j < moreBlanking; j++)
      {
        strcat(formatLine, " %d");
        sscanf(line, formatLine, &iblanks[j]);
        if (!iblanks[j])
        {
          output->BlankPoint(i * numLines + j);
        }
        strcat(tempLine, " %*d");
        strcpy(formatLine, tempLine);
      }
    }
  }

  points->Delete();
  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
void svtkEnSight6Reader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
