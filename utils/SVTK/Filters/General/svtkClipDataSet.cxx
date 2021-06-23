/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClipDataSet.h"

#include "svtkCallbackCommand.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkClipVolume.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGenericCell.h"
#include "svtkImageData.h"
#include "svtkImplicitFunction.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyhedron.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <cmath>

svtkStandardNewMacro(svtkClipDataSet);
svtkCxxSetObjectMacro(svtkClipDataSet, ClipFunction, svtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
svtkClipDataSet::svtkClipDataSet(svtkImplicitFunction* cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = nullptr;
  this->Value = 0.0;
  this->UseValueAsOffset = true;
  this->GenerateClipScalars = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->GenerateClippedOutput = 0;
  this->MergeTolerance = 0.01;

  this->SetNumberOfOutputPorts(2);
  svtkUnstructuredGrid* output2 = svtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);

  // Setup a callback for the internal readers to report progress.
  this->InternalProgressObserver = svtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(&svtkClipDataSet::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
svtkClipDataSet::~svtkClipDataSet()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->SetClipFunction(nullptr);
  this->InternalProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void svtkClipDataSet::InternalProgressCallbackFunction(
  svtkObject* arg, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<svtkClipDataSet*>(clientdata)
    ->InternalProgressCallback(static_cast<svtkAlgorithm*>(arg));
}

//----------------------------------------------------------------------------
void svtkClipDataSet::InternalProgressCallback(svtkAlgorithm* algorithm)
{
  float progress = algorithm->GetProgress();
  this->UpdateProgress(progress);
  if (this->AbortExecute)
  {
    algorithm->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
svtkMTimeType svtkClipDataSet::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->ClipFunction != nullptr)
  {
    time = this->ClipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkClipDataSet::GetClippedOutput()
{
  if (!this->GenerateClippedOutput)
  {
    return nullptr;
  }
  return svtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
//
// Clip through data generating surface.
//
int svtkClipDataSet::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* realInput = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  // We have to create a copy of the input because clip requires being
  // able to InterpolateAllocate point data from the input that is
  // exactly the same as output. If the input arrays and output arrays
  // are different svtkCell3D's Clip will fail. By calling InterpolateAllocate
  // here, we make sure that the output will look exactly like the output
  // (unwanted arrays will be eliminated in InterpolateAllocate). The
  // last argument of InterpolateAllocate makes sure that arrays are shallow
  // copied from realInput to input.
  svtkSmartPointer<svtkDataSet> input;
  input.TakeReference(realInput->NewInstance());
  input->CopyStructure(realInput);
  input->GetCellData()->PassData(realInput->GetCellData());
  input->GetPointData()->InterpolateAllocate(realInput->GetPointData(), 0, 0, 1);

  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkUnstructuredGrid* clippedOutput = this->GetClippedOutput();

  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD[2];
  svtkPoints* newPoints;
  svtkFloatArray* cellScalars;
  svtkDataArray* clipScalars;
  svtkPoints* cellPts;
  svtkIdList* cellIds;
  double s;
  svtkIdType npts;
  const svtkIdType* pts;
  int cellType = 0;
  svtkIdType i;
  int j;
  svtkIdType estimatedSize;
  svtkUnsignedCharArray* types[2];
  types[0] = types[1] = nullptr;
  int numOutputs = 1;

  outCD[0] = nullptr;
  outCD[1] = nullptr;

  svtkDebugMacro(<< "Clipping dataset");

  int inputObjectType = input->GetDataObjectType();

  // if we have volumes
  if (inputObjectType == SVTK_STRUCTURED_POINTS || inputObjectType == SVTK_IMAGE_DATA)
  {
    int dimension;
    int* dims = svtkImageData::SafeDownCast(input)->GetDimensions();
    for (dimension = 3, i = 0; i < 3; i++)
    {
      if (dims[i] <= 1)
      {
        dimension--;
      }
    }
    if (dimension >= 3)
    {
      this->ClipVolume(input, output);
      return 1;
    }
  }

  // Initialize self; create output objects
  //
  if (numPts < 1)
  {
    svtkDebugMacro(<< "No data to clip");
    return 1;
  }

  if (!this->ClipFunction && this->GenerateClipScalars)
  {
    svtkErrorMacro(<< "Cannot generate clip scalars if no clip function defined");
    return 1;
  }

  if (numCells < 1)
  {
    return this->ClipPoints(input, output, inputVector);
  }

  // allocate the output and associated helper classes
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  cellScalars = svtkFloatArray::New();
  cellScalars->Allocate(SVTK_CELL_SIZE);
  svtkCellArray* conn[2];
  conn[0] = conn[1] = nullptr;
  conn[0] = svtkCellArray::New();
  conn[0]->AllocateEstimate(estimatedSize, 1);
  conn[0]->InitTraversal();
  types[0] = svtkUnsignedCharArray::New();
  types[0]->Allocate(estimatedSize, estimatedSize / 2);
  if (this->GenerateClippedOutput)
  {
    numOutputs = 2;
    conn[1] = svtkCellArray::New();
    conn[1]->AllocateEstimate(estimatedSize, 1);
    conn[1]->InitTraversal();
    types[1] = svtkUnsignedCharArray::New();
    types[1]->Allocate(estimatedSize, estimatedSize / 2);
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

  newPoints->Allocate(numPts, numPts / 2);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  // Determine whether we're clipping with input scalars or a clip function
  // and do necessary setup.
  if (this->ClipFunction)
  {
    svtkFloatArray* tmpScalars = svtkFloatArray::New();
    tmpScalars->SetNumberOfTuples(numPts);
    tmpScalars->SetName("ClipDataSetScalars");
    inPD = svtkPointData::New();
    inPD->ShallowCopy(input->GetPointData()); // copies original
    if (this->GenerateClipScalars)
    {
      inPD->SetScalars(tmpScalars);
    }
    for (i = 0; i < numPts; i++)
    {
      s = this->ClipFunction->FunctionValue(input->GetPoint(i));
      tmpScalars->SetTuple1(i, s);
    }
    clipScalars = tmpScalars;
  }
  else // using input scalars
  {
    clipScalars = this->GetInputArrayToProcess(0, inputVector);
    if (!clipScalars)
    {
      for (i = 0; i < 2; i++)
      {
        if (conn[i])
        {
          conn[i]->Delete();
        }
        if (types[i])
        {
          types[i]->Delete();
        }
      }
      cellScalars->Delete();
      newPoints->Delete();
      // When processing composite datasets with partial arrays, this warning is
      // not applicable, hence disabling it.
      // svtkErrorMacro(<<"Cannot clip without clip function or input scalars");
      return 1;
    }
  }

  // Refer to BUG #8494 and BUG #11016. I cannot see any reason why one would
  // want to turn CopyScalars Off. My understanding is that this was done to
  // avoid copying of "ClipDataSetScalars" to the output when
  // this->GenerateClipScalars is false. But, if GenerateClipScalars is false,
  // then "ClipDataSetScalars" is not added as scalars to the input at all
  // (refer to code above) so it's a non-issue. Leaving CopyScalars untouched
  // i.e. ON avoids dropping of arrays (#8484) as well as segfaults (#11016).
  // if ( !this->GenerateClipScalars &&
  //  !this->GetInputArrayToProcess(0,inputVector))
  //  {
  //  outPD->CopyScalarsOff();
  //  }
  // else
  //  {
  //  outPD->CopyScalarsOn();
  //  }
  svtkDataSetAttributes* tempDSA = svtkDataSetAttributes::New();
  tempDSA->InterpolateAllocate(inPD, 1, 2);
  outPD->InterpolateAllocate(inPD, estimatedSize, estimatedSize / 2);
  tempDSA->Delete();
  outCD[0] = output->GetCellData();
  outCD[0]->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);
  if (this->GenerateClippedOutput)
  {
    outCD[1] = clippedOutput->GetCellData();
    outCD[1]->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);
  }

  // Process all cells and clip each in turn
  //
  int abort = 0;
  svtkIdType updateTime = numCells / 20 + 1; // update roughly every 5%
  svtkGenericCell* cell = svtkGenericCell::New();
  int num[2];
  num[0] = num[1] = 0;
  int numNew[2];
  numNew[0] = numNew[1] = 0;
  for (svtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    if (!(cellId % updateTime))
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(cellId, cell);
    cellPts = cell->GetPoints();
    cellIds = cell->GetPointIds();
    npts = cellPts->GetNumberOfPoints();

    // evaluate implicit cutting function
    for (i = 0; i < npts; i++)
    {
      s = clipScalars->GetComponent(cellIds->GetId(i), 0);
      cellScalars->InsertTuple(i, &s);
    }

    double value = 0.0;
    if (this->UseValueAsOffset || !this->ClipFunction)
    {
      value = this->Value;
    }

    // perform the clipping
    cell->Clip(value, cellScalars, this->Locator, conn[0], inPD, outPD, inCD, cellId, outCD[0],
      this->InsideOut);
    numNew[0] = conn[0]->GetNumberOfCells() - num[0];
    num[0] = conn[0]->GetNumberOfCells();

    if (this->GenerateClippedOutput)
    {
      cell->Clip(value, cellScalars, this->Locator, conn[1], inPD, outPD, inCD, cellId, outCD[1],
        !this->InsideOut);
      numNew[1] = conn[1]->GetNumberOfCells() - num[1];
      num[1] = conn[1]->GetNumberOfCells();
    }

    for (i = 0; i < numOutputs; i++) // for both outputs
    {
      for (j = 0; j < numNew[i]; j++)
      {
        if (cell->GetCellType() == SVTK_POLYHEDRON)
        {
          // Polyhedron cells have a special cell connectivity format
          //(nCell0Faces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...).
          // But we don't need to deal with it here. The special case is handled
          // by svtkUnstructuredGrid::SetCells(), which will be called next.
          types[i]->InsertNextValue(SVTK_POLYHEDRON);
        }
        else
        {
          conn[i]->GetNextCell(npts, pts);

          // For each new cell added, got to set the type of the cell
          switch (cell->GetCellDimension())
          {
            case 0: // points are generated--------------------------------
              cellType = (npts > 1 ? SVTK_POLY_VERTEX : SVTK_VERTEX);
              break;

            case 1: // lines are generated---------------------------------
              cellType = (npts > 2 ? SVTK_POLY_LINE : SVTK_LINE);
              break;

            case 2: // polygons are generated------------------------------
              cellType = (npts == 3 ? SVTK_TRIANGLE : (npts == 4 ? SVTK_QUAD : SVTK_POLYGON));
              break;

            case 3: // tetrahedra or wedges are generated------------------
              cellType = (npts == 4 ? SVTK_TETRA : SVTK_WEDGE);
              break;
          } // switch

          types[i]->InsertNextValue(cellType);
        }
      } // for each new cell
    }   // for both outputs
  }     // for each cell

  cell->Delete();
  cellScalars->Delete();

  if (this->ClipFunction)
  {
    clipScalars->Delete();
    inPD->Delete();
  }

  output->SetPoints(newPoints);
  output->SetCells(types[0], conn[0]);
  conn[0]->Delete();
  types[0]->Delete();

  if (this->GenerateClippedOutput)
  {
    clippedOutput->SetPoints(newPoints);
    clippedOutput->SetCells(types[1], conn[1]);
    conn[1]->Delete();
    types[1]->Delete();
  }

  newPoints->Delete();
  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int svtkClipDataSet::ClipPoints(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkInformationVector** inputVector)
{
  svtkPoints* outPoints = svtkPoints::New();

  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();

  svtkIdType numPts = input->GetNumberOfPoints();

  outPD->CopyAllocate(inPD, numPts / 2, numPts / 4);

  double value = 0.0;
  if (this->UseValueAsOffset || !this->ClipFunction)
  {
    value = this->Value;
  }
  if (this->ClipFunction)
  {
    for (svtkIdType i = 0; i < numPts; i++)
    {
      double* pt = input->GetPoint(i);
      double fv = this->ClipFunction->FunctionValue(pt);
      int addPoint = 0;
      if (this->InsideOut)
      {
        if (fv <= value)
        {
          addPoint = 1;
        }
      }
      else
      {
        if (fv > value)
        {
          addPoint = 1;
        }
      }
      if (addPoint)
      {
        svtkIdType id = outPoints->InsertNextPoint(input->GetPoint(i));
        outPD->CopyData(inPD, i, id);
      }
    }
  }
  else
  {
    svtkDataArray* clipScalars = this->GetInputArrayToProcess(0, inputVector);
    if (clipScalars)
    {
      for (svtkIdType i = 0; i < numPts; i++)
      {
        int addPoint = 0;
        double fv = clipScalars->GetTuple1(i);
        if (this->InsideOut)
        {
          if (fv <= value)
          {
            addPoint = 1;
          }
        }
        else
        {
          if (fv > value)
          {
            addPoint = 1;
          }
        }
        if (addPoint)
        {
          svtkIdType id = outPoints->InsertNextPoint(input->GetPoint(i));
          outPD->CopyData(inPD, i, id);
        }
      }
    }
  }

  output->SetPoints(outPoints);
  outPoints->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkClipDataSet::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkClipDataSet::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkClipDataSet::ClipVolume(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  svtkClipVolume* clipVolume = svtkClipVolume::New();

  clipVolume->AddObserver(svtkCommand::ProgressEvent, this->InternalProgressObserver);

  // We cannot set the input directly.  This messes up the partitioning.
  // output->UpdateNumberOfPieces gets set to 1.
  svtkImageData* tmp = svtkImageData::New();
  tmp->ShallowCopy(svtkImageData::SafeDownCast(input));

  clipVolume->SetInputData(tmp);
  double value = 0.0;
  if (this->UseValueAsOffset || !this->ClipFunction)
  {
    value = this->Value;
  }
  clipVolume->SetValue(value);
  clipVolume->SetInsideOut(this->InsideOut);
  clipVolume->SetClipFunction(this->ClipFunction);
  clipVolume->SetGenerateClipScalars(this->GenerateClipScalars);
  clipVolume->SetGenerateClippedOutput(this->GenerateClippedOutput);
  clipVolume->SetMergeTolerance(this->MergeTolerance);
  clipVolume->SetDebug(this->Debug);
  clipVolume->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  clipVolume->Update();

  clipVolume->RemoveObserver(this->InternalProgressObserver);
  svtkUnstructuredGrid* clipOutput = clipVolume->GetOutput();

  output->CopyStructure(clipOutput);
  output->GetPointData()->ShallowCopy(clipOutput->GetPointData());
  output->GetCellData()->ShallowCopy(clipOutput->GetCellData());
  clipVolume->Delete();
  tmp->Delete();
}

//----------------------------------------------------------------------------
int svtkClipDataSet::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkClipDataSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if (this->ClipFunction)
  {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");

  os << indent << "UseValueAsOffset: " << (this->UseValueAsOffset ? "On\n" : "Off\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
}
