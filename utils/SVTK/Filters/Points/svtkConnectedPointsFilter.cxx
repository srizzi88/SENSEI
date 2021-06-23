/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConnectedPointsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkConnectedPointsFilter.h"

#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkStaticPointLocator.h"

svtkStandardNewMacro(svtkConnectedPointsFilter);
svtkCxxSetObjectMacro(svtkConnectedPointsFilter, Locator, svtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Construct with default extraction mode to extract largest regions.
svtkConnectedPointsFilter::svtkConnectedPointsFilter()
{
  // How to define local neighborhood
  this->Radius = 1.0;

  // How to extract points
  this->ExtractionMode = SVTK_EXTRACT_ALL_REGIONS;

  // Seeding the extracted regions
  this->Seeds = svtkIdList::New();
  this->SpecifiedRegionIds = svtkIdList::New();

  // Support extraction of region closest to specified point
  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;

  // Segmenting based on normal alignment
  this->AlignedNormals = 0;
  this->NormalAngle = 10.0;
  this->NormalThreshold = cos(svtkMath::RadiansFromDegrees(this->NormalAngle));

  // If segmenting based on scalar values
  this->ScalarConnectivity = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  // Perform local operations efficiently
  this->Locator = svtkStaticPointLocator::New();

  // The labeling of points (i.e., their associated regions)
  this->CurrentRegionNumber = 0;
  this->RegionLabels = nullptr;

  // Keep track of region sizes
  this->NumPointsInRegion = 0;
  this->RegionSizes = svtkIdTypeArray::New();

  // Processing waves
  this->NeighborPointIds = svtkIdList::New();
  this->Wave = nullptr;
  this->Wave2 = nullptr;
}

//----------------------------------------------------------------------------
svtkConnectedPointsFilter::~svtkConnectedPointsFilter()
{
  this->Seeds->Delete();
  this->SpecifiedRegionIds->Delete();

  if (this->RegionLabels != nullptr)
  {
    this->RegionLabels->Delete();
  }
  this->RegionSizes->Delete();
  this->NeighborPointIds->Delete();

  this->SetLocator(nullptr);
}

//----------------------------------------------------------------------------
int svtkConnectedPointsFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Executing point connectivity filter.");

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    svtkDebugMacro(<< "No data to connect!");
    return 1;
  }
  svtkPoints* inPts = input->GetPoints();

  // Attribute data
  svtkPointData *pd = input->GetPointData(), *outputPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outputCD = output->GetCellData();

  // Grab normals if available and needed
  svtkFloatArray* normals = svtkFloatArray::SafeDownCast(pd->GetNormals());
  float* n = nullptr;
  if (normals && this->AlignedNormals)
  {
    n = static_cast<float*>(normals->GetVoidPointer(0));
    this->NormalThreshold = cos(svtkMath::RadiansFromDegrees(this->NormalAngle));
  }

  // Start by building the locator.
  if (!this->Locator)
  {
    svtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // See whether to consider scalar connectivity
  //
  svtkDataArray* inScalars = input->GetPointData()->GetScalars();
  if (!this->ScalarConnectivity)
  {
    inScalars = nullptr;
  }
  else
  {
    if (this->ScalarRange[1] < this->ScalarRange[0])
    {
      this->ScalarRange[1] = this->ScalarRange[0];
    }
  }

  // Initialize.  Keep track of points and cells visited.
  //
  this->RegionSizes->Reset();
  this->RegionLabels = svtkIdTypeArray::New();
  this->RegionLabels->SetName("RegionLabels");
  this->RegionLabels->SetNumberOfTuples(numPts);
  svtkIdType* labels = static_cast<svtkIdType*>(this->RegionLabels->GetVoidPointer(0));
  std::fill_n(labels, numPts, -1);

  // This is an incremental (propagating wave) traversal of the points. The
  // traversal is a function of proximity, planarity, and/or position on a
  // plane
  svtkIdType ptId;
  this->Wave = svtkIdList::New();
  this->Wave->Allocate(numPts / 4 + 1, numPts);
  this->Wave2 = svtkIdList::New();
  this->Wave2->Allocate(numPts / 4 + 1, numPts);

  // Traverse all points, and label all points
  if (this->ExtractionMode == SVTK_EXTRACT_ALL_REGIONS ||
    this->ExtractionMode == SVTK_EXTRACT_LARGEST_REGION ||
    this->ExtractionMode == SVTK_EXTRACT_SPECIFIED_REGIONS)
  {
    this->CurrentRegionNumber = 0;

    for (ptId = 0; ptId < numPts; ++ptId)
    {
      if (labels[ptId] < 0) // not yet visited
      {
        this->Wave->InsertNextId(ptId); // begin next connected wave
        this->NumPointsInRegion = 1;
        labels[ptId] = this->CurrentRegionNumber;
        this->TraverseAndMark(inPts, inScalars, n, labels);
        this->RegionSizes->InsertValue(this->CurrentRegionNumber++, this->NumPointsInRegion);
        this->Wave->Reset();
        this->Wave2->Reset();
      }
    } // for all points

    if (this->ExtractionMode == SVTK_EXTRACT_ALL_REGIONS)
    {
      // Can just copy input to output, add label array
      output->CopyStructure(input);
      outputPD->PassData(pd);
      outputCD->PassData(cd);

      outputPD->AddArray(this->RegionLabels);
      outputPD->SetActiveScalars("RegionLabels");
      this->RegionLabels->Delete();
      this->RegionLabels = nullptr;
    }

    else if (this->ExtractionMode == SVTK_EXTRACT_LARGEST_REGION)
    {
      svtkIdType regionSize, largestRegion = 0, largestRegionSize = 0;
      svtkIdType numRegions = this->RegionSizes->GetNumberOfTuples();
      for (svtkIdType regNum = 0; regNum < numRegions; ++regNum)
      {
        regionSize = this->RegionSizes->GetValue(regNum);
        if (regionSize > largestRegionSize)
        {
          largestRegionSize = regionSize;
          largestRegion = regNum;
        }
      }

      // Now create output: loop over points and find those that are in largest
      // region
      svtkPoints* outPts = svtkPoints::New(inPts->GetDataType());
      outputPD->CopyAllocate(pd);

      svtkIdType newId;
      for (ptId = 0; ptId < numPts; ++ptId)
      {
        // valid region ids (non-negative) are output.
        if (labels[ptId] == largestRegion)
        {
          newId = outPts->InsertNextPoint(inPts->GetPoint(ptId));
          outputPD->CopyData(pd, ptId, newId);
        }
      }
      output->SetPoints(outPts);
      outPts->Delete();
    }

    else // if ( this->ExtractionMode == SVTK_EXTRACT_SPECIFIED_REGIONS )
    {
      svtkPoints* outPts = svtkPoints::New(inPts->GetDataType());
      outputPD->CopyAllocate(pd);

      svtkIdType newId;
      for (ptId = 0; ptId < numPts; ++ptId)
      {
        // valid region ids (non-negative) are output.
        if (labels[ptId] >= 0 && this->SpecifiedRegionIds->IsId(labels[ptId]) >= 0)
        {
          newId = outPts->InsertNextPoint(inPts->GetPoint(ptId));
          outputPD->CopyData(pd, ptId, newId);
        }
      }
      output->SetPoints(outPts);
      outPts->Delete();
    }
  } // need to process all regions

  // Otherwise just a subset of points is extracted and labeled
  else
  {
    this->CurrentRegionNumber = 0;
    this->NumPointsInRegion = 0;
    if (this->ExtractionMode == SVTK_EXTRACT_POINT_SEEDED_REGIONS)
    {
      for (int i = 0; i < this->Seeds->GetNumberOfIds(); i++)
      {
        ptId = this->Seeds->GetId(i);
        if (ptId >= 0)
        {
          labels[ptId] = this->CurrentRegionNumber;
          this->NumPointsInRegion++;
          this->Wave->InsertNextId(ptId);
        }
      }
    }
    else if (this->ExtractionMode == SVTK_EXTRACT_CLOSEST_POINT_REGION)
    {
      ptId = this->Locator->FindClosestPoint(this->ClosestPoint);
      if (ptId >= 0)
      {
        labels[ptId] = this->CurrentRegionNumber;
        this->NumPointsInRegion++;
        this->Wave->InsertNextId(ptId);
      }
    }

    // Mark all seeded regions
    this->TraverseAndMark(inPts, inScalars, n, labels);
    this->RegionSizes->InsertValue(this->CurrentRegionNumber, this->NumPointsInRegion);

    // Now create output: loop over points and find those that are marked.
    svtkPoints* outPts = svtkPoints::New(inPts->GetDataType());
    outputPD->CopyAllocate(pd);

    svtkIdType newId;
    for (ptId = 0; ptId < numPts; ++ptId)
    {
      // valid region ids (non-negative) are output.
      if (labels[ptId] >= 0)
      {
        newId = outPts->InsertNextPoint(inPts->GetPoint(ptId));
        outputPD->CopyData(pd, ptId, newId);
      }
    }
    output->SetPoints(outPts);
    outPts->Delete();
  }

  svtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points");

  // Clean up
  this->Wave->Delete();
  this->Wave2->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Mark current points as visited and assign region number.  Note:
// traversal occurs across neighboring points.
//
void svtkConnectedPointsFilter::TraverseAndMark(
  svtkPoints* inPts, svtkDataArray* inScalars, float* normals, svtkIdType* labels)
{
  svtkIdType i, j, numPts, numIds, ptId, neiId;
  svtkIdList* tmpWave;
  double x[3];
  float *n, *n2;
  this->NeighborPointIds->Reset();
  svtkIdList* wave = this->Wave;
  svtkIdList* wave2 = this->Wave2;
  bool connectedPoint;

  while ((numIds = wave->GetNumberOfIds()) > 0)
  {
    for (i = 0; i < numIds; i++) // for all points in this wave
    {
      ptId = wave->GetId(i);
      inPts->GetPoint(ptId, x);
      this->Locator->FindPointsWithinRadius(this->Radius, x, this->NeighborPointIds);

      numPts = this->NeighborPointIds->GetNumberOfIds();
      for (j = 0; j < numPts; ++j)
      {
        neiId = this->NeighborPointIds->GetId(j);
        if (labels[neiId] < 0)
        {
          // proximial to the current point
          connectedPoint = true;

          // Does it satisfy scalar connectivity?
          if (inScalars != nullptr)
          {
            double s = inScalars->GetComponent(neiId, 0);
            if (s < this->ScalarRange[0] || s > this->ScalarRange[1])
            {
              connectedPoint = false;
            }
          }

          // Does it satisfy normal connectivity?
          if (normals != nullptr)
          {
            n = normals + 3 * ptId; // in case normals are used
            n2 = normals + 3 * neiId;
            if (svtkMath::Dot(n, n2) < this->NormalThreshold)
            {
              connectedPoint = false;
            }
          }

          // If all criterion are satisfied
          if (connectedPoint)
          {
            labels[neiId] = this->CurrentRegionNumber;
            this->NumPointsInRegion++;
            wave2->InsertNextId(neiId);
          }
        } // if point not yet visited
      }   // for all neighbors
    }     // for all cells in this wave

    tmpWave = wave;
    wave = wave2;
    wave2 = tmpWave;
    tmpWave->Reset();
  } // while wave is not empty
}

//----------------------------------------------------------------------------
// Obtain the number of connected regions.
int svtkConnectedPointsFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes->GetMaxId() + 1;
}

//----------------------------------------------------------------------------
// Initialize list of point ids used to seed regions.
void svtkConnectedPointsFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds->Reset();
}

//----------------------------------------------------------------------------
// Add a seed id (point id). Note: ids are 0-offset.
void svtkConnectedPointsFilter::AddSeed(svtkIdType id)
{
  if (id < 0)
  {
    return;
  }
  this->Modified();
  this->Seeds->InsertNextId(id);
}

//----------------------------------------------------------------------------
// Delete a seed id (point or cell id). Note: ids are 0-offset.
void svtkConnectedPointsFilter::DeleteSeed(svtkIdType id)
{
  this->Modified();
  this->Seeds->DeleteId(id);
}

//----------------------------------------------------------------------------
// Initialize list of region ids to extract.
void svtkConnectedPointsFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds->Reset();
}

//----------------------------------------------------------------------------
// Add a region id to extract. Note: ids are 0-offset.
void svtkConnectedPointsFilter::AddSpecifiedRegion(svtkIdType id)
{
  if (id < 0)
  {
    return;
  }
  this->Modified();
  this->SpecifiedRegionIds->InsertNextId(id);
}

//----------------------------------------------------------------------------
// Delete a region id to extract. Note: ids are 0-offset.
void svtkConnectedPointsFilter::DeleteSpecifiedRegion(svtkIdType id)
{
  this->Modified();
  this->SpecifiedRegionIds->DeleteId(id);
}

//----------------------------------------------------------------------------
int svtkConnectedPointsFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkConnectedPointsFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";

  os << indent << "Extraction Mode: ";
  os << this->GetExtractionModeAsString() << "\n";

  svtkIdType num;
  os << indent << "Point seeds: ";
  if ((num = this->Seeds->GetNumberOfIds()) > 1)
  {
    os << "(" << num << " seeds specified)\n";
  }
  else
  {
    os << "(no seeds specified)\n";
  }

  os << indent << "Specified regions: ";
  if ((num = this->SpecifiedRegionIds->GetNumberOfIds()) > 1)
  {
    os << "(" << num << " regions specified)\n";
  }
  else
  {
    os << "(no regions specified)\n";
  }

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " << this->ClosestPoint[1]
     << ", " << this->ClosestPoint[2] << ")\n";

  os << indent << "Scalar Connectivity: " << (this->ScalarConnectivity ? "On\n" : "Off\n");
  double* range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  os << indent << "Aligned Normals: " << (this->AlignedNormals ? "On\n" : "Off\n");
  os << indent << "Normal Angle: " << this->NormalAngle << "\n";

  os << indent << "Locator: " << this->Locator << "\n";
}
