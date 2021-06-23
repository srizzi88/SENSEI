/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQuadraturePointsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkQuadraturePointsGenerator.h"

#include "svtkArrayDispatch.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkType.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridAlgorithm.h"

#include "svtkQuadraturePointsUtilities.hxx"
#include "svtkQuadratureSchemeDefinition.h"

#include <sstream>
#include <vector>

using std::ostringstream;

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkQuadraturePointsGenerator);

//-----------------------------------------------------------------------------
svtkQuadraturePointsGenerator::svtkQuadraturePointsGenerator()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
svtkQuadraturePointsGenerator::~svtkQuadraturePointsGenerator() = default;

//-----------------------------------------------------------------------------
int svtkQuadraturePointsGenerator::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkQuadraturePointsGenerator::RequestData(
  svtkInformation*, svtkInformationVector** input, svtkInformationVector* output)
{
  svtkDataObject* tmpDataObj;
  // Get the input.
  tmpDataObj = input[0]->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT());
  svtkUnstructuredGrid* usgIn = svtkUnstructuredGrid::SafeDownCast(tmpDataObj);
  // Get the output.
  tmpDataObj = output->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT());
  svtkPolyData* pdOut = svtkPolyData::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (usgIn == nullptr || pdOut == nullptr || usgIn->GetNumberOfCells() == 0 ||
    usgIn->GetNumberOfPoints() == 0 || usgIn->GetCellData() == nullptr ||
    usgIn->GetCellData()->GetNumberOfArrays() == 0)
  {
    svtkErrorMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
  }

  // Generate points for the selected data array.
  // user specified the offsets array.
  this->Generate(usgIn, this->GetInputArrayToProcess(0, input), pdOut);

  return 1;
}

namespace
{

struct GenerateWorker
{
  template <typename OffsetArrayT>
  void operator()(OffsetArrayT* offsetArray, svtkDataArray* data, svtkUnstructuredGrid* usgIn,
    svtkPolyData* pdOut, std::vector<svtkQuadratureSchemeDefinition*>& dict)
  {
    const auto offsets = svtk::DataArrayValueRange<1>(offsetArray);

    const svtkIdType numCells = usgIn->GetNumberOfCells();
    const svtkIdType numVerts = pdOut->GetNumberOfPoints();
    svtkIdType previous = -1;

    bool shallowok = true;

    for (svtkIdType cellId = 0; cellId < numCells; cellId++)
    {
      svtkIdType offset = static_cast<svtkIdType>(offsets[cellId]);

      if (offset != previous + 1)
      {
        shallowok = false;
        break;
      }

      const int cellType = usgIn->GetCellType(cellId);
      if (dict[cellType] == nullptr)
      {
        previous = offset;
      }
      else
      {
        previous = offset + dict[cellType]->GetNumberOfQuadraturePoints();
      }
    }

    if ((previous + 1) != numVerts)
    {
      shallowok = false;
    }

    if (shallowok)
    {
      // ok, all the original cells are here, we can shallow copy the array
      // from input to output
      pdOut->GetPointData()->AddArray(data);
    }
    else
    {
      // in this case, we have to duplicate the valid tuples
      svtkDataArray* V_out = data->NewInstance();
      V_out->SetName(data->GetName());
      V_out->SetNumberOfComponents(data->GetNumberOfComponents());
      V_out->CopyComponentNames(data);
      for (svtkIdType cellId = 0; cellId < numCells; cellId++)
      {
        svtkIdType offset = static_cast<svtkIdType>(offsets[cellId]);
        const int cellType = usgIn->GetCellType(cellId);

        // a simple check to see if a scheme really exists for this cell type.
        // should not happen if the cell type has not been modified.
        if (dict[cellType] == nullptr)
        {
          continue;
        }

        int np = dict[cellType]->GetNumberOfQuadraturePoints();
        for (int id = 0; id < np; id++)
        {
          V_out->InsertNextTuple(offset + id, data);
        }
      }
      V_out->Squeeze();
      pdOut->GetPointData()->AddArray(V_out);
      V_out->Delete();
    }
  }
};

} // end anon namespace

// ----------------------------------------------------------------------------
int svtkQuadraturePointsGenerator::GenerateField(
  svtkUnstructuredGrid* usgIn, svtkDataArray* data, svtkDataArray* offsets, svtkPolyData* pdOut)
{
  svtkInformation* info = offsets->GetInformation();
  svtkInformationQuadratureSchemeDefinitionVectorKey* key =
    svtkQuadratureSchemeDefinition::DICTIONARY();
  if (!key->Has(info))
  {
    svtkErrorMacro(<< "Dictionary is not present in array " << offsets->GetName() << " " << offsets
                  << " Aborting.");
    return 0;
  }

  if (offsets->GetNumberOfComponents() != 1)
  {
    svtkErrorMacro("Expected offset array to only have a single component.");
    return 0;
  }

  int dictSize = key->Size(info);
  std::vector<svtkQuadratureSchemeDefinition*> dict(dictSize);
  key->GetRange(info, dict.data(), 0, 0, dictSize);

  // Use a fast path that assumes the offsets are integral:
  using svtkArrayDispatch::Integrals;
  using Dispatcher = svtkArrayDispatch::DispatchByValueType<Integrals>;

  GenerateWorker worker;
  if (!Dispatcher::Execute(offsets, worker, data, usgIn, pdOut, dict))
  { // Fallback to slow path for other arrays:
    worker(offsets, data, usgIn, pdOut, dict);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkQuadraturePointsGenerator::Generate(
  svtkUnstructuredGrid* usgIn, svtkDataArray* offsets, svtkPolyData* pdOut)
{
  if (usgIn == nullptr || offsets == nullptr || pdOut == nullptr)
  {
    svtkErrorMacro("configuration error");
    return 0;
  }

  if (offsets->GetNumberOfComponents() != 1)
  {
    svtkErrorMacro("Expected offsets array to have only a single component.");
    return 0;
  }

  // Strategy:
  // create the points then move the FieldData to PointData

  const char* offsetName = offsets->GetName();
  if (offsetName == nullptr)
  {
    svtkErrorMacro("offset array has no name, Skipping");
    return 1;
  }

  // get the dictionary
  svtkInformation* info = offsets->GetInformation();
  svtkInformationQuadratureSchemeDefinitionVectorKey* key =
    svtkQuadratureSchemeDefinition::DICTIONARY();
  if (!key->Has(info))
  {
    svtkErrorMacro(<< "Dictionary is not present in array " << offsets->GetName() << " Aborting.");
    return 0;
  }
  int dictSize = key->Size(info);
  std::vector<svtkQuadratureSchemeDefinition*> dict(dictSize);
  key->GetRange(info, dict.data(), 0, 0, dictSize);

  // Grab the point set.
  svtkDataArray* X = usgIn->GetPoints()->GetData();

  // Create the result array.
  svtkDoubleArray* qPts = svtkDoubleArray::New();
  svtkIdType nCells = usgIn->GetNumberOfCells();
  qPts->Allocate(3 * nCells); // Expect at least one point per cell
  qPts->SetNumberOfComponents(3);

  // For all cells interpolate.
  using Dispatcher = svtkArrayDispatch::Dispatch;
  svtkQuadraturePointsUtilities::InterpolateWorker worker;
  if (!Dispatcher::Execute(X, worker, usgIn, nCells, dict, qPts))
  { // fall back to slow path:
    worker(X, usgIn, nCells, dict, qPts);
  }

  // Add the interpolated quadrature points to the output
  svtkIdType nVerts = qPts->GetNumberOfTuples();
  svtkPoints* p = svtkPoints::New();
  p->SetDataTypeToDouble();
  p->SetData(qPts);
  qPts->Delete();
  pdOut->SetPoints(p);
  p->Delete();
  // Generate vertices at the quadrature points
  svtkIdTypeArray* va = svtkIdTypeArray::New();
  va->SetNumberOfTuples(2 * nVerts);
  svtkIdType* verts = va->GetPointer(0);
  for (int i = 0; i < nVerts; ++i)
  {
    verts[0] = 1;
    verts[1] = i;
    verts += 2;
  }
  svtkCellArray* cells = svtkCellArray::New();
  cells->AllocateExact(nVerts, va->GetNumberOfValues() - nVerts);
  cells->ImportLegacyFormat(va);
  pdOut->SetVerts(cells);
  cells->Delete();
  va->Delete();

  // then loop over all fields to map the field array to the points
  int nArrays = usgIn->GetFieldData()->GetNumberOfArrays();
  for (int i = 0; i < nArrays; ++i)
  {
    svtkDataArray* array = usgIn->GetFieldData()->GetArray(i);
    if (array == nullptr)
      continue;

    const char* arrayOffsetName =
      array->GetInformation()->Get(svtkQuadratureSchemeDefinition::QUADRATURE_OFFSET_ARRAY_NAME());
    if (arrayOffsetName == nullptr)
    {
      // not an error, since non-quadrature point field data may
      //  be present.
      svtkDebugMacro(<< "array " << array->GetName() << " has no offset array name, Skipping");
      continue;
    }

    if (strcmp(offsetName, arrayOffsetName) != 0)
    {
      // not an error, this array does not belong with the current
      // quadrature scheme definition.
      svtkDebugMacro(<< "array " << array->GetName()
                    << " has another offset array : " << arrayOffsetName << ", Skipping");
      continue;
    }

    this->GenerateField(usgIn, array, offsets, pdOut);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkQuadraturePointsGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
