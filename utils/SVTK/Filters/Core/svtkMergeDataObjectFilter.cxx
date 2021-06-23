/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeDataObjectFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMergeDataObjectFilter.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkFieldData.h"
#include "svtkFieldDataToAttributeDataFilter.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkMergeDataObjectFilter);

//----------------------------------------------------------------------------
// Create object with no input or output.
svtkMergeDataObjectFilter::svtkMergeDataObjectFilter()
{
  this->OutputField = SVTK_DATA_OBJECT_FIELD;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkMergeDataObjectFilter::~svtkMergeDataObjectFilter() = default;

//----------------------------------------------------------------------------
// Specify a data object at a specified table location.
void svtkMergeDataObjectFilter::SetDataObjectInputData(svtkDataObject* d)
{
  this->SetInputData(1, d);
}

//----------------------------------------------------------------------------
// Get a pointer to a data object at a specified table location.
svtkDataObject* svtkMergeDataObjectFilter::GetDataObject()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
// Merge it all together
int svtkMergeDataObjectFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* dataObjectInfo = nullptr;
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    dataObjectInfo = inputVector[1]->GetInformationObject(0);
  }

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataObject* dataObject = nullptr;
  if (dataObjectInfo)
  {
    dataObject = dataObjectInfo->Get(svtkDataObject::DATA_OBJECT());
  }

  svtkFieldData* fd;

  svtkDebugMacro(<< "Merging dataset and data object");

  if (!dataObject)
  {
    svtkErrorMacro(<< "Data Object's Field Data is nullptr.");
    return 1;
  }

  fd = dataObject->GetFieldData();

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (this->OutputField == SVTK_CELL_DATA_FIELD)
  {
    int ncells = fd->GetNumberOfTuples();
    if (ncells != input->GetNumberOfCells())
    {
      svtkErrorMacro(<< "Field data size incompatible with number of cells");
      return 1;
    }
    for (int i = 0; i < fd->GetNumberOfArrays(); i++)
    {
      output->GetCellData()->AddArray(fd->GetArray(i));
    }
  }
  else if (this->OutputField == SVTK_POINT_DATA_FIELD)
  {
    int npts = fd->GetNumberOfTuples();
    if (npts != input->GetNumberOfPoints())
    {
      svtkErrorMacro(<< "Field data size incompatible with number of points");
      return 1;
    }
    for (int i = 0; i < fd->GetNumberOfArrays(); i++)
    {
      output->GetPointData()->AddArray(fd->GetArray(i));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkMergeDataObjectFilter::SetOutputFieldToDataObjectField()
{
  this->SetOutputField(SVTK_DATA_OBJECT_FIELD);
}

//----------------------------------------------------------------------------
void svtkMergeDataObjectFilter::SetOutputFieldToPointDataField()
{
  this->SetOutputField(SVTK_POINT_DATA_FIELD);
}

//----------------------------------------------------------------------------
void svtkMergeDataObjectFilter::SetOutputFieldToCellDataField()
{
  this->SetOutputField(SVTK_CELL_DATA_FIELD);
}

//----------------------------------------------------------------------------
int svtkMergeDataObjectFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    return this->Superclass::FillInputPortInformation(port, info);
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void svtkMergeDataObjectFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Output Field: ";
  if (this->OutputField == SVTK_DATA_OBJECT_FIELD)
  {
    os << "DataObjectField\n";
  }
  else if (this->OutputField == SVTK_POINT_DATA_FIELD)
  {
    os << "PointDataField\n";
  }
  else // if ( this->OutputField == SVTK_CELL_DATA_FIELD )
  {
    os << "CellDataField\n";
  }
}
