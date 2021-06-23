/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQuadraturePointInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkQuadraturePointInterpolator.h"
#include "svtkQuadratureSchemeDefinition.h"

#include "svtkArrayDispatch.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkDataArray.h"
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

#include <sstream>
#include <vector>

using std::ostringstream;

#include "svtkQuadraturePointsUtilities.hxx"

svtkStandardNewMacro(svtkQuadraturePointInterpolator);

//-----------------------------------------------------------------------------
svtkQuadraturePointInterpolator::svtkQuadraturePointInterpolator()
{
  this->Clear();
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
svtkQuadraturePointInterpolator::~svtkQuadraturePointInterpolator()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
int svtkQuadraturePointInterpolator::FillInputPortInformation(int port, svtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUnstructuredGrid");
      break;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkQuadraturePointInterpolator::FillOutputPortInformation(int port, svtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUnstructuredGrid");
      break;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkQuadraturePointInterpolator::RequestData(
  svtkInformation*, svtkInformationVector** input, svtkInformationVector* output)
{
  svtkDataObject* tmpDataObj;
  // Get the inputs
  tmpDataObj = input[0]->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT());
  svtkUnstructuredGrid* usgIn = svtkUnstructuredGrid::SafeDownCast(tmpDataObj);
  // Get the outputs
  tmpDataObj = output->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT());
  svtkUnstructuredGrid* usgOut = svtkUnstructuredGrid::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (usgIn == nullptr || usgOut == nullptr || usgIn->GetNumberOfCells() == 0 ||
    usgIn->GetNumberOfPoints() == 0 || usgIn->GetPointData() == nullptr ||
    usgIn->GetPointData()->GetNumberOfArrays() == 0)
  {
    svtkWarningMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
  }

  // Copy the unstructured grid on the input
  usgOut->ShallowCopy(usgIn);

  // Interpolate the data arrays, but no points. Results
  // are stored in field data arrays.
  this->InterpolateFields(usgOut);

  return 1;
}

//-----------------------------------------------------------------------------
void svtkQuadraturePointInterpolator::Clear()
{
  // Nothing to do
}

//-----------------------------------------------------------------------------
int svtkQuadraturePointInterpolator::InterpolateFields(svtkUnstructuredGrid* usgOut)
{
  // Extract info we need for all cells.
  svtkIdType nCells = usgOut->GetNumberOfCells();

  // For each array we interpolate scalar data to the
  // integration point location. Results are in associated
  // field data arrays.
  int nArrays = usgOut->GetPointData()->GetNumberOfArrays();

  svtkDataArray* offsets = this->GetInputArrayToProcess(0, usgOut);
  if (offsets == nullptr)
  {
    svtkWarningMacro("no Offset array, skipping.");
    return 0;
  }

  if (offsets->GetNumberOfComponents() != 1)
  {
    svtkWarningMacro("expected Offset array to be single-component.");
    return 0;
  }

  const char* arrayOffsetName = offsets->GetName();

  svtkInformation* info = offsets->GetInformation();
  svtkInformationQuadratureSchemeDefinitionVectorKey* key =
    svtkQuadratureSchemeDefinition::DICTIONARY();
  if (!key->Has(info))
  {
    svtkWarningMacro("Dictionary is not present in.  Skipping.");
    return 0;
  }
  int dictSize = key->Size(info);
  std::vector<svtkQuadratureSchemeDefinition*> dict(dictSize);
  key->GetRange(info, dict.data(), 0, 0, dictSize);

  // interpolate the arrays
  for (int arrayId = 0; arrayId < nArrays; ++arrayId)
  {
    // Grab the next array
    svtkDataArray* V = usgOut->GetPointData()->GetArray(arrayId);

    // Use two arrays, one with the interpolated values,
    // the other with offsets to the start of each cell's
    // interpolated values.
    int nComps = V->GetNumberOfComponents();
    svtkDoubleArray* interpolated = svtkDoubleArray::New();
    interpolated->SetNumberOfComponents(nComps);
    interpolated->CopyComponentNames(V);
    interpolated->Allocate(nComps * nCells); // at least one qp per cell
    ostringstream interpolatedName;
    interpolatedName << V->GetName(); // << "_QP_Interpolated";
    interpolated->SetName(interpolatedName.str().c_str());
    usgOut->GetFieldData()->AddArray(interpolated);
    interpolated->GetInformation()->Set(
      svtkQuadratureSchemeDefinition::QUADRATURE_OFFSET_ARRAY_NAME(), arrayOffsetName);
    interpolated->Delete();

    // For all cells interpolate.
    // Don't restrict the value array's type, but only use the fast path for
    // integral offsets.
    using svtkArrayDispatch::AllTypes;
    using svtkArrayDispatch::Integrals;
    using Dispatcher = svtkArrayDispatch::Dispatch2ByValueType<AllTypes, Integrals>;

    svtkQuadraturePointsUtilities::InterpolateWorker worker;
    if (!Dispatcher::Execute(V, offsets, worker, usgOut, nCells, dict, interpolated))
    { // fall back to slow path:
      worker(V, offsets, usgOut, nCells, dict, interpolated);
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkQuadraturePointInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "No state." << endl;
}
