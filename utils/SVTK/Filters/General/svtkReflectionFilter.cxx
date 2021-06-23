/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReflectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkReflectionFilter.h"

#include "svtkBoundingBox.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkCompositeDataIterator.h"
#include "svtkHigherOrderHexahedron.h"
#include "svtkHigherOrderQuadrilateral.h"
#include "svtkHigherOrderTetra.h"
#include "svtkHigherOrderWedge.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

namespace
{
/**
 * method to determine which arrays from a field data can be flipped.
 * Only 3/6/9 component signed data array are considered flippable.
 */
static void FindFlippableArrays(
  svtkFieldData* fd, std::vector<std::pair<svtkIdType, int> >& flippableArrays)
{
  // Find all flippable arrays
  for (int iArr = 0; iArr < fd->GetNumberOfArrays(); iArr++)
  {
    svtkDataArray* array = svtkDataArray::SafeDownCast(fd->GetAbstractArray(iArr));
    if (!array)
    {
      continue;
    }

    // Only signed arrays are flippable
    int dataType = array->GetDataType();
    if ((dataType == SVTK_CHAR && SVTK_TYPE_CHAR_IS_SIGNED) || dataType == SVTK_SIGNED_CHAR ||
      dataType == SVTK_SHORT || dataType == SVTK_INT || dataType == SVTK_LONG ||
      dataType == SVTK_FLOAT || dataType == SVTK_DOUBLE || dataType == SVTK_ID_TYPE)
    {
      // Only vectors and tensors are flippable
      int nComp = array->GetNumberOfComponents();
      if (nComp == 3 || nComp == 6 || nComp == 9)
      {
        flippableArrays.push_back(std::make_pair(iArr, nComp));
      }
    }
  }
}
}

svtkStandardNewMacro(svtkReflectionFilter);

//---------------------------------------------------------------------------
svtkReflectionFilter::svtkReflectionFilter()
{
  this->Plane = USE_X_MIN;
  this->Center = 0.0;
  this->CopyInput = 1;
  this->FlipAllInputArrays = false;
}

//---------------------------------------------------------------------------
svtkReflectionFilter::~svtkReflectionFilter() = default;

//---------------------------------------------------------------------------
void svtkReflectionFilter::FlipTuple(double* tuple, int* mirrorDir, int nComp)
{
  for (int j = 0; j < nComp; j++)
  {
    tuple[j] *= mirrorDir[j];
  }
}

//---------------------------------------------------------------------------
int svtkReflectionFilter::ComputeBounds(svtkDataObject* input, double bounds[6])
{
  // get the input and output
  svtkDataSet* inputDS = svtkDataSet::SafeDownCast(input);
  svtkCompositeDataSet* inputCD = svtkCompositeDataSet::SafeDownCast(input);

  if (inputDS)
  {
    inputDS->GetBounds(bounds);
    return 1;
  }

  if (inputCD)
  {
    svtkBoundingBox bbox;

    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(inputCD->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!ds)
      {
        svtkErrorMacro("Input composite dataset must be comprised for svtkDataSet "
                      "subclasses alone.");
        return 0;
      }
      bbox.AddBounds(ds->GetBounds());
    }
    if (bbox.IsValid())
    {
      bbox.GetBounds(bounds);
      return 1;
    }
  }

  return 0;
}

//---------------------------------------------------------------------------
svtkIdType svtkReflectionFilter::ReflectNon3DCell(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdType numInputPoints)
{
  svtkNew<svtkIdList> cellPts;
  input->GetCellPoints(cellId, cellPts);
  int numCellPts = cellPts->GetNumberOfIds();
  std::vector<svtkIdType> newCellPts(numCellPts);
  int cellType = input->GetCellType(cellId);
  switch (cellType)
  {
    case SVTK_QUADRATIC_EDGE:
    case SVTK_CUBIC_LINE:
    case SVTK_BEZIER_CURVE:
    case SVTK_LAGRANGE_CURVE:
    {
      for (int i = 0; i < numCellPts; i++)
      {
        newCellPts[i] = cellPts->GetId(i);
      }
      break;
    }
    case SVTK_QUADRATIC_TRIANGLE:
    {
      newCellPts[0] = cellPts->GetId(2);
      newCellPts[1] = cellPts->GetId(1);
      newCellPts[2] = cellPts->GetId(0);
      newCellPts[3] = cellPts->GetId(4);
      newCellPts[4] = cellPts->GetId(3);
      newCellPts[5] = cellPts->GetId(5);
      break;
    }
    case SVTK_BEZIER_TRIANGLE:
    case SVTK_LAGRANGE_TRIANGLE:
    {
      if (numCellPts == 7)
      {
        newCellPts[0] = cellPts->GetId(0);
        newCellPts[1] = cellPts->GetId(2);
        newCellPts[2] = cellPts->GetId(1);
        newCellPts[3] = cellPts->GetId(5);
        newCellPts[4] = cellPts->GetId(4);
        newCellPts[5] = cellPts->GetId(3);
        newCellPts[6] = cellPts->GetId(6);
      }
      else
      {
        int order = (sqrt(8 * numCellPts + 1) - 3) / 2;
        int offset = 0;
        while (order > 0)
        {
          newCellPts[offset + 0] = cellPts->GetId(offset + 0);
          newCellPts[offset + 1] = cellPts->GetId(offset + 2);
          newCellPts[offset + 2] = cellPts->GetId(offset + 1);
          const int contour_n = 3 * (order - 1);
          for (int contour = 0; contour < contour_n; contour++)
          {
            newCellPts[offset + 3 + contour] = cellPts->GetId(offset + 3 + contour_n - 1 - contour);
          }
          if (order == 3) // This is is there is a single point in the middle
          {
            newCellPts[offset + 3 + contour_n] = cellPts->GetId(offset + 3 + contour_n);
          }
          order -= 3;
          offset += 3 * order;
        }
      }

      break;
    }
    case SVTK_QUADRATIC_QUAD:
    {
      newCellPts[0] = cellPts->GetId(1);
      newCellPts[1] = cellPts->GetId(0);
      newCellPts[2] = cellPts->GetId(3);
      newCellPts[3] = cellPts->GetId(2);
      newCellPts[4] = cellPts->GetId(4);
      newCellPts[5] = cellPts->GetId(7);
      newCellPts[6] = cellPts->GetId(6);
      newCellPts[7] = cellPts->GetId(5);
      break;
    }
    case SVTK_BIQUADRATIC_QUAD:
    {
      newCellPts[0] = cellPts->GetId(1);
      newCellPts[1] = cellPts->GetId(0);
      newCellPts[2] = cellPts->GetId(3);
      newCellPts[3] = cellPts->GetId(2);
      newCellPts[4] = cellPts->GetId(4);
      newCellPts[5] = cellPts->GetId(7);
      newCellPts[6] = cellPts->GetId(6);
      newCellPts[7] = cellPts->GetId(5);
      newCellPts[8] = cellPts->GetId(8);
      break;
    }
    case SVTK_QUADRATIC_LINEAR_QUAD:
    {
      newCellPts[0] = cellPts->GetId(1);
      newCellPts[1] = cellPts->GetId(0);
      newCellPts[2] = cellPts->GetId(3);
      newCellPts[3] = cellPts->GetId(2);
      newCellPts[4] = cellPts->GetId(4);
      newCellPts[5] = cellPts->GetId(5);
      break;
    }
    case SVTK_BEZIER_QUADRILATERAL:
    case SVTK_LAGRANGE_QUADRILATERAL:
    {
      svtkCell* cell = input->GetCell(cellId);
      svtkHigherOrderQuadrilateral* cellQuad = dynamic_cast<svtkHigherOrderQuadrilateral*>(cell);
      const int* order = cellQuad->GetOrder();
      const int i_max_half = (order[0] % 2 == 0) ? (order[0] + 2) / 2 : (order[0] + 1) / 2;
      for (int i = 0; i < i_max_half; i++)
      {
        const int i_reversed = order[0] - i;
        for (int j = 0; j < order[1] + 1; j++)
        {
          const int node_id = svtkHigherOrderQuadrilateral::PointIndexFromIJK(i, j, order);
          if (i != i_reversed)
          {
            const int node_id_reversed =
              svtkHigherOrderQuadrilateral::PointIndexFromIJK(i_reversed, j, order);
            newCellPts[node_id_reversed] = cellPts->GetId(node_id);
            newCellPts[node_id] = cellPts->GetId(node_id_reversed);
          }
          else
          {
            newCellPts[node_id] = cellPts->GetId(node_id);
          }
        }
      }
      break;
    }
    default:
    {
      if (input->GetCell(cellId)->IsA("svtkNonLinearCell") || cellType > SVTK_POLYHEDRON)
      {
        svtkWarningMacro("Cell may be inverted");
      }
      for (int j = numCellPts - 1; j >= 0; j--)
      {
        newCellPts[numCellPts - 1 - j] = cellPts->GetId(j);
      }
    }
  } // end switch
  if (this->CopyInput)
  {
    for (int j = 0; j < numCellPts; j++)
    {
      newCellPts[j] += numInputPoints;
    }
  }
  return output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
}

//---------------------------------------------------------------------------
int svtkReflectionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input and output
  svtkDataSet* inputDS = svtkDataSet::GetData(inputVector[0], 0);
  svtkUnstructuredGrid* outputUG = svtkUnstructuredGrid::GetData(outputVector, 0);

  svtkCompositeDataSet* inputCD = svtkCompositeDataSet::GetData(inputVector[0], 0);
  svtkCompositeDataSet* outputCD = svtkCompositeDataSet::GetData(outputVector, 0);

  if (inputDS && outputUG)
  {
    double bounds[6];
    this->ComputeBounds(inputDS, bounds);
    return this->RequestDataInternal(inputDS, outputUG, bounds);
  }

  if (inputCD && outputCD)
  {
    outputCD->CopyStructure(inputCD);
    double bounds[6];
    if (this->ComputeBounds(inputCD, bounds))
    {
      svtkSmartPointer<svtkCompositeDataIterator> iter;
      iter.TakeReference(inputCD->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        svtkSmartPointer<svtkUnstructuredGrid> ug = svtkSmartPointer<svtkUnstructuredGrid>::New();
        if (!this->RequestDataInternal(ds, ug, bounds))
        {
          return 0;
        }

        outputCD->SetDataSet(iter, ug);
      }
    }
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int svtkReflectionFilter::RequestDataInternal(
  svtkDataSet* input, svtkUnstructuredGrid* output, double bounds[6])
{
  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  double tuple[9];
  double point[3];
  double constant[3] = { 0.0, 0.0, 0.0 };
  int mirrorDir[3] = { 1, 1, 1 };
  int mirrorSymmetricTensorDir[6] = { 1, 1, 1, 1, 1, 1 };
  int mirrorTensorDir[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkPoints> outPoints = svtkSmartPointer<svtkPoints>::New();

  if (this->CopyInput)
  {
    outPoints->Allocate(2 * numPts);
    output->Allocate(numCells * 2);
  }
  else
  {
    outPoints->Allocate(numPts);
    output->Allocate(numCells);
  }
  outPD->CopyAllOn();
  outCD->CopyAllOn();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  // Copy first points.
  if (this->CopyInput)
  {
    for (svtkIdType i = 0; i < numPts; i++)
    {
      input->GetPoint(i, point);
      outPD->CopyData(inPD, i, outPoints->InsertNextPoint(point));
    }
  }

  // Compture transformation
  switch (this->Plane)
  {
    case USE_X_MIN:
      constant[0] = 2 * bounds[0];
      break;
    case USE_X_MAX:
      constant[0] = 2 * bounds[1];
      break;
    case USE_X:
      constant[0] = 2 * this->Center;
      break;
    case USE_Y_MIN:
      constant[1] = 2 * bounds[2];
      break;
    case USE_Y_MAX:
      constant[1] = 2 * bounds[3];
      break;
    case USE_Y:
      constant[1] = 2 * this->Center;
      break;
    case USE_Z_MIN:
      constant[2] = 2 * bounds[4];
      break;
    case USE_Z_MAX:
      constant[2] = 2 * bounds[5];
      break;
    case USE_Z:
      constant[2] = 2 * this->Center;
      break;
  }

  // Compute the element-wise multiplication needed for
  // vectors/sym tensors/tensors depending on the flipping axis
  //
  // For vectors it is as following
  // X axis
  // -1  1  1
  // Y axis
  //  1 -1  1
  // Z axis
  //  1  1 -1
  //
  // For symmetric tensor it is as following
  // X axis
  //  1 -1 -1
  //     1  1
  //        1
  // Y axis
  //  1 -1  1
  //     1 -1
  //        1
  // Z axis
  //  1  1 -1
  //     1 -1
  //        1
  //
  // For tensors it is as following :
  // X axis
  //  1 -1 -1
  // -1  1  1
  // -1  1  1
  // Y axis
  //  1 -1  1
  // -1  1 -1
  //  1 -1  1
  // Z axis
  //  1  1 -1
  //  1  1 -1
  // -1 -1  1
  //
  switch (this->Plane)
  {
    case USE_X_MIN:
    case USE_X_MAX:
    case USE_X:
      mirrorDir[0] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
    case USE_Y_MIN:
    case USE_Y_MAX:
    case USE_Y:
      mirrorDir[1] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      break;
    case USE_Z_MIN:
    case USE_Z_MAX:
    case USE_Z:
      mirrorDir[2] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
  }
  svtkMath::TensorFromSymmetricTensor(mirrorSymmetricTensorDir, mirrorTensorDir);

  // Find all flippable arrays
  std::vector<std::pair<svtkIdType, int> > flippableArrays;
  if (this->FlipAllInputArrays)
  {
    FindFlippableArrays(inPD, flippableArrays);
  }
  else
  {
    // Flip only vectors, normals and tensors
    svtkDataArray* vectors = inPD->GetVectors();
    svtkDataArray* normals = inPD->GetNormals();
    svtkDataArray* tensors = inPD->GetTensors();
    for (int iArr = 0; iArr < inPD->GetNumberOfArrays(); iArr++)
    {
      svtkAbstractArray* array = inPD->GetAbstractArray(iArr);
      if (array == vectors || array == normals || array == tensors)
      {
        flippableArrays.push_back(std::make_pair(iArr, array->GetNumberOfComponents()));
      }
    }
  }

  // Copy reflected points.
  for (svtkIdType i = 0; i < numPts; i++)
  {
    input->GetPoint(i, point);
    svtkIdType ptId = outPoints->InsertNextPoint(mirrorDir[0] * point[0] + constant[0],
      mirrorDir[1] * point[1] + constant[1], mirrorDir[2] * point[2] + constant[2]);
    outPD->CopyData(inPD, i, ptId);

    // Flip flippable arrays
    for (size_t iFlip = 0; iFlip < flippableArrays.size(); iFlip++)
    {
      svtkDataArray* inArray =
        svtkDataArray::SafeDownCast(inPD->GetAbstractArray(flippableArrays[iFlip].first));
      svtkDataArray* outArray =
        svtkDataArray::SafeDownCast(outPD->GetAbstractArray(flippableArrays[iFlip].first));
      inArray->GetTuple(i, tuple);
      int nComp = flippableArrays[iFlip].second;
      switch (nComp)
      {
        case 3:
          this->FlipTuple(tuple, mirrorDir, nComp);
          break;
        case 6:
          this->FlipTuple(tuple, mirrorSymmetricTensorDir, nComp);
          break;
        case 9:
          this->FlipTuple(tuple, mirrorTensorDir, nComp);
          break;
        default:
          break;
      }
      outArray->SetTuple(ptId, tuple);
    }
  }

  svtkNew<svtkIdList> cellPts;

  // Copy original cells.
  if (this->CopyInput)
  {
    for (svtkIdType i = 0; i < numCells; i++)
    {
      // special handling for polyhedron cells
      if (svtkUnstructuredGrid::SafeDownCast(input) && input->GetCellType(i) == SVTK_POLYHEDRON)
      {
        svtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, ptIds);
        output->InsertNextCell(SVTK_POLYHEDRON, ptIds);
      }
      else
      {
        input->GetCellPoints(i, ptIds);
        output->InsertNextCell(input->GetCellType(i), ptIds);
      }
      outCD->CopyData(inCD, i, i);
    }
  }

  // Find all flippable arrays
  flippableArrays.clear();
  if (this->FlipAllInputArrays)
  {
    FindFlippableArrays(inCD, flippableArrays);
  }
  else
  {
    // Flip only vectors, normals and tensors
    svtkDataArray* vectors = inCD->GetVectors();
    svtkDataArray* normals = inCD->GetNormals();
    svtkDataArray* tensors = inCD->GetTensors();
    for (int iArr = 0; iArr < inCD->GetNumberOfArrays(); iArr++)
    {
      svtkAbstractArray* array = inCD->GetAbstractArray(iArr);
      if (array == vectors || array == normals || array == tensors)
      {
        flippableArrays.push_back(std::make_pair(iArr, array->GetNumberOfComponents()));
      }
    }
  }

  // Generate reflected cells.
  for (svtkIdType i = 0; i < numCells; i++)
  {
    svtkIdType outputCellId = -1;
    int cellType = input->GetCellType(i);
    switch (cellType)
    {
      case SVTK_TRIANGLE_STRIP:
      {
        input->GetCellPoints(i, cellPts);
        int numCellPts = cellPts->GetNumberOfIds();
        if (numCellPts % 2 != 0)
        {
          this->ReflectNon3DCell(input, output, i, numPts);
        }
        else
        {
          // Triangle strips with even number of triangles have
          // to be handled specially. A degenerate triangle is
          // introduce to flip all the triangles properly.
          input->GetCellPoints(i, cellPts);
          numCellPts++;
          std::vector<svtkIdType> newCellPts(numCellPts);
          newCellPts[0] = cellPts->GetId(0);
          newCellPts[1] = cellPts->GetId(2);
          newCellPts[2] = cellPts->GetId(1);
          newCellPts[3] = cellPts->GetId(2);
          for (int j = 4; j < numCellPts; j++)
          {
            newCellPts[j] = cellPts->GetId(j - 1);
            if (this->CopyInput)
            {
              newCellPts[j] += numPts;
            }
          }
          outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        }
        break;
      }
      case SVTK_TETRA:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[4] = { cellPts->GetId(3), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(0) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 4; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 4, newCellPts);
        break;
      }
      case SVTK_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[8] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 8; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 8, newCellPts);
        break;
      }
      case SVTK_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[6] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 6; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 6, newCellPts);
        break;
      }
      case SVTK_PYRAMID:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[5];
        for (int j = 3; j >= 0; j--)
        {
          newCellPts[3 - j] = cellPts->GetId(j);
          if (this->CopyInput)
          {
            newCellPts[3 - j] += numPts;
          }
        }
        newCellPts[4] = cellPts->GetId(4);
        if (this->CopyInput)
        {
          newCellPts[4] += numPts;
        }
        outputCellId = output->InsertNextCell(cellType, 5, newCellPts);
        break;
      }
      case SVTK_PENTAGONAL_PRISM:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[10] = { cellPts->GetId(5), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(0), cellPts->GetId(1),
          cellPts->GetId(2), cellPts->GetId(3), cellPts->GetId(4) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 10; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 10, newCellPts);
        break;
      }
      case SVTK_HEXAGONAL_PRISM:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[12] = { cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8),
          cellPts->GetId(9), cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(0),
          cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(3), cellPts->GetId(4),
          cellPts->GetId(5) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 12; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 12, newCellPts);
        break;
      }
      case SVTK_QUADRATIC_TETRA:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[10] = { cellPts->GetId(3), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(0), cellPts->GetId(8), cellPts->GetId(5), cellPts->GetId(9),
          cellPts->GetId(7), cellPts->GetId(4), cellPts->GetId(6) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 10; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 10, newCellPts);
        break;
      }
      case SVTK_QUADRATIC_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[20] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10),
          cellPts->GetId(11), cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18),
          cellPts->GetId(19) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 20; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 20, newCellPts);
        break;
      }
      case SVTK_QUADRATIC_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[15] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9),
          cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 15; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 15, newCellPts);
        break;
      }
      case SVTK_QUADRATIC_PYRAMID:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[113] = { cellPts->GetId(2), cellPts->GetId(1), cellPts->GetId(0),
          cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(6), cellPts->GetId(5),
          cellPts->GetId(8), cellPts->GetId(7), cellPts->GetId(11), cellPts->GetId(10),
          cellPts->GetId(9), cellPts->GetId(12) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 13; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 13, newCellPts);
        break;
      }
      case SVTK_TRIQUADRATIC_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[27] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10),
          cellPts->GetId(11), cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18),
          cellPts->GetId(19), cellPts->GetId(20), cellPts->GetId(21), cellPts->GetId(22),
          cellPts->GetId(23), cellPts->GetId(25), cellPts->GetId(24), cellPts->GetId(26) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 27; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 27, newCellPts);
        break;
      }
      case SVTK_QUADRATIC_LINEAR_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[12] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9),
          cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 12; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 12, newCellPts);
        break;
      }
      case SVTK_BIQUADRATIC_QUADRATIC_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[18] = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5),
          cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9),
          cellPts->GetId(10), cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7),
          cellPts->GetId(8), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(16), cellPts->GetId(17) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 18; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 18, newCellPts);
        break;
      }
      case SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        svtkIdType newCellPts[24] = { cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(6),
          cellPts->GetId(7), cellPts->GetId(0), cellPts->GetId(1), cellPts->GetId(2),
          cellPts->GetId(3), cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14),
          cellPts->GetId(15), cellPts->GetId(8), cellPts->GetId(9), cellPts->GetId(10),
          cellPts->GetId(11), cellPts->GetId(16), cellPts->GetId(17), cellPts->GetId(18),
          cellPts->GetId(19), cellPts->GetId(20), cellPts->GetId(21), cellPts->GetId(22),
          cellPts->GetId(23) };
        if (this->CopyInput)
        {
          for (int j = 0; j < 24; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, 24, newCellPts);
        break;
      }
      case SVTK_POLYHEDRON:
      {
        svtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, cellPts);
        svtkIdType* idPtr = cellPts->GetPointer(0);
        int nfaces = static_cast<int>(*idPtr++);
        for (int j = 0; j < nfaces; j++)
        {
          svtkIdType npts = *idPtr++;
          for (svtkIdType k = 0; k < (npts + 1) / 2; k++)
          {
            svtkIdType temp = idPtr[k];
            idPtr[k] = idPtr[npts - 1 - k];
            idPtr[npts - 1 - k] = temp;
          }
          if (this->CopyInput)
          {
            for (svtkIdType k = 0; k < npts; k++)
            {
              idPtr[k] += numPts;
            }
          }
          idPtr += npts;
        }
        outputCellId = output->InsertNextCell(cellType, cellPts);
        break;
      }
      case SVTK_BEZIER_HEXAHEDRON:
      case SVTK_LAGRANGE_HEXAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        const int numCellPts = cellPts->GetNumberOfIds();
        std::vector<svtkIdType> newCellPts(numCellPts);

        svtkCell* cell = input->GetCell(i);
        svtkHigherOrderHexahedron* cellHex = dynamic_cast<svtkHigherOrderHexahedron*>(cell);
        const int* order = cellHex->GetOrder();
        const int k_max_half = (order[2] % 2 == 0) ? (order[2] + 2) / 2 : (order[2] + 1) / 2;
        for (int ii = 0; ii < order[0] + 1; ii++)
        {
          for (int jj = 0; jj < order[1] + 1; jj++)
          {
            for (int kk = 0; kk < k_max_half; kk++)
            {
              const int kk_reversed = order[2] - kk;
              const int node_id = svtkHigherOrderHexahedron::PointIndexFromIJK(ii, jj, kk, order);
              if (kk != kk_reversed)
              {
                const int node_id_reversed =
                  svtkHigherOrderHexahedron::PointIndexFromIJK(ii, jj, kk_reversed, order);
                newCellPts[node_id_reversed] = cellPts->GetId(node_id);
                newCellPts[node_id] = cellPts->GetId(node_id_reversed);
              }
              else
              {
                newCellPts[node_id] = cellPts->GetId(node_id);
              }
            }
          }
        }
        if (this->CopyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        break;
      }
      case SVTK_BEZIER_WEDGE:
      case SVTK_LAGRANGE_WEDGE:
      {
        input->GetCellPoints(i, cellPts);
        const int numCellPts = cellPts->GetNumberOfIds();
        std::vector<svtkIdType> newCellPts(numCellPts);
        if (numCellPts == 21)
        {
          newCellPts = { cellPts->GetId(3), cellPts->GetId(4), cellPts->GetId(5), cellPts->GetId(0),
            cellPts->GetId(1), cellPts->GetId(2), cellPts->GetId(9), cellPts->GetId(10),
            cellPts->GetId(11), cellPts->GetId(6), cellPts->GetId(7), cellPts->GetId(8),
            cellPts->GetId(12), cellPts->GetId(13), cellPts->GetId(14), cellPts->GetId(16),
            cellPts->GetId(15), cellPts->GetId(17), cellPts->GetId(18), cellPts->GetId(19),
            cellPts->GetId(20) };
        }
        else
        {
          svtkCell* cell = input->GetCell(i);
          svtkHigherOrderWedge* cellWedge = dynamic_cast<svtkHigherOrderWedge*>(cell);
          const int* order = cellWedge->GetOrder();
          const int k_max_half = (order[2] % 2 == 0) ? (order[2] + 2) / 2 : (order[2] + 1) / 2;
          for (int ii = 0; ii < order[0] + 1; ii++)
          {
            for (int jj = 0; jj < order[0] + 1 - ii; jj++)
            {
              for (int kk = 0; kk < k_max_half; kk++)
              {
                const int kk_reversed = order[2] - kk;
                const int node_id = svtkHigherOrderWedge::PointIndexFromIJK(ii, jj, kk, order);
                if (kk != kk_reversed)
                {
                  const int node_id_reversed =
                    svtkHigherOrderWedge::PointIndexFromIJK(ii, jj, kk_reversed, order);
                  newCellPts[node_id_reversed] = cellPts->GetId(node_id);
                  newCellPts[node_id] = cellPts->GetId(node_id_reversed);
                }
                else
                {
                  newCellPts[node_id] = cellPts->GetId(node_id);
                }
              }
            }
          }
        }
        if (this->CopyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        break;
      }
      case SVTK_BEZIER_TETRAHEDRON:
      case SVTK_LAGRANGE_TETRAHEDRON:
      {
        input->GetCellPoints(i, cellPts);
        const int numCellPts = cellPts->GetNumberOfIds();
        std::vector<svtkIdType> newCellPts(numCellPts);
        if (numCellPts == 15)
        {
          newCellPts = { cellPts->GetId(0), cellPts->GetId(2), cellPts->GetId(1), cellPts->GetId(3),
            cellPts->GetId(6), cellPts->GetId(5), cellPts->GetId(4), cellPts->GetId(7),
            cellPts->GetId(9), cellPts->GetId(8), cellPts->GetId(10), cellPts->GetId(13),
            cellPts->GetId(12), cellPts->GetId(11), cellPts->GetId(14) };
        }
        else
        {
          const int order = svtkHigherOrderTetra::ComputeOrder(numCellPts);
          for (int ii = 0; ii < order + 1; ii++)
          {
            for (int jj = 0; jj < order + 1 - ii; jj++)
            {
              for (int kk = 0; kk < order + 1 - jj; kk++)
              {
                for (int ll = 0; ll < order + 1 - kk; ll++)
                {
                  if ((ii + jj + kk + ll) == order)
                  {
                    const svtkIdType bindex[4]{ ii, jj, kk, ll };
                    const svtkIdType bindex_reversed[4]{ ii, jj, ll, kk };
                    const svtkIdType node_id = svtkHigherOrderTetra::Index(bindex, order);
                    const svtkIdType node_id_reversed =
                      svtkHigherOrderTetra::Index(bindex_reversed, order);
                    newCellPts[node_id] = cellPts->GetId(node_id_reversed);
                  }
                }
              }
            }
          }
        }
        if (this->CopyInput)
        {
          for (int j = 0; j < numCellPts; j++)
          {
            newCellPts[j] += numPts;
          }
        }
        outputCellId = output->InsertNextCell(cellType, numCellPts, &newCellPts[0]);
        break;
      }
      default:
      {
        outputCellId = this->ReflectNon3DCell(input, output, i, numPts);
      }
    }
    outCD->CopyData(inCD, i, outputCellId);

    // Flip flippable arrays
    for (size_t iFlip = 0; iFlip < flippableArrays.size(); iFlip++)
    {
      svtkDataArray* inArray =
        svtkDataArray::SafeDownCast(inCD->GetAbstractArray(flippableArrays[iFlip].first));
      svtkDataArray* outArray =
        svtkDataArray::SafeDownCast(outCD->GetAbstractArray(flippableArrays[iFlip].first));
      inArray->GetTuple(i, tuple);
      int nComp = flippableArrays[iFlip].second;
      switch (nComp)
      {
        case 3:
          this->FlipTuple(tuple, mirrorDir, nComp);
          break;
        case 6:
          this->FlipTuple(tuple, mirrorSymmetricTensorDir, nComp);
          break;
        case 9:
          this->FlipTuple(tuple, mirrorTensorDir, nComp);
          break;
        default:
          break;
      }
      outArray->SetTuple(outputCellId, tuple);
    }
  }

  output->SetPoints(outPoints);
  output->CheckAttributes();

  return 1;
}

//---------------------------------------------------------------------------
int svtkReflectionFilter::FillInputPortInformation(int, svtkInformation* info)
{
  // Input can be a dataset or a composite of datasets.
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//---------------------------------------------------------------------------
int svtkReflectionFilter::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkDataObject* input = svtkDataObject::GetData(inInfo);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
  {
    svtkDataObject* output = svtkDataObject::GetData(outInfo);
    // If input is composite dataset, output is a svtkMultiBlockDataSet of
    // unstructrued grids.
    // If input is a dataset, output is an unstructured grid.
    if (!output || (input->IsA("svtkCompositeDataSet") && !output->IsA("svtkMultiBlockDataSet")) ||
      (input->IsA("svtkDataSet") && !output->IsA("svtkUnstructuredGrid")))
    {
      svtkDataObject* newOutput = nullptr;
      if (input->IsA("svtkCompositeDataSet"))
      {
        newOutput = svtkMultiBlockDataSet::New();
      }
      else // if (input->IsA("svtkDataSet"))
      {
        newOutput = svtkUnstructuredGrid::New();
      }
      outInfo->Set(svtkDataSet::DATA_OBJECT(), newOutput);
      newOutput->FastDelete();
    }
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
void svtkReflectionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
  os << indent << "Center: " << this->Center << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
}
