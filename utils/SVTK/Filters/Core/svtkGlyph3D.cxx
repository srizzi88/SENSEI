/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlyph3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGlyph3D.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"
#include "svtkTrivialProducer.h"
#include "svtkUniformGrid.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkGlyph3D);
svtkCxxSetObjectMacro(svtkGlyph3D, SourceTransform, svtkTransform);

//----------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
svtkGlyph3D::svtkGlyph3D()
{
  this->Scaling = 1;
  this->ColorMode = SVTK_COLOR_BY_SCALE;
  this->ScaleMode = SVTK_SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->VectorMode = SVTK_USE_VECTOR;
  this->Clamping = 0;
  this->IndexMode = SVTK_INDEXING_OFF;
  this->GeneratePointIds = 0;
  this->PointIdsName = nullptr;
  this->SetPointIdsName("InputPointIds");
  this->SetNumberOfInputPorts(2);
  this->FillCellData = 0;
  this->SourceTransform = nullptr;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
  // by default process active point vectors
  this->SetInputArrayToProcess(
    1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
  // by default process active point normals
  this->SetInputArrayToProcess(
    2, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::NORMALS);
  // by default process active point scalars
  this->SetInputArrayToProcess(
    3, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkGlyph3D::~svtkGlyph3D()
{
  delete[] PointIdsName;
  this->SetSourceTransform(nullptr);
}

//----------------------------------------------------------------------------
svtkMTimeType svtkGlyph3D::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;
  if (this->SourceTransform != nullptr)
  {
    time = this->SourceTransform->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//----------------------------------------------------------------------------
int svtkGlyph3D::RequestData(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
  svtkInformationVector* outputVector)
{
  // get the info objects
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0], 0);
  svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);

  return this->Execute(input, inputVector[1], output) ? 1 : 0;
}

//----------------------------------------------------------------------------
bool svtkGlyph3D::Execute(svtkDataSet* input, svtkInformationVector* sourceVector, svtkPolyData* output)
{
  svtkDataArray* inSScalars = this->GetInputArrayToProcess(0, input);
  svtkDataArray* inVectors = this->GetInputArrayToProcess(1, input);
  return this->Execute(input, sourceVector, output, inSScalars, inVectors);
}

//----------------------------------------------------------------------------
bool svtkGlyph3D::Execute(svtkDataSet* input, svtkInformationVector* sourceVector, svtkPolyData* output,
  svtkDataArray* inSScalars, svtkDataArray* inVectors)
{
  assert(input && output);
  if (input == nullptr || output == nullptr)
  {
    // nothing to do.
    return true;
  }

  // this is used to respect blanking specified on uniform grids.
  svtkUniformGrid* inputUG = svtkUniformGrid::SafeDownCast(input);

  svtkPointData* pd;
  svtkDataArray* inCScalars; // Scalars for Coloring
  unsigned char* inGhostLevels = nullptr;
  svtkDataArray *inNormals, *sourceNormals = nullptr;
  svtkDataArray* sourceTCoords = nullptr;
  svtkIdType numPts, numSourcePts, numSourceCells, inPtId, i;
  svtkPoints* sourcePts = nullptr;
  svtkSmartPointer<svtkPoints> transformedSourcePts = svtkSmartPointer<svtkPoints>::New();
  svtkPoints* newPts;
  svtkDataArray* newScalars = nullptr;
  svtkDataArray* newVectors = nullptr;
  svtkDataArray* newNormals = nullptr;
  svtkDataArray* newTCoords = nullptr;
  double x[3], v[3], vNew[3], s = 0.0, vMag = 0.0, value, tc[3];
  svtkTransform* trans = svtkTransform::New();
  svtkNew<svtkIdList> pointIdList;
  svtkIdList* cellPts;
  int npts;
  svtkIdList* pts;
  svtkIdType ptIncr, cellIncr, cellId;
  int haveVectors, haveNormals, haveTCoords = 0;
  double scalex, scaley, scalez, den;
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  int numberOfSources = this->GetNumberOfInputConnections(1);
  svtkIdTypeArray* pointIds = nullptr;
  svtkSmartPointer<svtkPolyData> source = this->GetSource(0, sourceVector);
  svtkNew<svtkIdList> srcPointIdList;
  svtkNew<svtkIdList> dstPointIdList;
  svtkNew<svtkIdList> srcCellIdList;
  svtkNew<svtkIdList> dstCellIdList;

  svtkDebugMacro(<< "Generating glyphs");

  pts = svtkIdList::New();
  pts->Allocate(SVTK_CELL_SIZE);

  pd = input->GetPointData();
  inNormals = this->GetInputArrayToProcess(2, input);
  inCScalars = this->GetInputArrayToProcess(3, input);
  if (inCScalars == nullptr)
  {
    inCScalars = inSScalars;
  }

  svtkDataArray* temp = nullptr;
  if (pd)
  {
    temp = pd->GetArray(svtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != SVTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    svtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    inGhostLevels = static_cast<svtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    svtkDebugMacro(<< "No points to glyph!");
    pts->Delete();
    trans->Delete();
    return 1;
  }

  // Check input for consistency
  //
  if ((den = this->Range[1] - this->Range[0]) == 0.0)
  {
    den = 1.0;
  }
  if (this->VectorMode != SVTK_VECTOR_ROTATION_OFF &&
    ((this->VectorMode == SVTK_USE_VECTOR && inVectors != nullptr) ||
      (this->VectorMode == SVTK_USE_NORMAL && inNormals != nullptr)))
  {
    haveVectors = 1;
  }
  else
  {
    haveVectors = 0;
  }

  if ((this->IndexMode == SVTK_INDEXING_BY_SCALAR && !inSScalars) ||
    (this->IndexMode == SVTK_INDEXING_BY_VECTOR &&
      ((!inVectors && this->VectorMode == SVTK_USE_VECTOR) ||
        (!inNormals && this->VectorMode == SVTK_USE_NORMAL))))
  {
    if (source == nullptr)
    {
      svtkErrorMacro(<< "Indexing on but don't have data to index with");
      pts->Delete();
      trans->Delete();
      return true;
    }
    else
    {
      svtkWarningMacro(<< "Turning indexing off: no data to index with");
      this->IndexMode = SVTK_INDEXING_OFF;
    }
  }

  // Allocate storage for output PolyData
  //
  outputPD->CopyVectorsOff();
  outputPD->CopyNormalsOff();
  outputPD->CopyTCoordsOff();

  if (source == nullptr)
  {
    svtkNew<svtkPolyData> defaultSource;
    defaultSource->AllocateExact(0, 0, 1, 2, 0, 0, 0, 0);
    svtkNew<svtkPoints> defaultPoints;
    defaultPoints->Allocate(6);
    defaultPoints->InsertNextPoint(0, 0, 0);
    defaultPoints->InsertNextPoint(1, 0, 0);
    svtkIdType defaultPointIds[2];
    defaultPointIds[0] = 0;
    defaultPointIds[1] = 1;
    defaultSource->SetPoints(defaultPoints);
    defaultSource->InsertNextCell(SVTK_LINE, 2, defaultPointIds);
    source = defaultSource;
  }

  if (this->IndexMode != SVTK_INDEXING_OFF)
  {
    pd = nullptr;
    haveNormals = 1;
    for (numSourcePts = numSourceCells = i = 0; i < numberOfSources; i++)
    {
      source = this->GetSource(i, sourceVector);
      if (source != nullptr)
      {
        if (source->GetNumberOfPoints() > numSourcePts)
        {
          numSourcePts = source->GetNumberOfPoints();
        }
        if (source->GetNumberOfCells() > numSourceCells)
        {
          numSourceCells = source->GetNumberOfCells();
        }
        if (!(sourceNormals = source->GetPointData()->GetNormals()))
        {
          haveNormals = 0;
        }
      }
    }
  }
  else
  {
    sourcePts = source->GetPoints();
    numSourcePts = sourcePts->GetNumberOfPoints();
    numSourceCells = source->GetNumberOfCells();

    sourceNormals = source->GetPointData()->GetNormals();
    if (sourceNormals)
    {
      haveNormals = 1;
    }
    else
    {
      haveNormals = 0;
    }

    sourceTCoords = source->GetPointData()->GetTCoords();
    if (sourceTCoords)
    {
      haveTCoords = 1;
    }
    else
    {
      haveTCoords = 0;
    }

    // Prepare to copy output.
    pd = input->GetPointData();
    outputPD->CopyAllocate(pd, numPts * numSourcePts);
    if (this->FillCellData)
    {
      outputCD->CopyAllocate(pd, numPts * numSourceCells);
    }
  }

  srcPointIdList->SetNumberOfIds(numSourcePts);
  dstPointIdList->SetNumberOfIds(numSourcePts);
  srcCellIdList->SetNumberOfIds(numSourceCells);
  dstCellIdList->SetNumberOfIds(numSourceCells);

  newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(numPts * numSourcePts);
  if (this->GeneratePointIds)
  {
    pointIds = svtkIdTypeArray::New();
    pointIds->SetName(this->PointIdsName);
    pointIds->Allocate(numPts * numSourcePts);
    outputPD->AddArray(pointIds);
    pointIds->Delete();
  }
  if (this->ColorMode == SVTK_COLOR_BY_SCALAR && inCScalars)
  {
    newScalars = inCScalars->NewInstance();
    newScalars->SetNumberOfComponents(inCScalars->GetNumberOfComponents());
    newScalars->Allocate(inCScalars->GetNumberOfComponents() * numPts * numSourcePts);
    newScalars->SetName(inCScalars->GetName());
  }
  else if ((this->ColorMode == SVTK_COLOR_BY_SCALE) && inSScalars)
  {
    newScalars = svtkFloatArray::New();
    newScalars->Allocate(numPts * numSourcePts);
    newScalars->SetName("GlyphScale");
    if (this->ScaleMode == SVTK_SCALE_BY_SCALAR)
    {
      newScalars->SetName(inSScalars->GetName());
    }
  }
  else if ((this->ColorMode == SVTK_COLOR_BY_VECTOR) && haveVectors)
  {
    newScalars = svtkFloatArray::New();
    newScalars->Allocate(numPts * numSourcePts);
    newScalars->SetName("VectorMagnitude");
  }
  if (haveVectors)
  {
    newVectors = svtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3 * numPts * numSourcePts);
    newVectors->SetName("GlyphVector");
  }
  if (haveNormals)
  {
    newNormals = svtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3 * numPts * numSourcePts);
    newNormals->SetName("Normals");
  }
  if (haveTCoords)
  {
    newTCoords = svtkFloatArray::New();
    int numComps = sourceTCoords->GetNumberOfComponents();
    newTCoords->SetNumberOfComponents(numComps);
    newTCoords->Allocate(numComps * numPts * numSourcePts);
    newTCoords->SetName("TCoords");
  }

  // Setting up for calls to PolyData::InsertNextCell()
  output->AllocateEstimate(numPts * numSourceCells, 3);

  transformedSourcePts->SetDataTypeToDouble();
  transformedSourcePts->Allocate(numSourcePts);

  // Traverse all Input points, transforming Source points and copying
  // point attributes.
  //
  ptIncr = 0;
  cellIncr = 0;
  for (inPtId = 0; inPtId < numPts; inPtId++)
  {
    scalex = scaley = scalez = 1.0;
    if (!(inPtId % 10000))
    {
      this->UpdateProgress(static_cast<double>(inPtId) / numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    // Get the scalar and vector data
    if (inSScalars)
    {
      s = inSScalars->GetComponent(inPtId, 0);
      if (this->ScaleMode == SVTK_SCALE_BY_SCALAR || this->ScaleMode == SVTK_DATA_SCALING_OFF)
      {
        scalex = scaley = scalez = s;
      }
    }

    if (haveVectors)
    {
      svtkDataArray* array3D = this->VectorMode == SVTK_USE_NORMAL ? inNormals : inVectors;
      if (array3D->GetNumberOfComponents() > 3)
      {
        svtkErrorMacro(<< "svtkDataArray " << array3D->GetName() << " has more than 3 components.\n");
        pts->Delete();
        trans->Delete();
        if (newPts)
        {
          newPts->Delete();
        }
        if (newVectors)
        {
          newVectors->Delete();
        }
        return false;
      }

      v[0] = 0;
      v[1] = 0;
      v[2] = 0;
      array3D->GetTuple(inPtId, v);
      vMag = svtkMath::Norm(v);
      if (this->ScaleMode == SVTK_SCALE_BY_VECTORCOMPONENTS)
      {
        scalex = v[0];
        scaley = v[1];
        scalez = v[2];
      }
      else if (this->ScaleMode == SVTK_SCALE_BY_VECTOR)
      {
        scalex = scaley = scalez = vMag;
      }
    }

    // Clamp data scale if enabled
    if (this->Clamping)
    {
      scalex = (scalex < this->Range[0] ? this->Range[0]
                                        : (scalex > this->Range[1] ? this->Range[1] : scalex));
      scalex = (scalex - this->Range[0]) / den;
      scaley = (scaley < this->Range[0] ? this->Range[0]
                                        : (scaley > this->Range[1] ? this->Range[1] : scaley));
      scaley = (scaley - this->Range[0]) / den;
      scalez = (scalez < this->Range[0] ? this->Range[0]
                                        : (scalez > this->Range[1] ? this->Range[1] : scalez));
      scalez = (scalez - this->Range[0]) / den;
    }

    // Compute index into table of glyphs
    if (this->IndexMode != SVTK_INDEXING_OFF)
    {
      if (this->IndexMode == SVTK_INDEXING_BY_SCALAR)
      {
        value = s;
      }
      else
      {
        value = vMag;
      }

      int index = static_cast<int>((value - this->Range[0]) * numberOfSources / den);
      index = (index < 0 ? 0 : (index >= numberOfSources ? (numberOfSources - 1) : index));

      source = this->GetSource(index, sourceVector);
      if (source != nullptr)
      {
        sourcePts = source->GetPoints();
        sourceNormals = source->GetPointData()->GetNormals();
        numSourcePts = sourcePts->GetNumberOfPoints();
        numSourceCells = source->GetNumberOfCells();
      }
    }

    // Make sure we're not indexing into empty glyph
    if (source == nullptr)
    {
      continue;
    }

    // Check ghost points.
    // If we are processing a piece, we do not want to duplicate
    // glyphs on the borders.
    if (inGhostLevels && inGhostLevels[inPtId] & svtkDataSetAttributes::DUPLICATEPOINT)
    {
      continue;
    }

    if (inputUG && !inputUG->IsPointVisible(inPtId))
    {
      // input is a svtkUniformGrid and the current point is blanked. Don't glyph
      // it.
      continue;
    }

    if (!this->IsPointVisible(input, inPtId))
    {
      continue;
    }

    // Now begin copying/transforming glyph
    trans->Identity();

    // Copy all topology (transformation independent)
    for (cellId = 0; cellId < numSourceCells; cellId++)
    {
      source->GetCellPoints(cellId, pointIdList);
      cellPts = pointIdList;
      npts = cellPts->GetNumberOfIds();
      for (pts->Reset(), i = 0; i < npts; i++)
      {
        pts->InsertId(i, cellPts->GetId(i) + ptIncr);
      }
      output->InsertNextCell(source->GetCellType(cellId), pts);
    }

    // translate Source to Input point
    input->GetPoint(inPtId, x);
    trans->Translate(x[0], x[1], x[2]);

    if (haveVectors)
    {
      // Copy Input vector
      for (i = 0; i < numSourcePts; i++)
      {
        newVectors->InsertTuple(i + ptIncr, v);
      }
      if (this->Orient && (vMag > 0.0))
      {
        // if there is no y or z component
        if (v[1] == 0.0 && v[2] == 0.0)
        {
          if (v[0] < 0) // just flip x if we need to
          {
            trans->RotateWXYZ(180.0, 0, 1, 0);
          }
        }
        else
        {
          vNew[0] = (v[0] + vMag) / 2.0;
          vNew[1] = v[1] / 2.0;
          vNew[2] = v[2] / 2.0;
          trans->RotateWXYZ(180.0, vNew[0], vNew[1], vNew[2]);
        }
      }
    }

    if (haveTCoords)
    {
      for (i = 0; i < numSourcePts; i++)
      {
        sourceTCoords->GetTuple(i, tc);
        newTCoords->InsertTuple(i + ptIncr, tc);
      }
    }

    // determine scale factor from scalars if appropriate
    // Copy scalar value
    if (inSScalars && (this->ColorMode == SVTK_COLOR_BY_SCALE))
    {
      for (i = 0; i < numSourcePts; i++)
      {
        newScalars->InsertTuple(i + ptIncr, &scalex); // = scaley = scalez
      }
    }
    else if (inCScalars && (this->ColorMode == SVTK_COLOR_BY_SCALAR))
    {
      for (i = 0; i < numSourcePts; i++)
      {
        outputPD->CopyTuple(inCScalars, newScalars, inPtId, ptIncr + i);
      }
    }
    if (haveVectors && this->ColorMode == SVTK_COLOR_BY_VECTOR)
    {
      for (i = 0; i < numSourcePts; i++)
      {
        newScalars->InsertTuple(i + ptIncr, &vMag);
      }
    }

    // scale data if appropriate
    if (this->Scaling)
    {
      if (this->ScaleMode == SVTK_DATA_SCALING_OFF)
      {
        scalex = scaley = scalez = this->ScaleFactor;
      }
      else
      {
        scalex *= this->ScaleFactor;
        scaley *= this->ScaleFactor;
        scalez *= this->ScaleFactor;
      }

      if (scalex == 0.0)
      {
        scalex = 1.0e-10;
      }
      if (scaley == 0.0)
      {
        scaley = 1.0e-10;
      }
      if (scalez == 0.0)
      {
        scalez = 1.0e-10;
      }
      trans->Scale(scalex, scaley, scalez);
    }

    // multiply points and normals by resulting matrix
    if (this->SourceTransform)
    {
      transformedSourcePts->Reset();
      this->SourceTransform->TransformPoints(sourcePts, transformedSourcePts);
      trans->TransformPoints(transformedSourcePts, newPts);
    }
    else
    {
      trans->TransformPoints(sourcePts, newPts);
    }

    if (haveNormals)
    {
      trans->TransformNormals(sourceNormals, newNormals);
    }

    // Copy point data from source (if possible)
    if (pd)
    {
      for (i = 0; i < numSourcePts; ++i)
      {
        srcPointIdList->SetId(i, inPtId);
        dstPointIdList->SetId(i, ptIncr + i);
      }
      outputPD->CopyData(pd, srcPointIdList, dstPointIdList);
      if (this->FillCellData)
      {
        for (i = 0; i < numSourceCells; ++i)
        {
          srcCellIdList->SetId(i, inPtId);
          dstCellIdList->SetId(i, cellIncr + i);
        }
        outputCD->CopyData(pd, srcCellIdList, dstCellIdList);
      }
    }

    // If point ids are to be generated, do it here
    if (this->GeneratePointIds)
    {
      for (i = 0; i < numSourcePts; i++)
      {
        pointIds->InsertNextValue(inPtId);
      }
    }

    ptIncr += numSourcePts;
    cellIncr += numSourceCells;
  }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newScalars)
  {
    int idx = outputPD->AddArray(newScalars);
    outputPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  if (newVectors)
  {
    outputPD->SetVectors(newVectors);
    newVectors->Delete();
  }

  if (newNormals)
  {
    outputPD->SetNormals(newNormals);
    newNormals->Delete();
  }

  if (newTCoords)
  {
    outputPD->SetTCoords(newTCoords);
    newTCoords->Delete();
  }

  output->Squeeze();
  trans->Delete();
  pts->Delete();

  return true;
}

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void svtkGlyph3D::SetSourceConnection(int id, svtkAlgorithmOutput* algOutput)
{
  if (id < 0)
  {
    svtkErrorMacro("Bad index " << id << " for source.");
    return;
  }

  int numConnections = this->GetNumberOfInputConnections(1);
  if (id < numConnections)
  {
    this->SetNthInputConnection(1, id, algOutput);
  }
  else if (id == numConnections && algOutput)
  {
    this->AddInputConnection(1, algOutput);
  }
  else if (algOutput)
  {
    svtkWarningMacro("The source id provided is larger than the maximum "
                    "source id, using "
      << numConnections << " instead.");
    this->AddInputConnection(1, algOutput);
  }
}

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void svtkGlyph3D::SetSourceData(int id, svtkPolyData* pd)
{
  int numConnections = this->GetNumberOfInputConnections(1);

  if (id < 0 || id > numConnections)
  {
    svtkErrorMacro("Bad index " << id << " for source.");
    return;
  }

  svtkTrivialProducer* tp = nullptr;
  if (pd)
  {
    tp = svtkTrivialProducer::New();
    tp->SetOutput(pd);
  }

  if (id < numConnections)
  {
    if (tp)
    {
      this->SetNthInputConnection(1, id, tp->GetOutputPort());
    }
    else
    {
      this->SetNthInputConnection(1, id, nullptr);
    }
  }
  else if (id == numConnections && tp)
  {
    this->AddInputConnection(1, tp->GetOutputPort());
  }

  if (tp)
  {
    tp->Delete();
  }
}

//----------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
svtkPolyData* svtkGlyph3D::GetSource(int id)
{
  if (id < 0 || id >= this->GetNumberOfInputConnections(1))
  {
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, id));
}

//----------------------------------------------------------------------------
void svtkGlyph3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Point Ids " << (this->GeneratePointIds ? "On\n" : "Off\n");

  os << indent << "PointIdsName: " << (this->PointIdsName ? this->PointIdsName : "(none)") << "\n";

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;

  if (this->GetNumberOfInputConnections(1) < 2)
  {
    if (this->GetSource(0) != nullptr)
    {
      os << indent << "Source: (" << this->GetSource(0) << ")\n";
    }
    else
    {
      os << indent << "Source: (none)\n";
    }
  }
  else
  {
    os << indent << "A table of " << this->GetNumberOfInputConnections(1)
       << " glyphs has been defined\n";
  }

  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");

  os << indent << "Scale Mode: ";
  if (this->ScaleMode == SVTK_SCALE_BY_SCALAR)
  {
    os << "Scale by scalar\n";
  }
  else if (this->ScaleMode == SVTK_SCALE_BY_VECTOR)
  {
    os << "Scale by vector\n";
  }
  else
  {
    os << "Data scaling is turned off\n";
  }

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Clamping: " << (this->Clamping ? "On\n" : "Off\n");
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");
  os << indent << "Orient Mode: "
     << (this->VectorMode == SVTK_USE_VECTOR ? "Orient by vector\n" : "Orient by normal\n");
  os << indent << "Index Mode: ";
  if (this->IndexMode == SVTK_INDEXING_BY_SCALAR)
  {
    os << "Index by scalar value\n";
  }
  else if (this->IndexMode == SVTK_INDEXING_BY_VECTOR)
  {
    os << "Index by vector value\n";
  }
  else
  {
    os << "Indexing off\n";
  }

  os << indent << "Fill Cell Data: " << (this->FillCellData ? "On\n" : "Off\n");

  os << indent << "SourceTransform: ";
  if (this->SourceTransform)
  {
    os << endl;
    this->SourceTransform->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

int svtkGlyph3D::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (sourceInfo)
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
svtkPolyData* svtkGlyph3D::GetSource(int idx, svtkInformationVector* sourceInfo)
{
  svtkInformation* info = sourceInfo->GetInformationObject(idx);
  if (!info)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int svtkGlyph3D::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
    return 1;
  }
  return 0;
}
