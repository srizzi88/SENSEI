/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCutter.h"

#include "svtk3DLinearGridPlaneCutter.h"
#include "svtkArrayDispatch.h"
#include "svtkAssume.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkContourHelper.h"
#include "svtkContourValues.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkEventForwarderCommand.h"
#include "svtkFloatArray.h"
#include "svtkGenericCell.h"
#include "svtkGridSynchronizedTemplates3D.h"
#include "svtkImageData.h"
#include "svtkImplicitFunction.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearSynchronizedTemplates.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkSynchronizedTemplates3D.h"
#include "svtkSynchronizedTemplatesCutter3D.h"
#include "svtkTimerLog.h"
#include "svtkUnstructuredGridBase.h"

#include <algorithm>
#include <cmath>

svtkStandardNewMacro(svtkCutter);
svtkCxxSetObjectMacro(svtkCutter, CutFunction, svtkImplicitFunction);
svtkCxxSetObjectMacro(svtkCutter, Locator, svtkIncrementalPointLocator);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; initial value of 0.0; and
// generating cut scalars turned off.
svtkCutter::svtkCutter(svtkImplicitFunction* cf)
{
  this->ContourValues = svtkContourValues::New();
  this->SortBy = SVTK_SORT_BY_VALUE;
  this->CutFunction = cf;
  this->GenerateCutScalars = 0;
  this->Locator = nullptr;
  this->GenerateTriangles = 1;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->SynchronizedTemplates3D = svtkSynchronizedTemplates3D::New();
  this->SynchronizedTemplatesCutter3D = svtkSynchronizedTemplatesCutter3D::New();
  this->GridSynchronizedTemplates = svtkGridSynchronizedTemplates3D::New();
  this->RectilinearSynchronizedTemplates = svtkRectilinearSynchronizedTemplates::New();
}

//----------------------------------------------------------------------------
svtkCutter::~svtkCutter()
{
  this->ContourValues->Delete();
  this->SetCutFunction(nullptr);
  this->SetLocator(nullptr);

  this->SynchronizedTemplates3D->Delete();
  this->SynchronizedTemplatesCutter3D->Delete();
  this->GridSynchronizedTemplates->Delete();
  this->RectilinearSynchronizedTemplates->Delete();
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
svtkMTimeType svtkCutter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType contourValuesMTime = this->ContourValues->GetMTime();
  svtkMTimeType time;

  mTime = (contourValuesMTime > mTime ? contourValuesMTime : mTime);

  if (this->CutFunction != nullptr)
  {
    time = this->CutFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkCutter::StructuredPointsCutter(svtkDataSet* dataSetInput, svtkPolyData* thisOutput,
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkImageData* input = svtkImageData::SafeDownCast(dataSetInput);
  svtkPolyData* output;
  svtkIdType numPts = input->GetNumberOfPoints();

  if (numPts < 1)
  {
    return;
  }

  svtkIdType numContours = this->GetNumberOfContours();

  // for one contour we use the SyncTempCutter which is faster and has a
  // smaller memory footprint
  if (numContours == 1)
  {
    this->SynchronizedTemplatesCutter3D->SetCutFunction(this->CutFunction);
    this->SynchronizedTemplatesCutter3D->SetValue(0, this->GetValue(0));
    this->SynchronizedTemplatesCutter3D->SetGenerateTriangles(this->GetGenerateTriangles());
    this->SynchronizedTemplatesCutter3D->ProcessRequest(request, inputVector, outputVector);
    return;
  }

  // otherwise compute scalar data then contour
  svtkFloatArray* cutScalars = svtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  svtkImageData* contourData = svtkImageData::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
  {
    contourData->GetPointData()->SetScalars(cutScalars);
  }
  else
  {
    contourData->GetPointData()->AddArray(cutScalars);
  }

  double scalar;
  double x[3];
  for (svtkIdType i = 0; i < numPts; i++)
  {
    input->GetPoint(i, x);
    scalar = this->CutFunction->FunctionValue(x);
    cutScalars->SetComponent(i, 0, scalar);
  }

  this->SynchronizedTemplates3D->SetInputData(contourData);
  this->SynchronizedTemplates3D->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "cutScalars");
  this->SynchronizedTemplates3D->SetNumberOfContours(numContours);
  for (int i = 0; i < numContours; i++)
  {
    this->SynchronizedTemplates3D->SetValue(i, this->GetValue(i));
  }
  this->SynchronizedTemplates3D->ComputeScalarsOff();
  this->SynchronizedTemplates3D->ComputeNormalsOff();
  output = this->SynchronizedTemplates3D->GetOutput();
  this->SynchronizedTemplatesCutter3D->SetGenerateTriangles(this->GetGenerateTriangles());
  this->SynchronizedTemplates3D->Update();
  output->Register(this);

  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  thisOutput->GetCellData()->ShallowCopy(output->GetCellData());
  output->UnRegister(this);

  cutScalars->Delete();
  contourData->Delete();
}

//----------------------------------------------------------------------------
void svtkCutter::StructuredGridCutter(svtkDataSet* dataSetInput, svtkPolyData* thisOutput)
{
  svtkStructuredGrid* input = svtkStructuredGrid::SafeDownCast(dataSetInput);
  svtkPolyData* output;
  svtkIdType numPts = input->GetNumberOfPoints();

  if (numPts < 1)
  {
    return;
  }

  svtkFloatArray* cutScalars = svtkFloatArray::New();
  cutScalars->SetName("cutScalars");
  cutScalars->SetNumberOfTuples(numPts);

  svtkStructuredGrid* contourData = svtkStructuredGrid::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
  {
    contourData->GetPointData()->SetScalars(cutScalars);
  }
  else
  {
    contourData->GetPointData()->AddArray(cutScalars);
  }

  svtkDataArray* dataArrayInput = input->GetPoints()->GetData();
  this->CutFunction->FunctionValue(dataArrayInput, cutScalars);
  svtkIdType numContours = this->GetNumberOfContours();

  this->GridSynchronizedTemplates->SetDebug(this->GetDebug());
  this->GridSynchronizedTemplates->SetOutputPointsPrecision(this->OutputPointsPrecision);
  this->GridSynchronizedTemplates->SetInputData(contourData);
  this->GridSynchronizedTemplates->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "cutScalars");
  this->GridSynchronizedTemplates->SetNumberOfContours(numContours);
  for (int i = 0; i < numContours; i++)
  {
    this->GridSynchronizedTemplates->SetValue(i, this->GetValue(i));
  }
  this->GridSynchronizedTemplates->ComputeScalarsOff();
  this->GridSynchronizedTemplates->ComputeNormalsOff();
  this->GridSynchronizedTemplates->SetGenerateTriangles(this->GetGenerateTriangles());
  output = this->GridSynchronizedTemplates->GetOutput();
  this->GridSynchronizedTemplates->Update();
  output->Register(this);

  thisOutput->ShallowCopy(output);
  output->UnRegister(this);

  cutScalars->Delete();
  contourData->Delete();
}

//----------------------------------------------------------------------------
void svtkCutter::RectilinearGridCutter(svtkDataSet* dataSetInput, svtkPolyData* thisOutput)
{
  svtkRectilinearGrid* input = svtkRectilinearGrid::SafeDownCast(dataSetInput);
  svtkPolyData* output;
  svtkIdType numPts = input->GetNumberOfPoints();

  if (numPts < 1)
  {
    return;
  }

  svtkFloatArray* cutScalars = svtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  svtkRectilinearGrid* contourData = svtkRectilinearGrid::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
  {
    contourData->GetPointData()->SetScalars(cutScalars);
  }
  else
  {
    contourData->GetPointData()->AddArray(cutScalars);
  }

  for (svtkIdType i = 0; i < numPts; i++)
  {
    double x[3];
    input->GetPoint(i, x);
    double scalar = this->CutFunction->FunctionValue(x);
    cutScalars->SetComponent(i, 0, scalar);
  }
  svtkIdType numContours = this->GetNumberOfContours();

  this->RectilinearSynchronizedTemplates->SetInputData(contourData);
  this->RectilinearSynchronizedTemplates->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "cutScalars");
  this->RectilinearSynchronizedTemplates->SetNumberOfContours(numContours);
  for (int i = 0; i < numContours; i++)
  {
    this->RectilinearSynchronizedTemplates->SetValue(i, this->GetValue(i));
  }
  this->RectilinearSynchronizedTemplates->ComputeScalarsOff();
  this->RectilinearSynchronizedTemplates->ComputeNormalsOff();
  this->RectilinearSynchronizedTemplates->SetGenerateTriangles(this->GenerateTriangles);
  output = this->RectilinearSynchronizedTemplates->GetOutput();
  this->RectilinearSynchronizedTemplates->Update();
  output->Register(this);

  thisOutput->ShallowCopy(output);
  output->UnRegister(this);

  cutScalars->Delete();
  contourData->Delete();
}

namespace
{
//----------------------------------------------------------------------------
// Find the first visible cell in a svtkStructuredGrid.
//
svtkIdType GetFirstVisibleCell(svtkDataSet* DataSetInput)
{
  svtkStructuredGrid* input = svtkStructuredGrid::SafeDownCast(DataSetInput);
  if (input)
  {
    if (input->HasAnyBlankCells())
    {
      svtkIdType size = input->GetNumberOfElements(svtkDataSet::CELL);
      for (svtkIdType i = 0; i < size; ++i)
      {
        if (input->IsCellVisible(i) != 0)
        {
          return i;
        }
      }
    }
  }
  return 0;
}
}

//----------------------------------------------------------------------------
// Cut through data generating surface.
//
int svtkCutter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Executing cutter");
  if (!this->CutFunction)
  {
    svtkErrorMacro("No cut function specified");
    return 0;
  }

  if (!input)
  {
    // this could be a table in a multiblock structure, i.e. no cut!
    return 0;
  }

  if (input->GetNumberOfPoints() < 1 || this->GetNumberOfContours() < 1)
  {
    return 1;
  }

#ifdef TIMEME
  svtkSmartPointer<svtkTimerLog> timer = svtkSmartPointer<svtkTimerLog>::New();
  timer->StartTimer();
#endif

  if ((input->GetDataObjectType() == SVTK_STRUCTURED_POINTS ||
        input->GetDataObjectType() == SVTK_IMAGE_DATA) &&
    input->GetCell(0) && input->GetCell(0)->GetCellDimension() >= 3)
  {
    this->StructuredPointsCutter(input, output, request, inputVector, outputVector);
  }
  else if (input->GetDataObjectType() == SVTK_STRUCTURED_GRID && input->GetCell(0) &&
    input->GetCell(GetFirstVisibleCell(input))->GetCellDimension() >= 3)
  {
    this->StructuredGridCutter(input, output);
  }
  else if (input->GetDataObjectType() == SVTK_RECTILINEAR_GRID &&
    static_cast<svtkRectilinearGrid*>(input)->GetDataDimension() == 3)
  {
    this->RectilinearGridCutter(input, output);
  }
  else if (input->GetDataObjectType() == SVTK_UNSTRUCTURED_GRID_BASE ||
    input->GetDataObjectType() == SVTK_UNSTRUCTURED_GRID)
  {
    // See if the input can be fully processed by the fast svtk3DLinearGridPlaneCutter.
    // This algorithm can provide a substantial speed improvement over the more general
    // algorithm for svtkUnstructuredGrids.
    if (this->GetGenerateTriangles() && this->GetCutFunction() &&
      this->GetCutFunction()->IsA("svtkPlane") && this->GetNumberOfContours() == 1 &&
      this->GetGenerateCutScalars() == 0 &&
      (input->GetCellData() && input->GetCellData()->GetNumberOfArrays() == 0) &&
      svtk3DLinearGridPlaneCutter::CanFullyProcessDataObject(input))
    {
      svtkNew<svtk3DLinearGridPlaneCutter> linear3DCutter;

      // Create a copy of svtkPlane and nudge it by the single contour
      svtkPlane* plane = svtkPlane::SafeDownCast(this->GetCutFunction());
      svtkNew<svtkPlane> newPlane;
      newPlane->SetNormal(plane->GetNormal());
      newPlane->SetOrigin(plane->GetOrigin());

      // Evaluate the distance the origin is from the original plane. This accomodates
      // subclasses of svtkPlane that may have an additional offset parameter not
      // accessible through the svtkPlane interface. Use this distance to adjust the origin
      // in newPlane.
      double d = plane->EvaluateFunction(plane->GetOrigin());

      // In addition. We'll need to shift by the contour value.
      newPlane->Push(-d + this->GetValue(0));

      linear3DCutter->SetPlane(newPlane);
      linear3DCutter->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
      linear3DCutter->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
      svtkNew<svtkEventForwarderCommand> progressForwarder;
      progressForwarder->SetTarget(this);
      linear3DCutter->AddObserver(svtkCommand::ProgressEvent, progressForwarder);

      int retval = linear3DCutter->ProcessRequest(request, inputVector, outputVector);

      return retval;
    }

    svtkDebugMacro(<< "Executing Unstructured Grid Cutter");
    this->UnstructuredGridCutter(input, output);
  }
  else
  {
    svtkDebugMacro(<< "Executing DataSet Cutter");
    this->DataSetCutter(input, output);
  }

#ifdef TIMEME
  timer->StopTimer();
  cout << "Sliced " << output->GetNumberOfCells() << " cells in " << timer->GetElapsedTime()
       << " secs " << endl;
#endif
  return 1;
}

//----------------------------------------------------------------------------
void svtkCutter::GetCellTypeDimensions(unsigned char* cellTypeDimensions)
{
  // Assume most cells will be 3d.
  memset(cellTypeDimensions, 3, SVTK_NUMBER_OF_CELL_TYPES);
  cellTypeDimensions[SVTK_EMPTY_CELL] = 0;
  cellTypeDimensions[SVTK_VERTEX] = 0;
  cellTypeDimensions[SVTK_POLY_VERTEX] = 0;
  cellTypeDimensions[SVTK_LINE] = 1;
  cellTypeDimensions[SVTK_CUBIC_LINE] = 1;
  cellTypeDimensions[SVTK_POLY_LINE] = 1;
  cellTypeDimensions[SVTK_QUADRATIC_EDGE] = 1;
  cellTypeDimensions[SVTK_PARAMETRIC_CURVE] = 1;
  cellTypeDimensions[SVTK_HIGHER_ORDER_EDGE] = 1;
  cellTypeDimensions[SVTK_LAGRANGE_CURVE] = 1;
  cellTypeDimensions[SVTK_BEZIER_CURVE] = 1;
  cellTypeDimensions[SVTK_TRIANGLE] = 2;
  cellTypeDimensions[SVTK_TRIANGLE_STRIP] = 2;
  cellTypeDimensions[SVTK_POLYGON] = 2;
  cellTypeDimensions[SVTK_PIXEL] = 2;
  cellTypeDimensions[SVTK_QUAD] = 2;
  cellTypeDimensions[SVTK_QUADRATIC_TRIANGLE] = 2;
  cellTypeDimensions[SVTK_BIQUADRATIC_TRIANGLE] = 2;
  cellTypeDimensions[SVTK_QUADRATIC_QUAD] = 2;
  cellTypeDimensions[SVTK_QUADRATIC_LINEAR_QUAD] = 2;
  cellTypeDimensions[SVTK_BIQUADRATIC_QUAD] = 2;
  cellTypeDimensions[SVTK_PARAMETRIC_SURFACE] = 2;
  cellTypeDimensions[SVTK_PARAMETRIC_TRI_SURFACE] = 2;
  cellTypeDimensions[SVTK_PARAMETRIC_QUAD_SURFACE] = 2;
  cellTypeDimensions[SVTK_HIGHER_ORDER_TRIANGLE] = 2;
  cellTypeDimensions[SVTK_HIGHER_ORDER_QUAD] = 2;
  cellTypeDimensions[SVTK_HIGHER_ORDER_POLYGON] = 2;
  cellTypeDimensions[SVTK_LAGRANGE_TRIANGLE] = 2;
  cellTypeDimensions[SVTK_LAGRANGE_QUADRILATERAL] = 2;
  cellTypeDimensions[SVTK_BEZIER_TRIANGLE] = 2;
  cellTypeDimensions[SVTK_BEZIER_QUADRILATERAL] = 2;
}

//----------------------------------------------------------------------------
void svtkCutter::DataSetCutter(svtkDataSet* input, svtkPolyData* output)
{
  svtkIdType cellId;
  int iter;
  svtkPoints* cellPts;
  svtkDoubleArray* cellScalars;
  svtkGenericCell* cell;
  svtkCellArray *newVerts, *newLines, *newPolys;
  svtkPoints* newPoints;
  svtkDoubleArray* cutScalars;
  double value;
  svtkIdType estimatedSize, numCells = input->GetNumberOfCells();
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPointData *inPD, *outPD;
  svtkCellData *inCD = input->GetCellData(), *outCD = output->GetCellData();
  svtkIdList* cellIds;
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();
  int abortExecute = 0;

  cellScalars = svtkDoubleArray::New();

  // Create objects to hold output of contour operation
  //
  estimatedSize = static_cast<svtkIdType>(pow(static_cast<double>(numCells), .75)) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPoints = svtkPoints::New();
  // set precision for the points in the output
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPoints->SetDataType(SVTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }
  newPoints->Allocate(estimatedSize, estimatedSize / 2);
  newVerts = svtkCellArray::New();
  newVerts->AllocateEstimate(estimatedSize, 1);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(estimatedSize, 2);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(estimatedSize, 4);
  cutScalars = svtkDoubleArray::New();
  cutScalars->SetNumberOfTuples(numPts);

  // Interpolate data along edge. If generating cut scalars, do necessary setup
  if (this->GenerateCutScalars)
  {
    inPD = svtkPointData::New();
    inPD->ShallowCopy(input->GetPointData()); // copies original attributes
    inPD->SetScalars(cutScalars);
  }
  else
  {
    inPD = input->GetPointData();
  }
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD, estimatedSize, estimatedSize / 2);
  outCD->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  // Loop over all points evaluating scalar function at each point
  //
  for (svtkIdType i = 0; i < numPts; ++i)
  {
    double x[3];
    input->GetPoint(i, x);
    double s = this->CutFunction->FunctionValue(x);
    cutScalars->SetComponent(i, 0, s);
  }

  // Compute some information for progress methods
  //
  cell = svtkGenericCell::New();
  svtkContourHelper helper(this->Locator, newVerts, newLines, newPolys, inPD, inCD, outPD, outCD,
    estimatedSize, this->GenerateTriangles != 0);
  if (this->SortBy == SVTK_SORT_BY_CELL)
  {
    svtkIdType numCuts = numContours * numCells;
    svtkIdType progressInterval = numCuts / 20 + 1;
    int cut = 0;

    // Loop over all contour values.  Then for each contour value,
    // loop over all cells.
    //
    // This is going to have a problem if the input has 2D and 3D cells.
    // I am fixing a bug where cell data is scrambled because with
    // svtkPolyData output, verts and lines have lower cell ids than triangles.
    for (iter = 0; iter < numContours && !abortExecute; iter++)
    {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellId = 0; cellId < numCells && !abortExecute; cellId++)
      {
        if (!(++cut % progressInterval))
        {
          svtkDebugMacro(<< "Cutting #" << cut);
          this->UpdateProgress(static_cast<double>(cut) / numCuts);
          abortExecute = this->GetAbortExecute();
        }

        input->GetCell(cellId, cell);
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        svtkIdType numCellPts = cellPts->GetNumberOfPoints();
        cellScalars->SetNumberOfTuples(numCellPts);
        for (svtkIdType i = 0; i < numCellPts; ++i)
        {
          double s = cutScalars->GetComponent(cellIds->GetId(i), 0);
          cellScalars->SetTuple(i, &s);
        }

        value = this->ContourValues->GetValue(iter);

        helper.Contour(cell, value, cellScalars, cellId);
      } // for all cells
    }   // for all contour values
  }     // sort by cell

  else // SVTK_SORT_BY_VALUE:
  {
    // Three passes over the cells to process lower dimensional cells first.
    // For poly data output cells need to be added in the order:
    // verts, lines and then polys, or cell data gets mixed up.
    // A better solution is to have an unstructured grid output.
    // I create a table that maps cell type to cell dimensionality,
    // because I need a fast way to get cell dimensionality.
    // This assumes GetCell is slow and GetCellType is fast.
    // I do not like hard coding a list of cell types here,
    // but I do not want to add GetCellDimension(svtkIdType cellId)
    // to the svtkDataSet API.  Since I anticipate that the output
    // will change to svtkUnstructuredGrid.  This temporary solution
    // is acceptable.
    //
    int cellType;
    unsigned char cellTypeDimensions[SVTK_NUMBER_OF_CELL_TYPES];
    svtkCutter::GetCellTypeDimensions(cellTypeDimensions);
    int dimensionality;

    svtkIdType progressInterval = numCells / 20 + 1;

    // We skip 0d cells (points), because they cannot be cut (generate no data).
    for (dimensionality = 1; dimensionality <= 3; ++dimensionality)
    {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellId = 0; cellId < numCells && !abortExecute; cellId++)
      {
        if (!(cellId % progressInterval))
        {
          svtkDebugMacro(<< "Cutting #" << cellId);
          this->UpdateProgress(static_cast<double>(cellId) / numCells);
          abortExecute = this->GetAbortExecute();
        }

        // I assume that "GetCellType" is fast.
        cellType = input->GetCellType(cellId);
        if (cellType >= SVTK_NUMBER_OF_CELL_TYPES)
        { // Protect against new cell types added.
          svtkErrorMacro("Unknown cell type " << cellType);
          continue;
        }
        if (cellTypeDimensions[cellType] != dimensionality)
        {
          continue;
        }
        input->GetCell(cellId, cell);
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        svtkIdType numCellPts = cellPts->GetNumberOfPoints();
        cellScalars->SetNumberOfTuples(numCellPts);
        for (svtkIdType i = 0; i < numCellPts; i++)
        {
          double s = cutScalars->GetComponent(cellIds->GetId(i), 0);
          cellScalars->SetTuple(i, &s);
        }

        // Loop over all contour values.
        for (iter = 0; iter < numContours && !abortExecute; iter++)
        {
          value = this->ContourValues->GetValue(iter);
          helper.Contour(cell, value, cellScalars, cellId);
        } // for all contour values
      }   // for all cells
    }     // for all dimensions.
  }       // sort by value

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  cell->Delete();
  cellScalars->Delete();
  cutScalars->Delete();

  if (this->GenerateCutScalars)
  {
    inPD->Delete();
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells())
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();
}

//----------------------------------------------------------------------------
void svtkCutter::UnstructuredGridCutter(svtkDataSet* input, svtkPolyData* output)
{
  svtkIdType i;
  int iter;
  svtkDoubleArray* cellScalars;
  svtkCellArray *newVerts, *newLines, *newPolys;
  svtkPoints* newPoints;
  svtkDoubleArray* cutScalars;
  double value;
  svtkIdType estimatedSize, numCells = input->GetNumberOfCells();
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCellPts;
  svtkIdType* ptIds;
  svtkPointData *inPD, *outPD;
  svtkCellData *inCD = input->GetCellData(), *outCD = output->GetCellData();
  svtkIdList* cellIds;
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double* contourValues = this->ContourValues->GetValues();
  double* contourValuesEnd = contourValues + numContours;
  double* contourIter;

  int abortExecute = 0;

  double range[2];

  // Create objects to hold output of contour operation
  //
  estimatedSize = static_cast<svtkIdType>(pow(static_cast<double>(numCells), .75)) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPoints = svtkPoints::New();
  svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
  // set precision for the points in the output
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    if (inputPointSet)
    {
      newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPoints->SetDataType(SVTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }
  newPoints->Allocate(estimatedSize, estimatedSize / 2);
  newVerts = svtkCellArray::New();
  newVerts->AllocateEstimate(estimatedSize, 1);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(estimatedSize, 2);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(estimatedSize, 4);
  cutScalars = svtkDoubleArray::New();
  cutScalars->SetNumberOfTuples(numPts);

  // Interpolate data along edge. If generating cut scalars, do necessary setup
  if (this->GenerateCutScalars)
  {
    inPD = svtkPointData::New();
    inPD->ShallowCopy(input->GetPointData()); // copies original attributes
    inPD->SetScalars(cutScalars);
  }
  else
  {
    inPD = input->GetPointData();
  }
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD, estimatedSize, estimatedSize / 2);
  outCD->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  // Loop over all points evaluating scalar function at each point
  if (inputPointSet)
  {
    svtkDataArray* dataArrayInput = inputPointSet->GetPoints()->GetData();
    this->CutFunction->FunctionValue(dataArrayInput, cutScalars);
  }
  svtkSmartPointer<svtkCellIterator> cellIter =
    svtkSmartPointer<svtkCellIterator>::Take(input->NewCellIterator());
  svtkNew<svtkGenericCell> cell;
  svtkIdList* pointIdList;
  double* scalarArrayPtr = cutScalars->GetPointer(0);
  double tempScalar;
  cellScalars = cutScalars->NewInstance();
  cellScalars->SetNumberOfComponents(cutScalars->GetNumberOfComponents());
  cellScalars->Allocate(SVTK_CELL_SIZE * cutScalars->GetNumberOfComponents());

  svtkContourHelper helper(this->Locator, newVerts, newLines, newPolys, inPD, inCD, outPD, outCD,
    estimatedSize, this->GenerateTriangles != 0);
  if (this->SortBy == SVTK_SORT_BY_CELL)
  {
    // Compute some information for progress methods
    //
    svtkIdType numCuts = numContours * numCells;
    svtkIdType progressInterval = numCuts / 20 + 1;
    int cut = 0;

    // Loop over all contour values.  Then for each contour value,
    // loop over all cells.
    //
    for (iter = 0; iter < numContours && !abortExecute; iter++)
    {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal() && !abortExecute;
           cellIter->GoToNextCell())
      {
        if (!(++cut % progressInterval))
        {
          svtkDebugMacro(<< "Cutting #" << cut);
          this->UpdateProgress(static_cast<double>(cut) / numCuts);
          abortExecute = this->GetAbortExecute();
        }

        pointIdList = cellIter->GetPointIds();
        numCellPts = pointIdList->GetNumberOfIds();
        ptIds = pointIdList->GetPointer(0);

        // find min and max values in scalar data
        range[0] = range[1] = scalarArrayPtr[ptIds[0]];

        for (i = 1; i < numCellPts; i++)
        {
          tempScalar = scalarArrayPtr[ptIds[i]];
          range[0] = std::min(range[0], tempScalar);
          range[1] = std::max(range[1], tempScalar);
        } // for all points in this cell

        int needCell = 0;
        double val = this->ContourValues->GetValue(iter);
        if (val >= range[0] && val <= range[1])
        {
          needCell = 1;
        }

        if (needCell)
        {
          cellIter->GetCell(cell);
          svtkIdType cellId = cellIter->GetCellId();
          input->SetCellOrderAndRationalWeights(cellId, cell);
          cellIds = cell->GetPointIds();
          cutScalars->GetTuples(cellIds, cellScalars);
          // Loop over all contour values.
          for (iter = 0; iter < numContours && !abortExecute; iter++)
          {
            value = this->ContourValues->GetValue(iter);
            helper.Contour(cell, value, cellScalars, cellIter->GetCellId());
          }
        }

      } // for all cells
    }   // for all contour values
  }     // sort by cell

  else // SORT_BY_VALUE:
  {
    // Three passes over the cells to process lower dimensional cells first.
    // For poly data output cells need to be added in the order:
    // verts, lines and then polys, or cell data gets mixed up.
    // A better solution is to have an unstructured grid output.
    // I create a table that maps cell type to cell dimensionality,
    // because I need a fast way to get cell dimensionality.
    // This assumes GetCell is slow and GetCellType is fast.
    // I do not like hard coding a list of cell types here,
    // but I do not want to add GetCellDimension(svtkIdType cellId)
    // to the svtkDataSet API.  Since I anticipate that the output
    // will change to svtkUnstructuredGrid.  This temporary solution
    // is acceptable.
    //
    int cellType;
    unsigned char cellTypeDimensions[SVTK_NUMBER_OF_CELL_TYPES];
    svtkCutter::GetCellTypeDimensions(cellTypeDimensions);
    int dimensionality;

    // Compute some information for progress methods
    svtkIdType numCuts = 3 * numCells;
    svtkIdType progressInterval = numCuts / 20 + 1;
    int cellId = 0;

    // We skip 0d cells (points), because they cannot be cut (generate no data).
    for (dimensionality = 1; dimensionality <= 3; ++dimensionality)
    {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal() && !abortExecute;
           cellIter->GoToNextCell())
      {
        if (!(++cellId % progressInterval))
        {
          svtkDebugMacro(<< "Cutting #" << cellId);
          this->UpdateProgress(static_cast<double>(cellId) / numCuts);
          abortExecute = this->GetAbortExecute();
        }

        // Just fetch the cell type -- least expensive.
        cellType = cellIter->GetCellType();

        // Protect against new cell types added.
        if (cellType >= SVTK_NUMBER_OF_CELL_TYPES)
        {
          svtkErrorMacro("Unknown cell type " << cellType);
          continue;
        }

        // Check if the type is valid for this pass
        if (cellTypeDimensions[cellType] != dimensionality)
        {
          continue;
        }

        // Just fetch the cell point ids -- moderately expensive.
        pointIdList = cellIter->GetPointIds();
        numCellPts = pointIdList->GetNumberOfIds();
        ptIds = pointIdList->GetPointer(0);

        // find min and max values in scalar data
        range[0] = range[1] = scalarArrayPtr[ptIds[0]];

        for (i = 1; i < numCellPts; ++i)
        {
          tempScalar = scalarArrayPtr[ptIds[i]];
          range[0] = std::min(range[0], tempScalar);
          range[1] = std::max(range[1], tempScalar);
        } // for all points in this cell

        // Check if the full cell is needed
        int needCell = 0;
        for (contourIter = contourValues; contourIter != contourValuesEnd; ++contourIter)
        {
          if (*contourIter >= range[0] && *contourIter <= range[1])
          {
            needCell = 1;
            break;
          }
        }

        if (needCell)
        {
          // Fetch the full cell -- most expensive.
          cellIter->GetCell(cell);
          input->SetCellOrderAndRationalWeights(cellId, cell);
          cutScalars->GetTuples(pointIdList, cellScalars);
          // Loop over all contour values.
          for (contourIter = contourValues; contourIter != contourValuesEnd; ++contourIter)
          {
            helper.Contour(cell, *contourIter, cellScalars, cellIter->GetCellId());
          } // for all contour values
        }   // if need cell
      }     // for all cells
    }       // for all dimensions (1,2,3).
  }         // sort by value

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  cellScalars->Delete();
  cutScalars->Delete();

  if (this->GenerateCutScalars)
  {
    inPD->Delete();
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells())
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkCutter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
int svtkCutter::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int svtkCutter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkCutter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cut Function: " << this->CutFunction << "\n";
  os << indent << "Sort By: " << this->GetSortByAsString() << "\n";

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Generate Cut Scalars: " << (this->GenerateCutScalars ? "On\n" : "Off\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
}
