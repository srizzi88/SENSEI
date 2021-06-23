/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectToDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataObjectToDataSetFilter.h"

#include "svtkCellArray.h"
#include "svtkFieldData.h"
#include "svtkFieldDataToAttributeDataFilter.h"
#include "svtkInformation.h"
#include "svtkInformationExecutivePortKey.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkDataObjectToDataSetFilter);

//----------------------------------------------------------------------------
// Instantiate object with no input and no defined output.
svtkDataObjectToDataSetFilter::svtkDataObjectToDataSetFilter()
{
  int i;
  this->Updating = 0;

  this->DataSetType = SVTK_POLY_DATA;
  svtkPolyData* output = svtkPolyData::New();
  this->GetExecutive()->SetOutputData(0, output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

  for (i = 0; i < 3; i++)
  {
    this->PointArrays[i] = nullptr;
    this->PointArrayComponents[i] = -1; // uninitialized
    this->PointComponentRange[i][0] = this->PointComponentRange[i][1] = -1;
    this->PointNormalize[i] = 1; // yes, normalize
  }

  this->VertsArray = nullptr;
  this->VertsArrayComponent = -1;
  this->VertsComponentRange[0] = this->VertsComponentRange[1] = -1;

  this->LinesArray = nullptr;
  this->LinesArrayComponent = -1;
  this->LinesComponentRange[0] = this->LinesComponentRange[1] = -1;

  this->PolysArray = nullptr;
  this->PolysArrayComponent = -1;
  this->PolysComponentRange[0] = this->PolysComponentRange[1] = -1;

  this->StripsArray = nullptr;
  this->StripsArrayComponent = -1;
  this->StripsComponentRange[0] = this->StripsComponentRange[1] = -1;

  this->CellTypeArray = nullptr;
  this->CellTypeArrayComponent = -1;
  this->CellTypeComponentRange[0] = this->CellTypeComponentRange[1] = -1;

  this->CellConnectivityArray = nullptr;
  this->CellConnectivityArrayComponent = -1;
  this->CellConnectivityComponentRange[0] = this->CellConnectivityComponentRange[1] = -1;

  this->DefaultNormalize = 0;

  this->DimensionsArray = nullptr;
  ; // the name of the array
  this->DimensionsArrayComponent = -1;
  this->DimensionsComponentRange[0] = this->DimensionsComponentRange[1] = -1;

  this->SpacingArray = nullptr;
  ; // the name of the array
  this->SpacingArrayComponent = -1;
  this->SpacingComponentRange[0] = this->SpacingComponentRange[1] = -1;

  this->OriginArray = nullptr;
  ; // the name of the array
  this->OriginArrayComponent = -1;
  this->OriginComponentRange[0] = this->OriginComponentRange[1] = -1;

  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
}

//----------------------------------------------------------------------------
svtkDataObjectToDataSetFilter::~svtkDataObjectToDataSetFilter()
{
  for (int i = 0; i < 3; i++)
  {
    delete[] this->PointArrays[i];
  }
  delete[] this->VertsArray;
  delete[] this->LinesArray;
  delete[] this->PolysArray;
  delete[] this->StripsArray;
  delete[] this->CellTypeArray;
  delete[] this->CellConnectivityArray;
  delete[] this->DimensionsArray;
  delete[] this->SpacingArray;
  delete[] this->OriginArray;
}

void svtkDataObjectToDataSetFilter::SetDataSetType(int dt)
{
  if (dt == this->DataSetType)
  {
    return;
  }

  svtkDataSet* output;
  switch (dt)
  {
    case SVTK_POLY_DATA:
      output = svtkPolyData::New();
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      break;
    case SVTK_STRUCTURED_GRID:
      output = svtkStructuredGrid::New();
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      break;
    case SVTK_STRUCTURED_POINTS:
      output = svtkStructuredPoints::New();
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      break;
    case SVTK_UNSTRUCTURED_GRID:
      output = svtkUnstructuredGrid::New();
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      break;
    case SVTK_RECTILINEAR_GRID:
      output = svtkRectilinearGrid::New();
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      break;
    default:
      svtkWarningMacro("unknown type in SetDataSetType");
  }
  this->DataSetType = dt;
  this->Modified();
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataObjectToDataSetFilter::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(0, 0);
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkExecutive* inputExec = svtkExecutive::PRODUCER()->GetExecutive(inInfo);

  switch (this->DataSetType)
  {
    case SVTK_POLY_DATA:
      break;

    case SVTK_STRUCTURED_POINTS:
      // We need the array to get the dimensions
      inputExec->Update();
      this->ConstructDimensions(input);
      this->ConstructSpacing(input);
      this->ConstructOrigin(input);

      outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->Dimensions[0] - 1, 0,
        this->Dimensions[1] - 1, 0, this->Dimensions[2] - 1);
      outInfo->Set(svtkDataObject::ORIGIN(), this->Origin, 3);
      outInfo->Set(svtkDataObject::SPACING(), this->Spacing, 3);
      break;

    case SVTK_STRUCTURED_GRID:
      // We need the array to get the dimensions
      inputExec->Update();
      this->ConstructDimensions(input);
      outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->Dimensions[0] - 1, 0,
        this->Dimensions[1] - 1, 0, this->Dimensions[2] - 1);
      break;

    case SVTK_RECTILINEAR_GRID:
      // We need the array to get the dimensions
      inputExec->Update();
      this->ConstructDimensions(input);
      outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->Dimensions[0] - 1, 0,
        this->Dimensions[1] - 1, 0, this->Dimensions[2] - 1);
      break;

    case SVTK_UNSTRUCTURED_GRID:
      break;

    default:
      svtkErrorMacro(<< "Unsupported dataset type!");
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType npts;

  svtkDebugMacro(<< "Generating dataset from field data");

  switch (this->DataSetType)
  {
    case SVTK_POLY_DATA:
      if ((npts = this->ConstructPoints(input, svtkPolyData::SafeDownCast(output))))
      {
        this->ConstructCells(input, svtkPolyData::SafeDownCast(output));
      }
      else
      {
        svtkErrorMacro(<< "Couldn't create any points");
      }
      break;

    case SVTK_STRUCTURED_POINTS:
    {
      this->ConstructDimensions(input);
      this->ConstructSpacing(input);
      this->ConstructOrigin(input);
      svtkStructuredPoints* sp = svtkStructuredPoints::SafeDownCast(output);
      sp->SetDimensions(this->Dimensions);
      sp->SetOrigin(this->Origin);
      sp->SetSpacing(this->Spacing);
      break;
    }

    case SVTK_STRUCTURED_GRID:
      if ((npts = this->ConstructPoints(input, this->GetStructuredGridOutput())))
      {
        this->ConstructDimensions(input);
        if (npts == (this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2]))
        {
          svtkStructuredGrid* sg = svtkStructuredGrid::SafeDownCast(output);
          sg->SetDimensions(this->Dimensions);
        }
        else
        {
          svtkErrorMacro(<< "Number of points don't match dimensions");
        }
      }
      break;

    case SVTK_RECTILINEAR_GRID:
      if ((npts = this->ConstructPoints(input, this->GetRectilinearGridOutput())))
      {
        this->ConstructDimensions(input);
        if (npts == (this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2]))
        {
          svtkRectilinearGrid* rg = svtkRectilinearGrid::SafeDownCast(output);
          rg->SetDimensions(this->Dimensions);
        }
        else
        {
          svtkErrorMacro(<< "Number of points don't match dimensions");
        }
      }
      break;

    case SVTK_UNSTRUCTURED_GRID:
      if (this->ConstructPoints(input, svtkUnstructuredGrid::SafeDownCast(output)))
      {
        this->ConstructCells(input, svtkUnstructuredGrid::SafeDownCast(output));
      }
      else
      {
        svtkErrorMacro(<< "Couldn't create any points");
      }
      break;

    default:
      svtkErrorMacro(<< "Unsupported dataset type!");
  }

  svtkFieldData* inFD = input->GetFieldData();
  svtkFieldData* outFD = output->GetFieldData();
  outFD->CopyAllOn();
  outFD->PassData(inFD);

  return 1;
}

// Get the output as svtkPolyData.
svtkPolyData* svtkDataObjectToDataSetFilter::GetPolyDataOutput()
{
  return svtkPolyData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
svtkDataSet* svtkDataObjectToDataSetFilter::GetOutput()
{
  if (this->GetNumberOfOutputPorts() < 1)
  {
    return nullptr;
  }

  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetOutputData(0));
}

// Get the output as svtkStructuredPoints.
svtkStructuredPoints* svtkDataObjectToDataSetFilter::GetStructuredPointsOutput()
{
  return svtkStructuredPoints::SafeDownCast(this->GetOutput());
}

// Get the output as svtkStructuredGrid.
svtkStructuredGrid* svtkDataObjectToDataSetFilter::GetStructuredGridOutput()
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutput());
}

// Get the output as svtkUnstructuredGrid.
svtkUnstructuredGrid* svtkDataObjectToDataSetFilter::GetUnstructuredGridOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

// Get the output as svtkRectilinearGrid.
svtkRectilinearGrid* svtkDataObjectToDataSetFilter::GetRectilinearGridOutput()
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the info object
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Data Set Type: ";
  if (this->DataSetType == SVTK_POLY_DATA)
  {
    os << "svtkPolyData\n";
  }
  else if (this->DataSetType == SVTK_STRUCTURED_POINTS)
  {
    os << "svtkStructuredPoints\n";
  }
  else if (this->DataSetType == SVTK_STRUCTURED_GRID)
  {
    os << "svtkStructuredGrid\n";
  }
  else if (this->DataSetType == SVTK_RECTILINEAR_GRID)
  {
    os << "svtkRectilinearGrid\n";
  }
  else // if ( this->DataSetType == SVTK_UNSTRUCTURED_GRID )
  {
    os << "svtkUnstructuredGrid\n";
  }

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", " << this->Dimensions[1] << ", "
     << this->Dimensions[2] << ")\n";

  os << indent << "Spacing: (" << this->Spacing[0] << ", " << this->Spacing[1] << ", "
     << this->Spacing[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", " << this->Origin[1] << ", "
     << this->Origin[2] << ")\n";

  os << indent << "Default Normalize: " << (this->DefaultNormalize ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
// Stuff related to points --------------------------------------------
//
void svtkDataObjectToDataSetFilter::SetPointComponent(
  int comp, const char* arrayName, int arrayComp, int min, int max, int normalize)
{
  if (comp < 0 || comp > 2)
  {
    svtkErrorMacro(<< "Point component must be between (0,2)");
    return;
  }

  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->PointArrays[comp], arrayName);
  if (this->PointArrayComponents[comp] != arrayComp)
  {
    this->PointArrayComponents[comp] = arrayComp;
    this->Modified();
  }
  if (this->PointComponentRange[comp][0] != min)
  {
    this->PointComponentRange[comp][0] = min;
    this->Modified();
  }
  if (this->PointComponentRange[comp][1] != max)
  {
    this->PointComponentRange[comp][1] = max;
    this->Modified();
  }
  if (this->PointNormalize[comp] != normalize)
  {
    this->PointNormalize[comp] = normalize;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkDataObjectToDataSetFilter::GetPointComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointArrays[comp];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetPointComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointArrayComponents[comp];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetPointComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointComponentRange[comp][0];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetPointComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointComponentRange[comp][1];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetPointComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->PointNormalize[comp];
}

//----------------------------------------------------------------------------
svtkIdType svtkDataObjectToDataSetFilter::ConstructPoints(svtkDataObject* input, svtkPointSet* ps)
{
  int i, updated = 0;
  svtkDataArray* fieldArray[3];
  svtkIdType npts;
  svtkFieldData* fd = input->GetFieldData();

  for (i = 0; i < 3; i++)
  {
    fieldArray[i] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
      fd, this->PointArrays[i], this->PointArrayComponents[i]);

    if (fieldArray[i] == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested");
      return 0;
    }
    updated |= svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[i], this->PointComponentRange[i]);
  }

  npts = this->PointComponentRange[0][1] - this->PointComponentRange[0][0] + 1;
  if (npts != (this->PointComponentRange[1][1] - this->PointComponentRange[1][0] + 1) ||
    npts != (this->PointComponentRange[2][1] - this->PointComponentRange[2][0] + 1))
  {
    svtkErrorMacro(<< "Number of point components not consistent");
    return 0;
  }

  // Try using the arrays directly if possible; otherwise copy data
  svtkPoints* newPts = svtkPoints::New();
  if (fieldArray[0]->GetNumberOfComponents() == 3 && fieldArray[0] == fieldArray[1] &&
    fieldArray[1] == fieldArray[2] && fieldArray[0]->GetNumberOfTuples() == npts &&
    !this->PointNormalize[0] && !this->PointNormalize[1] && !this->PointNormalize[2])
  {
    newPts->SetData(fieldArray[0]);
  }
  else // have to copy data into created array
  {
    newPts->SetDataType(svtkFieldDataToAttributeDataFilter::GetComponentsType(3, fieldArray));
    newPts->SetNumberOfPoints(npts);

    for (i = 0; i < 3; i++)
    {
      if (svtkFieldDataToAttributeDataFilter::ConstructArray(newPts->GetData(), i, fieldArray[i],
            this->PointArrayComponents[i], this->PointComponentRange[i][0],
            this->PointComponentRange[i][1], this->PointNormalize[i]) == 0)
      {
        newPts->Delete();
        return 0;
      }
    }
  }

  ps->SetPoints(newPts);
  newPts->Delete();
  if (updated) // reset for next execution pass
  {
    for (i = 0; i < 3; i++)
    {
      this->PointComponentRange[i][0] = this->PointComponentRange[i][1] = -1;
    }
  }

  return npts;
}

//----------------------------------------------------------------------------
svtkIdType svtkDataObjectToDataSetFilter::ConstructPoints(
  svtkDataObject* input, svtkRectilinearGrid* rg)
{
  int i, nXpts, nYpts, nZpts, updated = 0;
  svtkIdType npts;
  svtkDataArray* fieldArray[3];
  svtkFieldData* fd = input->GetFieldData();

  for (i = 0; i < 3; i++)
  {
    fieldArray[i] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
      fd, this->PointArrays[i], this->PointArrayComponents[i]);

    if (fieldArray[i] == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested");
      return 0;
    }
  }

  for (i = 0; i < 3; i++)
  {
    updated |= svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[i], this->PointComponentRange[i]);
  }

  nXpts = this->PointComponentRange[0][1] - this->PointComponentRange[0][0] + 1;
  nYpts = this->PointComponentRange[1][1] - this->PointComponentRange[1][0] + 1;
  nZpts = this->PointComponentRange[2][1] - this->PointComponentRange[2][0] + 1;
  npts = nXpts * nYpts * nZpts;

  // Create the coordinate arrays
  svtkDataArray* XPts;
  svtkDataArray* YPts;
  svtkDataArray* ZPts;

  // Decide whether to use the field array or whether to copy data
  // First look at the x-coordinates
  if (fieldArray[0]->GetNumberOfComponents() == 1 && fieldArray[0]->GetNumberOfTuples() == nXpts &&
    !this->PointNormalize[0])
  {
    XPts = fieldArray[0];
    XPts->Register(this);
  }
  else // have to copy data into created array
  {
    XPts = svtkDataArray::CreateDataArray(
      svtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray));
    XPts->SetNumberOfComponents(1);
    XPts->SetNumberOfTuples(nXpts);

    if (svtkFieldDataToAttributeDataFilter::ConstructArray(XPts, 0, fieldArray[0],
          this->PointArrayComponents[0], this->PointComponentRange[0][0],
          this->PointComponentRange[0][1], this->PointNormalize[0]) == 0)
    {
      XPts->Delete();
      return 0;
    }
  }

  // Look at the y-coordinates
  if (fieldArray[1]->GetNumberOfComponents() == 1 && fieldArray[1]->GetNumberOfTuples() == nYpts &&
    !this->PointNormalize[1])
  {
    YPts = fieldArray[1];
    YPts->Register(this);
  }
  else // have to copy data into created array
  {
    YPts = svtkDataArray::CreateDataArray(
      svtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray + 1));
    YPts->SetNumberOfComponents(1);
    YPts->SetNumberOfTuples(nYpts);

    if (svtkFieldDataToAttributeDataFilter::ConstructArray(YPts, 0, fieldArray[1],
          this->PointArrayComponents[1], this->PointComponentRange[1][0],
          this->PointComponentRange[1][1], this->PointNormalize[1]) == 0)
    {
      XPts->Delete();
      YPts->Delete();
      return 0;
    }
  }

  // Look at the z-coordinates
  if (fieldArray[2]->GetNumberOfComponents() == 1 && fieldArray[2]->GetNumberOfTuples() == nZpts &&
    !this->PointNormalize[2])
  {
    ZPts = fieldArray[2];
    ZPts->Register(this);
  }
  else // have to copy data into created array
  {
    ZPts = svtkDataArray::CreateDataArray(
      svtkFieldDataToAttributeDataFilter::GetComponentsType(1, fieldArray + 2));
    ZPts->SetNumberOfComponents(1);
    ZPts->SetNumberOfTuples(nZpts);

    if (svtkFieldDataToAttributeDataFilter::ConstructArray(ZPts, 0, fieldArray[2],
          this->PointArrayComponents[2], this->PointComponentRange[2][0],
          this->PointComponentRange[2][1], this->PointNormalize[2]) == 0)
    {
      XPts->Delete();
      YPts->Delete();
      ZPts->Delete();
      return 0;
    }
  }

  rg->SetXCoordinates(XPts);
  rg->SetYCoordinates(YPts);
  rg->SetZCoordinates(ZPts);
  XPts->Delete();
  YPts->Delete();
  ZPts->Delete();

  if (updated) // reset for next execution pass
  {
    for (i = 0; i < 3; i++)
    {
      this->PointComponentRange[i][0] = this->PointComponentRange[i][1] = -1;
    }
  }

  return npts;
}

//----------------------------------------------------------------------------
// Stuff related to svtkPolyData --------------------------------------------
//
void svtkDataObjectToDataSetFilter::SetVertsComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->VertsArray, arrayName);
  if (this->VertsArrayComponent != arrayComp)
  {
    this->VertsArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->VertsComponentRange[0] != min)
  {
    this->VertsComponentRange[0] = min;
    this->Modified();
  }
  if (this->VertsComponentRange[1] != max)
  {
    this->VertsComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkDataObjectToDataSetFilter::GetVertsComponentArrayName()
{
  return this->VertsArray;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetVertsComponentArrayComponent()
{
  return this->VertsArrayComponent;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetVertsComponentMinRange()
{
  return this->VertsComponentRange[0];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetVertsComponentMaxRange()
{
  return this->VertsComponentRange[1];
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::SetLinesComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->LinesArray, arrayName);
  if (this->LinesArrayComponent != arrayComp)
  {
    this->LinesArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->LinesComponentRange[0] != min)
  {
    this->LinesComponentRange[0] = min;
    this->Modified();
  }
  if (this->LinesComponentRange[1] != max)
  {
    this->LinesComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkDataObjectToDataSetFilter::GetLinesComponentArrayName()
{
  return this->LinesArray;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetLinesComponentArrayComponent()
{
  return this->LinesArrayComponent;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetLinesComponentMinRange()
{
  return this->LinesComponentRange[0];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetLinesComponentMaxRange()
{
  return this->LinesComponentRange[1];
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::SetPolysComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->PolysArray, arrayName);
  if (this->PolysArrayComponent != arrayComp)
  {
    this->PolysArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->PolysComponentRange[0] != min)
  {
    this->PolysComponentRange[0] = min;
    this->Modified();
  }
  if (this->PolysComponentRange[1] != max)
  {
    this->PolysComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkDataObjectToDataSetFilter::GetPolysComponentArrayName()
{
  return this->PolysArray;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetPolysComponentArrayComponent()
{
  return this->PolysArrayComponent;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetPolysComponentMinRange()
{
  return this->PolysComponentRange[0];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetPolysComponentMaxRange()
{
  return this->PolysComponentRange[1];
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::SetStripsComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->StripsArray, arrayName);
  if (this->StripsArrayComponent != arrayComp)
  {
    this->StripsArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->StripsComponentRange[0] != min)
  {
    this->StripsComponentRange[0] = min;
    this->Modified();
  }
  if (this->StripsComponentRange[1] != max)
  {
    this->StripsComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkDataObjectToDataSetFilter::GetStripsComponentArrayName()
{
  return this->StripsArray;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetStripsComponentArrayComponent()
{
  return this->StripsArrayComponent;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetStripsComponentMinRange()
{
  return this->StripsComponentRange[0];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetStripsComponentMaxRange()
{
  return this->StripsComponentRange[1];
}

//----------------------------------------------------------------------------
// Stuff related to svtkUnstructuredGrid --------------------------------------
void svtkDataObjectToDataSetFilter::SetCellTypeComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->CellTypeArray, arrayName);
  if (this->CellTypeArrayComponent != arrayComp)
  {
    this->CellTypeArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->CellTypeComponentRange[0] != min)
  {
    this->CellTypeComponentRange[0] = min;
    this->Modified();
  }
  if (this->CellTypeComponentRange[1] != max)
  {
    this->CellTypeComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkDataObjectToDataSetFilter::GetCellTypeComponentArrayName()
{
  return this->CellTypeArray;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetCellTypeComponentArrayComponent()
{
  return this->CellTypeArrayComponent;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetCellTypeComponentMinRange()
{
  return this->CellTypeComponentRange[0];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetCellTypeComponentMaxRange()
{
  return this->CellTypeComponentRange[1];
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::SetCellConnectivityComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->CellConnectivityArray, arrayName);
  if (this->CellConnectivityArrayComponent != arrayComp)
  {
    this->CellConnectivityArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->CellConnectivityComponentRange[0] != min)
  {
    this->CellConnectivityComponentRange[0] = min;
    this->Modified();
  }
  if (this->CellConnectivityComponentRange[1] != max)
  {
    this->CellConnectivityComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkDataObjectToDataSetFilter::GetCellConnectivityComponentArrayName()
{
  return this->CellConnectivityArray;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetCellConnectivityComponentArrayComponent()
{
  return this->CellConnectivityArrayComponent;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetCellConnectivityComponentMinRange()
{
  return this->CellConnectivityComponentRange[0];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::GetCellConnectivityComponentMaxRange()
{
  return this->CellConnectivityComponentRange[1];
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::ConstructCells(svtkDataObject* input, svtkPolyData* pd)
{
  svtkIdType ncells = 0;
  svtkDataArray* fieldArray[4];
  svtkFieldData* fd = input->GetFieldData();

  fieldArray[0] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->VertsArray, this->VertsArrayComponent);
  if (this->VertsArray && fieldArray[0] == nullptr)
  {
    svtkErrorMacro(<< "Can't find array requested for vertices");
    return 0;
  }

  fieldArray[1] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->LinesArray, this->LinesArrayComponent);
  if (this->LinesArray && fieldArray[1] == nullptr)
  {
    svtkErrorMacro(<< "Can't find array requested for lines");
    return 0;
  }

  fieldArray[2] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->PolysArray, this->PolysArrayComponent);
  if (this->PolysArray && fieldArray[2] == nullptr)
  {
    svtkErrorMacro(<< "Can't find array requested for polygons");
    return 0;
  }

  fieldArray[3] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->StripsArray, this->StripsArrayComponent);
  if (this->StripsArray && fieldArray[3] == nullptr)
  {
    svtkErrorMacro(<< "Can't find array requested for triangle strips");
    return 0;
  }

  if (fieldArray[0])
  {
    svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[0], this->VertsComponentRange);
    svtkCellArray* verts =
      this->ConstructCellArray(fieldArray[0], this->VertsArrayComponent, this->VertsComponentRange);
    if (verts != nullptr)
    {
      pd->SetVerts(verts);
      ncells += verts->GetNumberOfCells();
      verts->Delete();
    }
    this->VertsComponentRange[0] = this->VertsComponentRange[1] = -1;
  }

  if (fieldArray[1])
  {
    svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[1], this->LinesComponentRange);
    svtkCellArray* lines =
      this->ConstructCellArray(fieldArray[1], this->LinesArrayComponent, this->LinesComponentRange);
    if (lines != nullptr)
    {
      pd->SetLines(lines);
      ncells += lines->GetNumberOfCells();
      lines->Delete();
    }
    this->LinesComponentRange[0] = this->LinesComponentRange[1] = -1;
  }

  if (fieldArray[2])
  {
    svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[2], this->PolysComponentRange);
    svtkCellArray* polys =
      this->ConstructCellArray(fieldArray[2], this->PolysArrayComponent, this->PolysComponentRange);
    if (polys != nullptr)
    {
      pd->SetPolys(polys);
      ncells += polys->GetNumberOfCells();
      polys->Delete();
    }
    this->PolysComponentRange[0] = this->PolysComponentRange[1] = -1;
  }

  if (fieldArray[3])
  {
    svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray[3], this->StripsComponentRange);
    svtkCellArray* triStrips = this->ConstructCellArray(
      fieldArray[3], this->StripsArrayComponent, this->StripsComponentRange);
    if (triStrips != nullptr)
    {
      pd->SetStrips(triStrips);
      ncells += triStrips->GetNumberOfCells();
      triStrips->Delete();
    }
    this->StripsComponentRange[0] = this->StripsComponentRange[1] = -1;
  }

  return ncells;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::ConstructCells(svtkDataObject* input, svtkUnstructuredGrid* ug)
{
  int i, *types, typesAllocated = 0;
  svtkDataArray* fieldArray[2];
  int ncells;
  svtkFieldData* fd = input->GetFieldData();

  fieldArray[0] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->CellTypeArray, this->CellTypeArrayComponent);

  if (fieldArray[0] == nullptr)
  {
    svtkErrorMacro(<< "Can't find array requested for cell types");
    return 0;
  }

  svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
    fieldArray[0], this->CellTypeComponentRange);
  ncells = this->CellTypeComponentRange[1] - this->CellTypeComponentRange[0] + 1;

  fieldArray[1] = svtkFieldDataToAttributeDataFilter::GetFieldArray(
    fd, this->CellConnectivityArray, this->CellConnectivityArrayComponent);
  if (fieldArray[1] == nullptr)
  {
    svtkErrorMacro(<< "Can't find array requested for cell connectivity");
    return 0;
  }

  // Okay, let's piece it together
  if (fieldArray[0]) // cell types defined
  {
    // first we create the integer array of types
    svtkDataArray* da = fieldArray[0];

    if (da->GetDataType() == SVTK_INT && da->GetNumberOfComponents() == 1 &&
      this->CellTypeArrayComponent == 0 && this->CellTypeComponentRange[0] == 0 &&
      this->CellTypeComponentRange[1] == da->GetMaxId())
    {
      types = static_cast<svtkIntArray*>(da)->GetPointer(0);
    }
    // Otherwise, we'll copy the data by inserting it into a svtkCellArray
    else
    {
      typesAllocated = 1;
      types = new int[ncells];
      for (i = this->CellTypeComponentRange[0]; i <= this->CellTypeComponentRange[1]; i++)
      {
        types[i] = static_cast<int>(da->GetComponent(i, this->CellTypeArrayComponent));
      }
    }
    this->CellTypeComponentRange[0] = this->CellTypeComponentRange[1] = -1;

    // create connectivity
    if (fieldArray[1]) // cell connectivity defined
    {
      svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
        fieldArray[1], this->CellConnectivityComponentRange);
      svtkCellArray* carray = this->ConstructCellArray(
        fieldArray[1], this->CellConnectivityArrayComponent, this->CellConnectivityComponentRange);
      if (carray != nullptr) // insert into unstructured grid
      {
        ug->SetCells(types, carray);
        carray->Delete();
      }
      this->CellConnectivityComponentRange[0] = this->CellConnectivityComponentRange[1] = -1;
    }
    if (typesAllocated)
    {
      delete[] types;
    }
  }

  return ncells;
}

//----------------------------------------------------------------------------
svtkCellArray* svtkDataObjectToDataSetFilter::ConstructCellArray(
  svtkDataArray* da, int comp, svtkIdType compRange[2])
{
  int j, min, max, numComp = da->GetNumberOfComponents();
  svtkCellArray* carray;
  svtkIdType npts, ncells, i;

  min = 0;
  max = da->GetMaxId();

  if (comp < 0 || comp >= numComp)
  {
    svtkErrorMacro(<< "Bad component specification");
    return nullptr;
  }

  carray = svtkCellArray::New();

  // If the data type is svtkIdType, and the number of components is 1, then
  // we can directly use the data array without copying it. We just have to
  // figure out how many cells we have.
  if (da->GetDataType() == SVTK_ID_TYPE && da->GetNumberOfComponents() == 1 && comp == 0 &&
    compRange[0] == 0 && compRange[1] == max)
  {
    svtkIdTypeArray* ia = static_cast<svtkIdTypeArray*>(da);
    for (ncells = i = 0; i < ia->GetMaxId(); i += (npts + 1))
    {
      ncells++;
      npts = ia->GetValue(i);
    }
    carray->AllocateExact(ncells, ia->GetNumberOfValues() - ncells);
    carray->ImportLegacyFormat(ia);
  }
  // Otherwise, we'll copy the data by inserting it into a svtkCellArray
  else
  {
    for (i = min; i < max; i += (npts + 1))
    {
      npts = static_cast<int>(da->GetComponent(i, comp));
      if (npts <= 0)
      {
        svtkErrorMacro(<< "Error constructing cell array");
        carray->Delete();
        return nullptr;
      }
      else
      {
        carray->InsertNextCell(npts);
        for (j = 1; j <= npts; j++)
        {
          carray->InsertCellPoint(static_cast<int>(da->GetComponent(i + j, comp)));
        }
      }
    }
  }

  return carray;
}

//----------------------------------------------------------------------------
// Alternative methods for Dimensions, Spacing, and Origin -------------------
//
void svtkDataObjectToDataSetFilter::SetDimensionsComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->DimensionsArray, arrayName);
  if (this->DimensionsArrayComponent != arrayComp)
  {
    this->DimensionsArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->DimensionsComponentRange[0] != min)
  {
    this->DimensionsComponentRange[0] = min;
    this->Modified();
  }
  if (this->DimensionsComponentRange[1] != max)
  {
    this->DimensionsComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::SetSpacingComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->SpacingArray, arrayName);
  if (this->SpacingArrayComponent != arrayComp)
  {
    this->SpacingArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->SpacingComponentRange[0] != min)
  {
    this->SpacingComponentRange[0] = min;
    this->Modified();
  }
  if (this->SpacingComponentRange[1] != max)
  {
    this->SpacingComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::SetOriginComponent(
  const char* arrayName, int arrayComp, int min, int max)
{
  svtkFieldDataToAttributeDataFilter::SetArrayName(this, this->OriginArray, arrayName);
  if (this->OriginArrayComponent != arrayComp)
  {
    this->OriginArrayComponent = arrayComp;
    this->Modified();
  }
  if (this->OriginComponentRange[0] != min)
  {
    this->OriginComponentRange[0] = min;
    this->Modified();
  }
  if (this->OriginComponentRange[1] != max)
  {
    this->OriginComponentRange[1] = max;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::ConstructDimensions(svtkDataObject* input)
{
  if (this->DimensionsArray == nullptr || this->DimensionsArrayComponent < 0)
  {
    return; // assume dimensions have been set
  }
  else
  {
    svtkFieldData* fd = input->GetFieldData();
    svtkDataArray* fieldArray = svtkFieldDataToAttributeDataFilter::GetFieldArray(
      fd, this->DimensionsArray, this->DimensionsArrayComponent);
    if (fieldArray == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested for dimensions");
      return;
    }

    svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray, this->DimensionsComponentRange);

    for (int i = 0; i < 3; i++)
    {
      this->Dimensions[i] = static_cast<int>(fieldArray->GetComponent(
        this->DimensionsComponentRange[0] + i, this->DimensionsArrayComponent));
    }
  }

  this->DimensionsComponentRange[0] = this->DimensionsComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::ConstructSpacing(svtkDataObject* input)
{
  if (this->SpacingArray == nullptr || this->SpacingArrayComponent < 0)
  {
    return; // assume Spacing have been set
  }
  else
  {
    svtkFieldData* fd = input->GetFieldData();
    svtkDataArray* fieldArray = svtkFieldDataToAttributeDataFilter::GetFieldArray(
      fd, this->SpacingArray, this->SpacingArrayComponent);
    if (fieldArray == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested for Spacing");
      return;
    }

    svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
      fieldArray, this->SpacingComponentRange);

    for (int i = 0; i < 3; i++)
    {
      this->Spacing[i] =
        fieldArray->GetComponent(this->SpacingComponentRange[0] + i, this->SpacingArrayComponent);
    }
  }
  this->SpacingComponentRange[0] = this->SpacingComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
svtkDataSet* svtkDataObjectToDataSetFilter::GetOutput(int idx)
{
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetOutputData(idx));
}

//----------------------------------------------------------------------------
void svtkDataObjectToDataSetFilter::ConstructOrigin(svtkDataObject* input)
{
  if (this->OriginArray == nullptr || this->OriginArrayComponent < 0)
  {
    return; // assume Origin have been set
  }
  else
  {
    svtkFieldData* fd = input->GetFieldData();
    svtkDataArray* fieldArray = svtkFieldDataToAttributeDataFilter::GetFieldArray(
      fd, this->OriginArray, this->OriginArrayComponent);
    if (fieldArray == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested for Origin");
      return;
    }

    svtkFieldDataToAttributeDataFilter::UpdateComponentRange(fieldArray, this->OriginComponentRange);

    for (int i = 0; i < 3; i++)
    {
      this->Origin[i] =
        fieldArray->GetComponent(this->OriginComponentRange[0] + i, this->OriginArrayComponent);
    }
  }

  this->OriginComponentRange[0] = this->OriginComponentRange[1] = -1;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDataObjectToDataSetFilter::RequestDataObject(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output || (output->GetDataObjectType() != this->DataSetType))
  {
    switch (this->DataSetType)
    {
      case SVTK_POLY_DATA:
        output = svtkPolyData::New();
        break;
      case SVTK_STRUCTURED_GRID:
        output = svtkStructuredGrid::New();
        break;
      case SVTK_STRUCTURED_POINTS:
        output = svtkStructuredPoints::New();
        break;
      case SVTK_UNSTRUCTURED_GRID:
        output = svtkUnstructuredGrid::New();
        break;
      case SVTK_RECTILINEAR_GRID:
        output = svtkRectilinearGrid::New();
        break;
      default:
        svtkWarningMacro("unknown DataSetType");
    }
    if (output)
    {
      outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
      output->Delete();
    }
  }
  return 1;
}
