/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProjectSphereFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProjectSphereFilter.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMergePoints.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <map>

namespace
{
void ConvertXYZToLatLonDepth(double xyz[3], double lonLatDepth[3], double center[3])
{
  double dist2 = svtkMath::Distance2BetweenPoints(xyz, center);
  lonLatDepth[2] = sqrt(dist2);
  double radianAngle = atan2(xyz[1] - center[1], xyz[0] - center[0]);
  lonLatDepth[0] = radianAngle * 180. / svtkMath::Pi() - 180.;
  lonLatDepth[1] = 90. - acos((xyz[2] - center[2]) / lonLatDepth[2]) * 180. / svtkMath::Pi();
}

template <class data_type>
void TransformVector(double* transformMatrix, data_type* data)
{
  double d0 = static_cast<double>(data[0]);
  double d1 = static_cast<double>(data[1]);
  double d2 = static_cast<double>(data[2]);
  data[0] = static_cast<data_type>(
    transformMatrix[0] * d0 + transformMatrix[1] * d1 + transformMatrix[2] * d2);
  data[1] = static_cast<data_type>(
    transformMatrix[3] * d0 + transformMatrix[4] * d1 + transformMatrix[5] * d2);
  data[2] = static_cast<data_type>(
    transformMatrix[6] * d0 + transformMatrix[7] * d1 + transformMatrix[8] * d2);
}
} // end anonymous namespace

svtkStandardNewMacro(svtkProjectSphereFilter);

//-----------------------------------------------------------------------------
svtkProjectSphereFilter::svtkProjectSphereFilter()
  : SplitLongitude(-180)
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0;
  this->KeepPolePoints = false;
  this->TranslateZ = false;
}

//-----------------------------------------------------------------------------
svtkProjectSphereFilter::~svtkProjectSphereFilter() = default;

//-----------------------------------------------------------------------------
void svtkProjectSphereFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  double center[3];
  this->GetCenter(center);

  os << indent << "Center: (" << center[0] << ", " << center[1] << ", " << center[2] << ")\n";
  os << indent << "KeepPolePoints " << this->GetKeepPolePoints() << "\n";
  os << indent << "TranslateZ " << this->GetTranslateZ() << "\n";
}

//-----------------------------------------------------------------------------
int svtkProjectSphereFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkProjectSphereFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDebugMacro("RequestData");

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (svtkPolyData* poly = svtkPolyData::SafeDownCast(input))
  {
    if (poly->GetVerts()->GetNumberOfCells() > 0 || poly->GetLines()->GetNumberOfCells() > 0 ||
      poly->GetStrips()->GetNumberOfCells() > 0)
    {
      svtkErrorMacro("Can only deal with svtkPolyData polys.");
      return 0;
    }
  }

  svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkNew<svtkIdList> polePointIds;
  this->TransformPointInformation(input, output, polePointIds.GetPointer());
  this->TransformCellInformation(input, output, polePointIds.GetPointer());
  output->GetFieldData()->ShallowCopy(input->GetFieldData());

  svtkDebugMacro("Leaving RequestData");

  return 1;
}

//-----------------------------------------------------------------------------
void svtkProjectSphereFilter::TransformPointInformation(
  svtkPointSet* input, svtkPointSet* output, svtkIdList* polePointIds)
{
  polePointIds->Reset();

  // Deep copy point data since TransformPointInformation modifies
  // the point data
  output->GetPointData()->DeepCopy(input->GetPointData());

  svtkNew<svtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(input->GetNumberOfPoints());

  double zTranslation = (this->TranslateZ == true ? this->GetZTranslation(input) : 0.);

  output->SetPoints(points.GetPointer());
  svtkIdType numberOfPoints = input->GetNumberOfPoints();
  double minDist2ToCenterLine = SVTK_DOUBLE_MAX;
  for (svtkIdType i = 0; i < numberOfPoints; i++)
  {
    double coordIn[3], coordOut[3];
    input->GetPoint(i, coordIn);
    ConvertXYZToLatLonDepth(coordIn, coordOut, this->Center);
    // if we allow the user to specify SplitLongitude we have to make
    // sure that we respect their choice since the output of atan
    // is from -180 to 180.
    if (coordOut[0] < this->SplitLongitude)
    {
      coordOut[0] += 360.;
    }
    coordOut[2] -= zTranslation;

    // acbauer -- a hack to make the grid look better by forcing it to be flat.
    // leaving this in for now even though it's commented out. if I figure out
    // a proper way to do this i'll replace it.
    // if(this->TranslateZ)
    //   {
    //   coordOut[2] = 0;
    //   }
    points->SetPoint(i, coordOut);

    // keep track of the ids of the points that are closest to the
    // centerline between -90 and 90 latitude. this is done as a single
    // pass algorithm.
    double dist2 = (coordIn[0] - this->Center[0]) * (coordIn[0] - this->Center[0]) +
      (coordIn[1] - this->Center[1]) * (coordIn[1] - this->Center[1]);
    if (dist2 < minDist2ToCenterLine)
    {
      // we found a closer point so throw out the previous closest
      // point ids.
      minDist2ToCenterLine = dist2;
      polePointIds->SetNumberOfIds(1);
      polePointIds->SetId(0, i);
    }
    else if (dist2 == minDist2ToCenterLine)
    {
      // this point is just as close as the current closest point
      // so we just add it to our list.
      polePointIds->InsertNextId(i);
    }
    this->TransformTensors(i, coordIn, output->GetPointData());
  }
  this->ComputePointsClosestToCenterLine(minDist2ToCenterLine, polePointIds);
}

//-----------------------------------------------------------------------------
void svtkProjectSphereFilter::TransformCellInformation(
  svtkPointSet* input, svtkPointSet* output, svtkIdList* polePointIds)
{
  // a map from the old point to the newly created point for split cells
  std::map<svtkIdType, svtkIdType> boundaryMap;

  double TOLERANCE = .0001;
  svtkNew<svtkMergePoints> locator;
  locator->InitPointInsertion(
    output->GetPoints(), output->GetBounds(), output->GetNumberOfPoints());
  double coord[3];
  for (svtkIdType i = 0; i < output->GetNumberOfPoints(); i++)
  {
    // this is a bit annoying but required for building up the locator properly
    // otherwise it won't either know these points exist or will start
    // counting new points at index 0.
    output->GetPoint(i, coord);
    locator->InsertNextPoint(coord);
  }

  svtkIdType numberOfCells = input->GetNumberOfCells();
  svtkCellArray* connectivity = nullptr;
  svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast(output);
  svtkPolyData* poly = svtkPolyData::SafeDownCast(output);
  if (ugrid)
  {
    ugrid->Allocate(numberOfCells);
    connectivity = ugrid->GetCells();
  }
  else if (poly)
  {
    poly->AllocateEstimate(numberOfCells, 3);
    connectivity = poly->GetPolys();
  }
  output->GetCellData()->CopyAllOn();
  output->GetCellData()->CopyAllocate(input->GetCellData(), input->GetNumberOfCells());
  svtkPointData* pointData = output->GetPointData();
  pointData->CopyAllOn();
  pointData->CopyAllocate(pointData, output->GetNumberOfPoints());

  svtkNew<svtkIdList> cellPoints;
  svtkNew<svtkIdList> skippedCells;
  svtkIdType mostPointsInCell = 0;
  for (svtkIdType cellId = 0; cellId < numberOfCells; cellId++)
  {
    bool onLeftBoundary = false;
    bool onRightBoundary = false;
    bool leftSideInterior = false;  // between SplitLongitude and SplitLongitude+90
    bool rightSideInterior = false; // between SplitLongitude+270 and SplitLongitude+360
    bool middleInterior = false;    // between SplitLongitude+90 and SplitLongitude+270

    bool skipCell = false;
    bool splitCell = false;
    double xyz[3];
    input->GetCellPoints(cellId, cellPoints.GetPointer());
    mostPointsInCell =
      (mostPointsInCell > cellPoints->GetNumberOfIds() ? mostPointsInCell
                                                       : cellPoints->GetNumberOfIds());
    for (svtkIdType pt = 0; pt < cellPoints->GetNumberOfIds(); pt++)
    {
      output->GetPoint(cellPoints->GetId(pt), xyz);
      if (xyz[0] < this->SplitLongitude + TOLERANCE)
      {
        onLeftBoundary = true;
      }
      else if (xyz[0] > this->SplitLongitude + 360. - TOLERANCE)
      {
        onRightBoundary = true;
      }
      else if (xyz[0] < this->SplitLongitude + 90.)
      {
        leftSideInterior = true;
      }
      else if (xyz[0] > this->SplitLongitude + 270.)
      {
        rightSideInterior = true;
      }
      else
      {
        middleInterior = true;
      }
      if (polePointIds->IsId(cellPoints->GetId(pt)) != -1 && this->KeepPolePoints == false)
      {
        skipCell = true;
        skippedCells->InsertNextId(cellId);
        continue;
      }
    }
    if (skipCell)
    {
      continue;
    }
    if ((onLeftBoundary || onRightBoundary) && rightSideInterior && leftSideInterior)
    { // this cell stretches across the split longitude
      splitCell = true;
    }
    else if (onLeftBoundary && rightSideInterior)
    {
      for (svtkIdType pt = 0; pt < cellPoints->GetNumberOfIds(); pt++)
      {
        output->GetPoint(cellPoints->GetId(pt), xyz);
        if (xyz[0] < this->SplitLongitude + TOLERANCE)
        {
          std::map<svtkIdType, svtkIdType>::iterator it = boundaryMap.find(cellPoints->GetId(pt));
          if (it == boundaryMap.end())
          { // need to create another point
            xyz[0] += 360.;
            svtkIdType id = locator->InsertNextPoint(xyz);
            boundaryMap[cellPoints->GetId(pt)] = id;
            pointData->CopyData(pointData, cellPoints->GetId(pt), id);
            cellPoints->SetId(pt, id);
          }
          else
          {
            cellPoints->SetId(pt, it->second);
          }
        }
      }
    }
    else if (onRightBoundary && leftSideInterior)
    {
      for (svtkIdType pt = 0; pt < cellPoints->GetNumberOfIds(); pt++)
      {
        output->GetPoint(cellPoints->GetId(pt), xyz);
        if (xyz[0] > this->SplitLongitude + 360. - TOLERANCE)
        {
          std::map<svtkIdType, svtkIdType>::iterator it = boundaryMap.find(cellPoints->GetId(pt));
          if (it == boundaryMap.end())
          { // need to create another point
            xyz[0] -= 360.;
            svtkIdType id = locator->InsertNextPoint(xyz);
            boundaryMap[cellPoints->GetId(pt)] = id;
            pointData->CopyData(pointData, cellPoints->GetId(pt), id);
            cellPoints->SetId(pt, id);
          }
          else
          {
            cellPoints->SetId(pt, it->second);
          }
        }
      }
    }
    else if ((onLeftBoundary || onRightBoundary) && middleInterior)
    {
      splitCell = true;
    }
    else if (leftSideInterior && rightSideInterior)
    {
      splitCell = true;
    }
    if (splitCell)
    {
      this->SplitCell(input, output, cellId, locator.GetPointer(), connectivity, 0);
      this->SplitCell(input, output, cellId, locator.GetPointer(), connectivity, 1);
    }
    else if (ugrid)
    {
      ugrid->InsertNextCell(input->GetCellType(cellId), cellPoints.GetPointer());
      output->GetCellData()->CopyData(input->GetCellData(), cellId, output->GetNumberOfCells() - 1);
    }
    else if (poly)
    {
      poly->InsertNextCell(input->GetCellType(cellId), cellPoints.GetPointer());
      output->GetCellData()->CopyData(input->GetCellData(), cellId, output->GetNumberOfCells() - 1);
    }
  }

  if (poly)
  {
    // we have to rebuild the polydata cell data structures since when
    // we split a cell we don't do it right away due to the expense
    poly->DeleteCells();
    poly->BuildCells();
  }

  // deal with cell data
  std::vector<double> weights(mostPointsInCell);
  svtkIdType skipCounter = 0;
  for (svtkIdType cellId = 0; cellId < input->GetNumberOfCells(); cellId++)
  {
    if (skippedCells->IsId(cellId) != -1)
    {
      skippedCells->DeleteId(cellId);
      skipCounter++;
      continue;
    }
    int subId = 0;
    double parametricCenter[3];
    svtkCell* cell = input->GetCell(cellId);
    cell->GetParametricCenter(parametricCenter);
    cell->EvaluateLocation(subId, parametricCenter, coord, &weights[0]);
    this->TransformTensors(cellId - skipCounter, coord, output->GetCellData());
  }
}

//-----------------------------------------------------------------------------
void svtkProjectSphereFilter::TransformTensors(
  svtkIdType pointId, double* coord, svtkDataSetAttributes* dataArrays)
{
  double theta = atan2(sqrt((coord[0] - this->Center[0]) * (coord[0] - this->Center[0]) +
                         (coord[1] - this->Center[1]) * (coord[1] - this->Center[1])),
    coord[2] - this->Center[2]);
  double phi = atan2(coord[1] - this->Center[1], coord[0] - this->Center[0]);
  double sinTheta = sin(theta);
  double cosTheta = cos(theta);
  double sinPhi = sin(phi);
  double cosPhi = cos(phi);
  double transformMatrix[9] = { -sinPhi, cosPhi, 0., cosTheta * cosPhi, cosTheta * sinPhi,
    -sinTheta, sinTheta * cosPhi, sinTheta * sinPhi, cosTheta };
  for (int i = 0; i < dataArrays->GetNumberOfArrays(); i++)
  {
    svtkDataArray* array = dataArrays->GetArray(i);
    if (array->GetNumberOfComponents() == 3)
    {
      switch (array->GetDataType())
      {
        svtkTemplateMacro(TransformVector(transformMatrix,
          static_cast<SVTK_TT*>(array->GetVoidPointer(pointId * array->GetNumberOfComponents()))));
      }
    }
  }
}

//-----------------------------------------------------------------------------
double svtkProjectSphereFilter::GetZTranslation(svtkPointSet* input)
{
  double maxRadius2 = 0; // squared radius
  double coord[3];
  for (svtkIdType i = 0; i < input->GetNumberOfPoints(); i++)
  {
    input->GetPoint(i, coord);
    double dist2 = svtkMath::Distance2BetweenPoints(coord, this->Center);
    if (dist2 > maxRadius2)
    {
      maxRadius2 = dist2;
    }
  }
  return sqrt(maxRadius2);
}

//-----------------------------------------------------------------------------
void svtkProjectSphereFilter::SplitCell(svtkPointSet* input, svtkPointSet* output,
  svtkIdType inputCellId, svtkIncrementalPointLocator* locator, svtkCellArray* connectivity,
  int splitSide)
{
  // i screw up the canonical ordering of the cell but apparently this
  // gets fixed by svtkCell::Clip().
  svtkCell* cell = input->GetCell(inputCellId);
  svtkNew<svtkDoubleArray> cellScalars;
  cellScalars->SetNumberOfTuples(cell->GetNumberOfPoints());
  double coord[3];
  for (svtkIdType pt = 0; pt < cell->GetNumberOfPoints(); pt++)
  {
    output->GetPoint(cell->GetPointId(pt), coord);
    if (splitSide == 0 && coord[0] > this->SplitLongitude + 180.)
    {
      coord[0] -= 360.;
    }
    else if (splitSide == 1 && coord[0] < this->SplitLongitude + 180.)
    {
      coord[0] += 360.;
    }
    cellScalars->SetValue(pt, coord[0]);
    cell->GetPoints()->SetPoint(pt, coord);
  }
  svtkIdType numberOfCells = output->GetNumberOfCells();
  double splitLocation = (splitSide == 0 ? -180. : 180.);
  cell->Clip(splitLocation, cellScalars.GetPointer(), locator, connectivity, output->GetPointData(),
    output->GetPointData(), input->GetCellData(), inputCellId, output->GetCellData(), splitSide);
  // if the grid was an unstructured grid we have to update the cell
  // types and locations for the created cells.
  if (svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast(output))
  {
    this->SetCellInformation(ugrid, cell, output->GetNumberOfCells() - numberOfCells);
  }
}

//-----------------------------------------------------------------------------
void svtkProjectSphereFilter::SetCellInformation(
  svtkUnstructuredGrid* output, svtkCell* cell, svtkIdType numberOfNewCells)
{
  for (svtkIdType i = 0; i < numberOfNewCells; i++)
  {
    svtkIdType prevCellId = output->GetNumberOfCells() + i - numberOfNewCells - 1;
    svtkIdType newCellId = prevCellId + 1;
    svtkIdType numPts;
    const svtkIdType* pts;
    output->GetCells()->GetCellAtId(newCellId, numPts, pts);
    if (cell->GetCellDimension() == 0)
    {
      if (numPts > 2)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_POLY_VERTEX);
      }
      else
      {
        svtkErrorMacro("Cannot handle 0D cell with " << numPts << " number of points.");
      }
    }
    else if (cell->GetCellDimension() == 1)
    {
      if (numPts == 2)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_LINE);
      }
      else if (numPts > 2)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_POLY_LINE);
      }
      else
      {
        svtkErrorMacro("Cannot handle 1D cell with " << numPts << " number of points.");
      }
    }
    else if (cell->GetCellDimension() == 2)
    {
      if (numPts == 3)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_TRIANGLE);
      }
      else if (numPts > 3 && cell->GetCellType() == SVTK_TRIANGLE_STRIP)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_TRIANGLE_STRIP);
      }
      else if (numPts == 4)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_QUAD);
      }
      else
      {
        svtkErrorMacro("Cannot handle 2D cell with " << numPts << " number of points.");
      }
    }
    else // 3D cell
    {
      if (numPts == 4)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_TETRA);
      }
      else if (numPts == 5)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_PYRAMID);
      }
      else if (numPts == 6)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_WEDGE);
      }
      else if (numPts == 8)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, SVTK_HEXAHEDRON);
      }
      else
      {
        svtkErrorMacro("Unknown 3D cell type.");
      }
    }
  }
}
