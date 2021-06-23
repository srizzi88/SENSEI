/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkmTriangleMeshPointNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmTriangleMeshPointNormals.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/PolyDataConverter.h"

#include "svtkmFilterPolicy.h"

#include "svtkm/filter/SurfaceNormals.h"

namespace
{

struct InputFilterPolicy : public svtkmInputFilterPolicy
{
  using UnstructuredCellSetList =
    svtkm::List<tosvtkm::CellSetSingleType32Bit, tosvtkm::CellSetSingleType64Bit>;
};

}

svtkStandardNewMacro(svtkmTriangleMeshPointNormals);

//------------------------------------------------------------------------------
void svtkmTriangleMeshPointNormals::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkmTriangleMeshPointNormals::svtkmTriangleMeshPointNormals() = default;
svtkmTriangleMeshPointNormals::~svtkmTriangleMeshPointNormals() = default;

//------------------------------------------------------------------------------
int svtkmTriangleMeshPointNormals::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // check if polydata is in supported format
  if (input->GetVerts()->GetNumberOfCells() != 0 || input->GetLines()->GetNumberOfCells() != 0 ||
    input->GetStrips()->GetNumberOfCells() != 0 ||
    (input->GetPolys()->GetNumberOfConnectivityIds() % 3) != 0)
  {
    svtkErrorMacro(<< "This filter only works with polydata containing just triangles.");
    return 0;
  }

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::None);

    svtkm::filter::PolicyBase<InputFilterPolicy> policy;
    svtkm::filter::SurfaceNormals filter;
    filter.SetGenerateCellNormals(false);
    filter.SetNormalizeCellNormals(false);
    filter.SetGeneratePointNormals(true);
    filter.SetPointNormalsName("Normals");
    auto result = filter.Execute(in, policy);

    if (!fromsvtkm::Convert(result, output, input))
    {
      svtkErrorMacro(<< "Unable to convert SVTKm DataSet back to SVTK");
      return 0;
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkWarningMacro(<< "SVTK-m error: " << e.GetMessage()
                    << "Falling back to svtkTriangleMeshPointNormals");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  svtkSmartPointer<svtkDataArray> pointNormals = output->GetPointData()->GetArray("Normals");

  output->GetPointData()->CopyNormalsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff();
  output->GetCellData()->PassData(input->GetPointData());

  if (pointNormals)
  {
    output->GetPointData()->SetNormals(pointNormals);
  }

  return 1;
}
