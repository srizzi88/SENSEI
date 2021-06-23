/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellSizeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkCellSizeFilter.h"

#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMeshQuality.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkPolygon.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkCellSizeFilter);

//-----------------------------------------------------------------------------
svtkCellSizeFilter::svtkCellSizeFilter()
  : ComputeVertexCount(true)
  , ComputeLength(true)
  , ComputeArea(true)
  , ComputeVolume(true)
  , ComputeSum(false)
  , VertexCountArrayName(nullptr)
  , LengthArrayName(nullptr)
  , AreaArrayName(nullptr)
  , VolumeArrayName(nullptr)
{
  this->SetVertexCountArrayName("VertexCount");
  this->SetLengthArrayName("Length");
  this->SetAreaArrayName("Area");
  this->SetVolumeArrayName("Volume");
}

//-----------------------------------------------------------------------------
svtkCellSizeFilter::~svtkCellSizeFilter()
{
  this->SetVertexCountArrayName(nullptr);
  this->SetLengthArrayName(nullptr);
  this->SetAreaArrayName(nullptr);
  this->SetVolumeArrayName(nullptr);
}

//----------------------------------------------------------------------------
void svtkCellSizeFilter::ExecuteBlock(svtkDataSet* input, svtkDataSet* output, double sum[4])
{
  svtkSmartPointer<svtkIdList> cellPtIds = svtkSmartPointer<svtkIdList>::New();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkSmartPointer<svtkPoints> cellPoints = svtkSmartPointer<svtkPoints>::New();
  int cellType;
  svtkDoubleArray* arrays[4] = { nullptr, nullptr, nullptr, nullptr };
  if (this->ComputeVertexCount)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->VertexCountArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[0] = array;
  }
  if (this->ComputeLength)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->LengthArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[1] = array;
  }
  if (this->ComputeArea)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->AreaArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[2] = array;
  }
  if (this->ComputeVolume)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->VolumeArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[3] = array;
  }

  svtkNew<svtkGenericCell> cell;
  svtkPointSet* inputPS = svtkPointSet::SafeDownCast(input);

  svtkUnsignedCharArray* ghostArray = nullptr;
  if (sum)
  {
    ghostArray = input->GetCellGhostArray();
  }
  for (svtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    double value = 0;
    int cellDimension = -1;
    cellType = input->GetCellType(cellId);
    value = -1;
    switch (cellType)
    {
      case SVTK_EMPTY_CELL:
        value = 0;
        break;
      case SVTK_VERTEX:
        if (this->ComputeVertexCount)
        {
          value = 1;
          cellDimension = 0;
        }
        else
        {
          value = 0;
        }
        break;
      case SVTK_POLY_VERTEX:
        if (this->ComputeVertexCount)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = static_cast<double>(cellPtIds->GetNumberOfIds());
          cellDimension = 0;
        }
        else
        {
          value = 0;
        }
        break;
      case SVTK_POLY_LINE:
      case SVTK_LINE:
      {
        if (this->ComputeLength)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegratePolyLine(input, cellPtIds);
          cellDimension = 1;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case SVTK_TRIANGLE:
      {
        if (this->ComputeArea)
        {
          input->GetCell(cellId, cell);
          value = svtkMeshQuality::TriangleArea(cell);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case SVTK_TRIANGLE_STRIP:
      {
        if (this->ComputeArea)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegrateTriangleStrip(inputPS, cellPtIds);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case SVTK_POLYGON:
      {
        if (this->ComputeArea)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegratePolygon(inputPS, cellPtIds);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case SVTK_PIXEL:
      {
        if (this->ComputeArea)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegratePixel(input, cellPtIds);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case SVTK_QUAD:
      {
        if (this->ComputeArea)
        {
          input->GetCell(cellId, cell);
          value = svtkMeshQuality::QuadArea(cell);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case SVTK_VOXEL:
      {
        if (this->ComputeVolume)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegrateVoxel(input, cellPtIds);
          cellDimension = 3;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case SVTK_TETRA:
      {
        if (this->ComputeVolume)
        {
          input->GetCell(cellId, cell);
          value = svtkMeshQuality::TetVolume(cell);
          cellDimension = 3;
        }
        else
        {
          value = 0;
        }
      }
      break;

      default:
      {
        // We need to explicitly get the cell
        input->GetCell(cellId, cell);
        cellDimension = cell->GetCellDimension();
        switch (cellDimension)
        {
          case 0:
            if (this->ComputeVertexCount)
            {
              input->GetCellPoints(cellId, cellPtIds);
              value = static_cast<double>(cellPtIds->GetNumberOfIds());
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          case 1:
            if (this->ComputeLength)
            {
              cell->Triangulate(1, cellPtIds, cellPoints);
              value = this->IntegrateGeneral1DCell(input, cellPtIds);
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          case 2:
            if (this->ComputeArea)
            {
              cell->Triangulate(1, cellPtIds, cellPoints);
              value = this->IntegrateGeneral2DCell(inputPS, cellPtIds);
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          case 3:
            if (this->ComputeVolume)
            {
              cell->Triangulate(1, cellPtIds, cellPoints);
              value = this->IntegrateGeneral3DCell(inputPS, cellPtIds);
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          default:
            svtkWarningMacro("Unsupported Cell Dimension = " << cellDimension);
            cellDimension = -1;
        }
      }
    } // end switch (cellType)
    if (cellDimension != -1)
    { // a valid cell that we want to compute the size of
      arrays[cellDimension]->SetValue(cellId, value);
      if (sum && (!ghostArray || !ghostArray->GetValue(cellId)))
      {
        sum[cellDimension] += value;
      }
    }
  } // end cell iteration
}

//-----------------------------------------------------------------------------
int svtkCellSizeFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  bool retVal = true;
  if (svtkDataSet* inputDataSet =
        svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT())))
  {
    svtkDataSet* output = svtkDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
    double sum[4] = { 0, 0, 0, 0 };
    retVal = this->ComputeDataSet(inputDataSet, output, sum);
    if (this->ComputeSum)
    {
      this->ComputeGlobalSum(sum);
      this->AddSumFieldData(output, sum);
    }
  }
  else if (svtkCompositeDataSet* input =
             svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT())))
  {
    svtkCompositeDataSet* output =
      svtkCompositeDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
    output->CopyStructure(input);
    svtkCompositeDataIterator* iter = input->NewIterator();
    iter->SkipEmptyNodesOff();
    double sumComposite[4] = { 0, 0, 0, 0 };
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      double sum[4] = { 0, 0, 0, 0 };
      if (svtkDataSet* inputDS = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        svtkDataSet* outputDS = inputDS->NewInstance();
        retVal = retVal && this->ComputeDataSet(inputDS, outputDS, sum);
        output->SetDataSet(iter, outputDS);
        outputDS->Delete();
        if (this->ComputeSum)
        {
          this->ComputeGlobalSum(sum);
        }
      }
      if (this->ComputeSum)
      {
        for (int i = 0; i < 4; i++)
        {
          sumComposite[i] += sum[i];
        }
      }
    }
    iter->Delete();
    if (this->ComputeSum)
    {
      this->AddSumFieldData(output, sumComposite);
    }
  }
  else
  {
    retVal = false;
    svtkWarningMacro(
      "Cannot handle input of type " << inInfo->Get(svtkDataObject::DATA_OBJECT())->GetClassName());
  }

  return retVal;
}

//-----------------------------------------------------------------------------
bool svtkCellSizeFilter::ComputeDataSet(svtkDataSet* input, svtkDataSet* output, double sum[4])
{
  output->ShallowCopy(input);

  // fast path for image data since all the cells have the same volume
  if (svtkImageData* imageData = svtkImageData::SafeDownCast(input))
  {
    this->IntegrateImageData(imageData, svtkImageData::SafeDownCast(output), sum);
  }
  else
  {
    this->ExecuteBlock(input, output, sum);
  }
  if (this->ComputeSum)
  {
    this->AddSumFieldData(output, sum);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkCellSizeFilter::IntegrateImageData(svtkImageData* input, svtkImageData* output, double sum[4])
{
  int extent[6];
  input->GetExtent(extent);
  double spacing[3];
  input->GetSpacing(spacing);
  double val = 1;
  int dimension = 0;
  for (int i = 0; i < 3; i++)
  {
    if (extent[2 * i + 1] > extent[2 * i])
    {
      val *= spacing[i];
      dimension++;
    }
  }
  if (this->ComputeVertexCount)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->VertexCountArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 0)
    {
      array->SetValue(0, 1);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeLength)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->LengthArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 1)
    {
      array->Fill(val);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeArea)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->AreaArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 2)
    {
      array->Fill(val);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeVolume)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetName(this->VolumeArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 3)
    {
      array->Fill(val);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeSum)
  {
    if (svtkUnsignedCharArray* ghosts = input->GetCellGhostArray())
    {
      for (svtkIdType i = 0; i < output->GetNumberOfCells(); i++)
      {
        if (!ghosts->GetValue(i))
        {
          sum[dimension] += val;
        }
      }
    }
    else
    {
      sum[dimension] = input->GetNumberOfCells() * val;
    }
  }
}

//-----------------------------------------------------------------------------
double svtkCellSizeFilter::IntegratePolyLine(svtkDataSet* input, svtkIdList* ptIds)
{
  double sum = 0;
  double pt1[3], pt2[3];

  svtkIdType numLines = ptIds->GetNumberOfIds() - 1;
  for (svtkIdType lineIdx = 0; lineIdx < numLines; ++lineIdx)
  {
    svtkIdType pt1Id = ptIds->GetId(lineIdx);
    svtkIdType pt2Id = ptIds->GetId(lineIdx + 1);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    double length = sqrt(svtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;
  }
  return sum;
}

//-----------------------------------------------------------------------------
double svtkCellSizeFilter::IntegrateGeneral1DCell(svtkDataSet* input, svtkIdList* ptIds)
{
  // Determine the number of lines
  svtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be an even number of points from the triangulation
  if (nPnts % 2)
  {
    svtkWarningMacro("Odd number of points(" << nPnts << ")  encountered - skipping ");
    return 0;
  }

  double pt1[3], pt2[3];
  svtkIdType pid = 0;
  svtkIdType pt1Id, pt2Id;
  double sum = 0;
  while (pid < nPnts)
  {
    pt1Id = ptIds->GetId(pid++);
    pt2Id = ptIds->GetId(pid++);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    double length = sqrt(svtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;
  }
  return sum;
}

//-----------------------------------------------------------------------------
double svtkCellSizeFilter::IntegrateTriangleStrip(svtkPointSet* input, svtkIdList* ptIds)
{
  svtkIdType trianglePtIds[3];
  svtkIdType numTris = ptIds->GetNumberOfIds() - 2;
  double sum = 0;
  for (svtkIdType triIdx = 0; triIdx < numTris; ++triIdx)
  {
    trianglePtIds[0] = ptIds->GetId(triIdx);
    trianglePtIds[1] = ptIds->GetId(triIdx + 1);
    trianglePtIds[2] = ptIds->GetId(triIdx + 2);
    svtkNew<svtkTriangle> triangle;
    triangle->Initialize(3, trianglePtIds, input->GetPoints());
    sum += triangle->ComputeArea();
  }
  return sum;
}

//-----------------------------------------------------------------------------
// Works for convex polygons, and interpolation is not correct.
double svtkCellSizeFilter::IntegratePolygon(svtkPointSet* input, svtkIdList* ptIds)
{
  svtkIdType numTris = ptIds->GetNumberOfIds() - 2;
  svtkIdType trianglePtIds[3] = { ptIds->GetId(0), 0, 0 };
  double sum = 0;
  for (svtkIdType triIdx = 0; triIdx < numTris; ++triIdx)
  {
    trianglePtIds[1] = ptIds->GetId(triIdx + 1);
    trianglePtIds[2] = ptIds->GetId(triIdx + 2);
    svtkNew<svtkTriangle> triangle;
    triangle->Initialize(3, trianglePtIds, input->GetPoints());
    sum += triangle->ComputeArea();
  }
  return sum;
}

//-----------------------------------------------------------------------------
// For axis aligned rectangular cells
double svtkCellSizeFilter::IntegratePixel(svtkDataSet* input, svtkIdList* cellPtIds)
{
  svtkIdType pt1Id, pt2Id, pt3Id, pt4Id;
  double pts[4][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  // get the lengths of its 2 orthogonal sides.  Since only 1 coordinate
  // can be different we can add the differences in all 3 directions
  double l = (pts[0][0] - pts[1][0]) + (pts[0][1] - pts[1][1]) + (pts[0][2] - pts[1][2]);
  double w = (pts[0][0] - pts[2][0]) + (pts[0][1] - pts[2][1]) + (pts[0][2] - pts[2][2]);

  return fabs(l * w);
}

//-----------------------------------------------------------------------------
double svtkCellSizeFilter::IntegrateGeneral2DCell(svtkPointSet* input, svtkIdList* ptIds)
{
  svtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 3
  // from the triangulation
  if (nPnts % 3)
  {
    svtkWarningMacro("Number of points (" << nPnts << ") is not divisible by 3 - skipping ");
    return 0;
  }

  svtkIdType triIdx = 0;
  svtkIdType trianglePtIds[3];
  double sum = 0;
  while (triIdx < nPnts)
  {
    trianglePtIds[0] = ptIds->GetId(triIdx++);
    trianglePtIds[1] = ptIds->GetId(triIdx++);
    trianglePtIds[2] = ptIds->GetId(triIdx++);
    svtkNew<svtkTriangle> triangle;
    triangle->Initialize(3, trianglePtIds, input->GetPoints());
    sum += triangle->ComputeArea();
  }
  return sum;
}

//-----------------------------------------------------------------------------
// For axis aligned hexahedral cells
double svtkCellSizeFilter::IntegrateVoxel(svtkDataSet* input, svtkIdList* cellPtIds)
{
  svtkIdType pt1Id, pt2Id, pt3Id, pt4Id, pt5Id;
  double pts[5][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  pt5Id = cellPtIds->GetId(4);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);
  input->GetPoint(pt5Id, pts[4]);

  // Calculate the volume of the voxel
  double l = pts[1][0] - pts[0][0];
  double w = pts[2][1] - pts[0][1];
  double h = pts[4][2] - pts[0][2];
  return fabs(l * w * h);
}

//-----------------------------------------------------------------------------
double svtkCellSizeFilter::IntegrateGeneral3DCell(svtkPointSet* input, svtkIdList* ptIds)
{
  svtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 4
  // from the triangulation
  if (nPnts % 4)
  {
    svtkWarningMacro("Number of points (" << nPnts << ") is not divisible by 4 - skipping ");
    return 0;
  }

  svtkIdType tetIdx = 0;
  svtkIdType tetPtIds[4];
  double sum = 0;

  while (tetIdx < nPnts)
  {
    tetPtIds[0] = ptIds->GetId(tetIdx++);
    tetPtIds[1] = ptIds->GetId(tetIdx++);
    tetPtIds[2] = ptIds->GetId(tetIdx++);
    tetPtIds[3] = ptIds->GetId(tetIdx++);
    svtkNew<svtkTetra> tet;
    tet->Initialize(4, tetPtIds, input->GetPoints());
    sum += svtkMeshQuality::TetVolume(tet);
  }
  return sum;
}

//-----------------------------------------------------------------------------
void svtkCellSizeFilter::AddSumFieldData(svtkDataObject* output, double sum[4])
{
  if (this->ComputeVertexCount)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[0]);
    array->SetName(this->VertexCountArrayName);
    output->GetFieldData()->AddArray(array);
  }
  if (this->ComputeLength)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[1]);
    array->SetName(this->LengthArrayName);
    output->GetFieldData()->AddArray(array);
  }
  if (this->ComputeArea)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[2]);
    array->SetName(this->AreaArrayName);
    output->GetFieldData()->AddArray(array);
  }
  if (this->ComputeVolume)
  {
    svtkNew<svtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[3]);
    array->SetName(this->VolumeArrayName);
    output->GetFieldData()->AddArray(array);
  }
}

//-----------------------------------------------------------------------------
void svtkCellSizeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ComputeVertexCount: " << this->ComputeVertexCount << endl;
  os << indent << "ComputeLength: " << this->ComputeLength << endl;
  os << indent << "ComputeArea: " << this->ComputeArea << endl;
  os << indent << "ComputeVolume: " << this->ComputeVolume << endl;
  if (this->VertexCountArrayName)
  {
    os << indent << "VertexCountArrayName:" << this->VertexCountArrayName << endl;
  }
  else
  {
    os << indent << "VertexCountArrayName: (null)\n";
  }
  if (this->LengthArrayName)
  {
    os << indent << "LengthArrayName:" << this->LengthArrayName << endl;
  }
  else
  {
    os << indent << "LengthArrayName: (null)\n";
  }
  if (this->AreaArrayName)
  {
    os << indent << "AreaArrayName:" << this->AreaArrayName << endl;
  }
  else
  {
    os << indent << "AreaArrayName: (null)\n";
  }
  if (this->VolumeArrayName)
  {
    os << indent << "VolumeArrayName:" << this->VolumeArrayName << endl;
  }
  else
  {
    os << indent << "VolumeArrayName: (null)\n";
  }
  os << indent << "ComputeSum: " << this->ComputeSum << endl;
}
