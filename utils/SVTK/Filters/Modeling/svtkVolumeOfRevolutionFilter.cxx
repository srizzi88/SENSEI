/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeOfRevolutionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVolumeOfRevolutionFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMergePoints.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <vector>

svtkStandardNewMacro(svtkVolumeOfRevolutionFilter);

namespace
{
struct AxisOfRevolution
{
  double Position[3];
  double Direction[3];
};

void RevolvePoint(
  const double in[3], const AxisOfRevolution* axis, double angleInRadians, double out[3])
{
  double c = cos(angleInRadians);
  double cm = 1. - c;
  double s = sin(angleInRadians);

  double translated[3];
  svtkMath::Subtract(in, axis->Position, translated);

  double dot = svtkMath::Dot(translated, axis->Direction);
  double cross[3];
  svtkMath::Cross(translated, axis->Direction, cross);

  for (svtkIdType i = 0; i < 3; i++)
  {
    out[i] =
      ((translated[i] * c + axis->Direction[i] * dot * cm - cross[i] * s) + axis->Position[i]);
  }
}

void RevolvePoints(svtkDataSet* pts, svtkPoints* newPts, AxisOfRevolution* axis, double sweepAngle,
  int resolution, svtkPointData* outPd, bool partialSweep)
{
  double angleInRadians = svtkMath::RadiansFromDegrees(sweepAngle / resolution);

  svtkIdType n2DPoints = pts->GetNumberOfPoints();
  svtkIdType counter = 0;
  double p2d[3], p3d[3];

  for (int i = 0; i < resolution + partialSweep; i++)
  {
    for (int id = 0; id < n2DPoints; id++)
    {
      pts->GetPoint(id, p2d);
      RevolvePoint(p2d, axis, i * angleInRadians, p3d);
      newPts->SetPoint(counter, p3d);
      outPd->CopyData(pts->GetPointData(), i, counter);
      counter++;
    }
  }
}

template <int CellType>
void Revolve(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution, svtkCellArray* connectivity,
  svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId, svtkCellData* outCd,
  bool partialSweep);

template <>
void Revolve<SVTK_VERTEX>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  svtkIdType newPtIds[2], newCellId;

  newPtIds[0] = pointIds->GetId(0);

  for (int i = 0; i < resolution; i++)
  {
    newPtIds[1] = (pointIds->GetId(0) + ((i + 1) % (resolution + partialSweep)) * n2DPoints);
    newCellId = connectivity->InsertNextCell(2, newPtIds);
    types->InsertNextValue(SVTK_LINE);
    outCd->CopyData(inCd, cellId, newCellId);
    newPtIds[0] = newPtIds[1];
  }
}

template <>
void Revolve<SVTK_POLY_VERTEX>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  svtkNew<svtkIdList> pointId;
  pointId->SetNumberOfIds(1);
  for (svtkIdType i = 0; i < pointIds->GetNumberOfIds(); i++)
  {
    pointId->SetId(0, pointIds->GetId(i));
    Revolve<SVTK_VERTEX>(
      pointId, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);
  }
}

template <>
void Revolve<SVTK_LINE>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 2;

  svtkIdType newPtIds[2 * nPoints], newCellId;

  for (svtkIdType i = 0; i < nPoints; i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[2 * nPoints - 1 - j] =
        pointIds->GetId(j) + ((i + 1) % (resolution + partialSweep)) * n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds);
    types->InsertNextValue(SVTK_QUAD);
    outCd->CopyData(inCd, cellId, newCellId);
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[nPoints - 1 - j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<SVTK_POLY_LINE>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  svtkNew<svtkIdList> newPointIds;
  newPointIds->SetNumberOfIds(2);
  newPointIds->SetId(0, pointIds->GetId(0));
  for (svtkIdType i = 1; i < pointIds->GetNumberOfIds(); i++)
  {
    newPointIds->SetId(1, pointIds->GetId(i));
    Revolve<SVTK_LINE>(
      newPointIds, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);
    newPointIds->SetId(0, pointIds->GetId(i));
  }
}

template <>
void Revolve<SVTK_TRIANGLE>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 3;

  svtkIdType newPtIds[2 * nPoints], newCellId;

  for (svtkIdType i = 0; i < nPoints; i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j + nPoints] =
        pointIds->GetId(j) + ((i + 1) % (resolution + partialSweep)) * n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds);
    types->InsertNextValue(SVTK_WEDGE);
    outCd->CopyData(inCd, cellId, newCellId);
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<SVTK_TRIANGLE_STRIP>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  svtkNew<svtkIdList> newPointIds;
  newPointIds->SetNumberOfIds(3);
  newPointIds->SetId(0, pointIds->GetId(0));
  newPointIds->SetId(1, pointIds->GetId(1));
  for (svtkIdType i = 2; i < pointIds->GetNumberOfIds(); i++)
  {
    newPointIds->SetId(2, pointIds->GetId(i));
    Revolve<SVTK_TRIANGLE>(
      newPointIds, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);
    newPointIds->SetId(0, pointIds->GetId(i));
    newPointIds->SetId(1, pointIds->GetId(i - 1));
  }
}

template <>
void Revolve<SVTK_QUAD>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 4;

  svtkIdType newPtIds[2 * nPoints], newCellId;

  for (svtkIdType i = 0; i < nPoints; i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j + nPoints] =
        pointIds->GetId(j) + ((i + 1) % (resolution + partialSweep)) * n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds);
    types->InsertNextValue(SVTK_HEXAHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<SVTK_PIXEL>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 4;

  svtkIdType newPtIds[2 * nPoints], newCellId;

  for (svtkIdType i = 0; i < nPoints; i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j + nPoints] =
        pointIds->GetId(j) + ((i + 1) % (resolution + partialSweep)) * n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds);
    types->InsertNextValue(SVTK_HEXAHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<SVTK_POLYGON>(svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  // A swept polygon creates a polyhedron with two polygon faces and <nPoly>
  // quad faces, comprised from 2*<nPoly> points. Because polyhedra have a
  // special connectivity format, the length of the connectivity array is
  // 1 + (<nPoly>+2) + 2*<nPoly> + 4*<nPoly> = 7*<nPoly> + 3.
  // ^        ^           ^           ^
  // integer describing # of faces (<nPoly> + 2)
  //          ^           ^           ^
  //          integers describing # of points per face
  //                      ^           ^
  //                      point ids for the two polygon faces
  //                                  ^
  //                                  point ids for the 4 quad faces

  int nPoly = pointIds->GetNumberOfIds();
  std::vector<svtkIdType> newPtIds(7 * nPoly + 3);
  // newFacePtIds are pointers to the point arrays describing each face
  std::vector<svtkIdType*> newFacePtIds(nPoly + 2);
  svtkIdType newCellId;

  newPtIds[0] = nPoly + 2;
  newPtIds[1] = nPoly;
  newPtIds[nPoly + 2] = nPoly;
  newFacePtIds[0] = &newPtIds[2];
  newFacePtIds[1] = &newPtIds[nPoly + 3];
  for (svtkIdType i = 0; i < nPoly; i++)
  {
    // All of the subsequent faces have four point ids
    newPtIds[3 + 2 * nPoly + 5 * i] = 4;
    newFacePtIds[2 + i] = &newPtIds[4 + 2 * nPoly + 5 * i];
    newFacePtIds[0][i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (svtkIdType j = 0; j < nPoly; j++)
    {
      newFacePtIds[1][nPoly - 1 - j] =
        pointIds->GetId(j) + ((i + 1) % (resolution + partialSweep)) * n2DPoints;
    }
    for (svtkIdType j = 0; j < nPoly; j++)
    {
      newFacePtIds[j + 2][0] = newFacePtIds[0][j];
      newFacePtIds[j + 2][1] = newFacePtIds[0][(j + 1) % nPoly];
      newFacePtIds[j + 2][2] = newFacePtIds[1][(2 * nPoly - 2 - j) % nPoly];
      newFacePtIds[j + 2][3] = newFacePtIds[1][nPoly - 1 - j];
    }
    newCellId = connectivity->InsertNextCell(7 * nPoly + 3, &newPtIds[0]);
    types->InsertNextValue(SVTK_POLYHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (svtkIdType j = 0; j < nPoly; j++)
    {
      newFacePtIds[0][j] = newFacePtIds[1][nPoly - 1 - j];
    }
  }
}

int RevolveCell(int cellType, svtkIdList* pointIds, svtkIdType n2DPoints, int resolution,
  svtkCellArray* connectivity, svtkUnsignedCharArray* types, svtkCellData* inCd, svtkIdType cellId,
  svtkCellData* outCd, bool partialSweep)
{
  int returnValue = 0;
#define RevolveCellCase(CellType)                                                                  \
  case CellType:                                                                                   \
    Revolve<CellType>(                                                                             \
      pointIds, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);    \
    break

  switch (cellType)
  {
    RevolveCellCase(SVTK_VERTEX);
    RevolveCellCase(SVTK_POLY_VERTEX);
    RevolveCellCase(SVTK_LINE);
    RevolveCellCase(SVTK_POLY_LINE);
    RevolveCellCase(SVTK_TRIANGLE);
    RevolveCellCase(SVTK_TRIANGLE_STRIP);
    RevolveCellCase(SVTK_POLYGON);
    RevolveCellCase(SVTK_PIXEL);
    RevolveCellCase(SVTK_QUAD);
    default:
      returnValue = 1;
  }
  return returnValue;
#undef RevolveCellCase
}
}

//----------------------------------------------------------------------------
svtkVolumeOfRevolutionFilter::svtkVolumeOfRevolutionFilter()
{
  this->SweepAngle = 360.0;
  this->Resolution = 12; // 30 degree increments
  this->AxisPosition[0] = this->AxisPosition[1] = this->AxisPosition[2] = 0.;
  this->AxisDirection[0] = this->AxisDirection[1] = 0.;
  this->AxisDirection[2] = 1.;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
svtkVolumeOfRevolutionFilter::~svtkVolumeOfRevolutionFilter() = default;

//----------------------------------------------------------------------------
int svtkVolumeOfRevolutionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointData* inPd = input->GetPointData();
  svtkCellData* inCd = input->GetCellData();
  svtkPointData* outPd = output->GetPointData();
  svtkCellData* outCd = output->GetCellData();

  svtkNew<svtkPoints> outPts;

  // Check to see that the input data is amenable to this operation
  svtkCellIterator* it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    int cellDimension = it->GetCellDimension();
    if (cellDimension > 2)
    {
      svtkErrorMacro(<< "All cells must have a topological dimension < 2.");
      return 1;
    }
  }
  it->Delete();

  // Set up output points
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      outPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      outPts->SetDataType(SVTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(SVTK_DOUBLE);
  }

  // determine whether or not the sweep angle is a full 2*pi
  bool partialSweep = false;
  if (fabs(360. - fabs(this->SweepAngle)) > 1024 * SVTK_DBL_EPSILON)
  {
    partialSweep = true;
  }

  // Set up output points and point data
  outPts->SetNumberOfPoints(input->GetNumberOfPoints() * (this->Resolution + partialSweep));
  outPd->CopyAllocate(inPd, input->GetNumberOfPoints() * (this->Resolution + partialSweep));

  // Set up output cell data
  svtkIdType nNewCells = input->GetNumberOfCells() * this->Resolution;
  outCd->CopyAllocate(inCd, nNewCells);

  svtkNew<svtkUnsignedCharArray> outTypes;
  svtkNew<svtkCellArray> outCells;

  AxisOfRevolution axis;
  for (svtkIdType i = 0; i < 3; i++)
  {
    axis.Position[i] = this->AxisPosition[i];
    axis.Direction[i] = this->AxisDirection[i];
  }

  RevolvePoints(input, outPts, &axis, this->SweepAngle, this->Resolution, outPd, partialSweep);

  it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    if (RevolveCell(it->GetCellType(), it->GetPointIds(), input->GetNumberOfPoints(),
          this->Resolution, outCells, outTypes, inCd, it->GetCellId(), outCd, partialSweep) == 1)
    {
      svtkWarningMacro(<< "No method for revolving cell type " << it->GetCellType()
                      << ". Skipping.");
    }
  }
  it->Delete();

  output->SetPoints(outPts);
  output->SetCells(outTypes, outCells);

  return 1;
}

//----------------------------------------------------------------------------
int svtkVolumeOfRevolutionFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkVolumeOfRevolutionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Sweep Angle: " << this->SweepAngle << "\n";
  os << indent << "Axis Position: (" << this->AxisPosition[0] << "," << this->AxisPosition[1] << ","
     << this->AxisPosition[2] << ")\n";
  os << indent << "Axis Direction: (" << this->AxisPosition[0] << "," << this->AxisDirection[1]
     << "," << this->AxisDirection[2] << ")\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
