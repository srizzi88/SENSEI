/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlyph3DMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGlyph3DMapper.h"

#include "svtkActor.h"
#include "svtkBitArray.h"
#include "svtkBoundingBox.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataSetRange.h"
#include "svtkDataArray.h"
#include "svtkDataObjectTree.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataObjectTreeRange.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTimerLog.h"
#include "svtkTransform.h"
#include "svtkTrivialProducer.h"

#include <cassert>
#include <vector>

namespace
{
int getNumberOfChildren(svtkDataObjectTree* tree)
{
  int result = 0;
  if (tree)
  {
    svtkDataObjectTreeIterator* it = tree->NewTreeIterator();
    it->SetTraverseSubTree(false);
    it->SetVisitOnlyLeaves(false);
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      ++result;
    }
    it->Delete();
  }
  return result;
}
}

// Return nullptr if no override is supplied.
svtkAbstractObjectFactoryNewMacro(svtkGlyph3DMapper);
svtkCxxSetObjectMacro(svtkGlyph3DMapper, BlockAttributes, svtkCompositeDataDisplayAttributes);

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
svtkGlyph3DMapper::svtkGlyph3DMapper()
{
  this->SetNumberOfInputPorts(2);

  this->BlockAttributes = nullptr;
  this->Scaling = true;
  this->ScaleMode = NO_DATA_SCALING;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = true;
  this->Clamping = false;
  this->SourceIndexing = false;
  this->UseSourceTableTree = false;
  this->UseSelectionIds = false;
  this->OrientationMode = svtkGlyph3DMapper::DIRECTION;

  // Set default arrays.
  this->SetScaleArray(svtkDataSetAttributes::SCALARS);
  this->SetMaskArray(svtkDataSetAttributes::SCALARS);
  this->SetOrientationArray(svtkDataSetAttributes::VECTORS);
  this->SetSelectionIdArray(svtkDataSetAttributes::SCALARS);

  this->Masking = false;
  this->SelectionColorId = 1;
}

// ---------------------------------------------------------------------------
svtkGlyph3DMapper::~svtkGlyph3DMapper()
{
  this->SetBlockAttributes(nullptr);
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetMaskArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(
    svtkGlyph3DMapper::MASK, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetMaskArray(const char* maskarrayname)
{
  this->SetInputArrayToProcess(
    svtkGlyph3DMapper::MASK, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, maskarrayname);
}

// ---------------------------------------------------------------------------
svtkDataArray* svtkGlyph3DMapper::GetMaskArray(svtkDataSet* input)
{
  if (this->Masking)
  {
    int association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
    return this->GetInputArrayToProcess(svtkGlyph3DMapper::MASK, input, association);
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetOrientationArray(const char* orientationarrayname)
{
  this->SetInputArrayToProcess(svtkGlyph3DMapper::ORIENTATION, 0, 0,
    svtkDataObject::FIELD_ASSOCIATION_POINTS, orientationarrayname);
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetOrientationArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(svtkGlyph3DMapper::ORIENTATION, 0, 0,
    svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
svtkDataArray* svtkGlyph3DMapper::GetOrientationArray(svtkDataSet* input)
{
  if (this->Orient)
  {
    int association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
    return this->GetInputArrayToProcess(svtkGlyph3DMapper::ORIENTATION, input, association);
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetScaleArray(const char* scalarsarrayname)
{
  this->SetInputArrayToProcess(
    svtkGlyph3DMapper::SCALE, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, scalarsarrayname);
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetScaleArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(
    svtkGlyph3DMapper::SCALE, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
svtkDataArray* svtkGlyph3DMapper::GetScaleArray(svtkDataSet* input)
{
  if (this->Scaling && this->ScaleMode != svtkGlyph3DMapper::NO_DATA_SCALING)
  {
    int association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
    svtkDataArray* arr = this->GetInputArrayToProcess(svtkGlyph3DMapper::SCALE, input, association);
    return arr;
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetSourceIndexArray(const char* arrayname)
{
  this->SetInputArrayToProcess(
    svtkGlyph3DMapper::SOURCE_INDEX, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, arrayname);
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetSourceIndexArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(svtkGlyph3DMapper::SOURCE_INDEX, 0, 0,
    svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
svtkDataArray* svtkGlyph3DMapper::GetSourceIndexArray(svtkDataSet* input)
{
  if (this->SourceIndexing)
  {
    int association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
    return this->GetInputArrayToProcess(svtkGlyph3DMapper::SOURCE_INDEX, input, association);
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetSelectionIdArray(const char* selectionIdArrayName)
{
  this->SetInputArrayToProcess(svtkGlyph3DMapper::SELECTIONID, 0, 0,
    svtkDataObject::FIELD_ASSOCIATION_POINTS, selectionIdArrayName);
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetSelectionIdArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(svtkGlyph3DMapper::SELECTIONID, 0, 0,
    svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
svtkDataArray* svtkGlyph3DMapper::GetSelectionIdArray(svtkDataSet* input)
{
  if (this->UseSelectionIds)
  {
    int association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
    svtkDataArray* arr =
      this->GetInputArrayToProcess(svtkGlyph3DMapper::SELECTIONID, input, association);
    return arr;
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
svtkUnsignedCharArray* svtkGlyph3DMapper::GetColors(svtkDataSet* input)
{
  return svtkArrayDownCast<svtkUnsignedCharArray>(input->GetPointData()->GetScalars());
}

// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void svtkGlyph3DMapper::SetSourceConnection(int idx, svtkAlgorithmOutput* algOutput)
{
  if (idx < 0)
  {
    svtkErrorMacro("Bad index " << idx << " for source.");
    return;
  }

  int numConnections = this->GetNumberOfInputConnections(1);
  if (idx < numConnections)
  {
    this->SetNthInputConnection(1, idx, algOutput);
  }
  else if (idx == numConnections && algOutput)
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

// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void svtkGlyph3DMapper::SetSourceData(int idx, svtkPolyData* pd)
{
  int numConnections = this->GetNumberOfInputConnections(1);

  if (idx < 0 || idx > numConnections)
  {
    svtkErrorMacro("Bad index " << idx << " for source.");
    return;
  }

  svtkTrivialProducer* tp = nullptr;
  if (pd)
  {
    tp = svtkTrivialProducer::New();
    tp->SetOutput(pd);
  }

  if (idx < numConnections)
  {
    if (tp)
    {
      this->SetNthInputConnection(1, idx, tp->GetOutputPort());
    }
    else
    {
      this->SetNthInputConnection(1, idx, nullptr);
    }
  }
  else if (idx == numConnections && tp)
  {
    this->AddInputConnection(1, tp->GetOutputPort());
  }

  if (tp)
  {
    tp->Delete();
  }
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetSourceTableTree(svtkDataObjectTree* tree)
{
  svtkNew<svtkTrivialProducer> tp;
  tp->SetOutput(tree);
  this->SetNumberOfInputConnections(1, 1);
  this->SetInputConnection(1, tp->GetOutputPort());
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetSourceData(svtkPolyData* pd)
{
  this->SetSourceData(0, pd);
}

// ---------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
svtkPolyData* svtkGlyph3DMapper::GetSource(int idx)
{
  if (idx < 0 || idx >= this->GetNumberOfInputConnections(1))
  {
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, idx));
}

// ---------------------------------------------------------------------------
svtkDataObjectTree* svtkGlyph3DMapper::GetSourceTableTree()
{
  return this->UseSourceTableTree
    ? svtkDataObjectTree::SafeDownCast(this->GetExecutive()->GetInputData(1, 0))
    : nullptr;
}

// ---------------------------------------------------------------------------
svtkPolyData* svtkGlyph3DMapper::GetSource(int idx, svtkInformationVector* sourceInfo)
{
  svtkInformation* info = sourceInfo->GetInformationObject(idx);
  if (!info)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
}

// ---------------------------------------------------------------------------
const char* svtkGlyph3DMapper::GetOrientationModeAsString()
{
  switch (this->OrientationMode)
  {
    case svtkGlyph3DMapper::DIRECTION:
      return "Direction";
    case svtkGlyph3DMapper::ROTATION:
      return "Rotation";
    case svtkGlyph3DMapper::QUATERNION:
      return "Quaternion";
  }
  return "Invalid";
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->UseSourceTableTree)
  {
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
  }
  else
  {
    os << indent << "SourceTableTree: (" << this->GetSourceTableTree() << ")\n";
  }

  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");

  os << indent << "Scale Mode: " << this->GetScaleModeAsString() << endl;
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Clamping: " << (this->Clamping ? "On\n" : "Off\n");
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");
  os << indent << "OrientationMode: " << this->GetOrientationModeAsString() << "\n";
  os << indent << "SourceIndexing: " << (this->SourceIndexing ? "On" : "Off") << endl;
  os << indent << "UseSourceTableTree: " << (this->UseSourceTableTree ? "On" : "Off") << endl;
  os << indent << "UseSelectionIds: " << (this->UseSelectionIds ? "On" : "Off") << endl;
  os << indent << "SelectionColorId: " << this->SelectionColorId << endl;
  os << indent << "Masking: " << (this->Masking ? "On" : "Off") << endl;
  os << indent << "BlockAttributes: (" << this->BlockAttributes << ")" << endl;
  if (this->BlockAttributes)
  {
    this->BlockAttributes->PrintSelf(os, indent.GetNextIndent());
  }
}

// ---------------------------------------------------------------------------
int svtkGlyph3DMapper::RequestUpdateExtent(
  svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector, svtkInformationVector*)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);

  if (sourceInfo)
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

// ---------------------------------------------------------------------------
int svtkGlyph3DMapper::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObjectTree");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
    return 1;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// Description:
// Return the method of scaling as a descriptive character string.
const char* svtkGlyph3DMapper::GetScaleModeAsString()
{
  if (this->ScaleMode == SCALE_BY_MAGNITUDE)
  {
    return "ScaleByMagnitude";
  }
  else if (this->ScaleMode == SCALE_BY_COMPONENTS)
  {
    return "ScaleByVectorComponents";
  }

  return "NoDataScaling";
}

//-------------------------------------------------------------------------
bool svtkGlyph3DMapper::GetBoundsInternal(svtkDataSet* ds, double ds_bounds[6])
{
  if (ds == nullptr)
  {
    return false;
  }

  ds->GetBounds(ds_bounds);
  // if there is nothing inside the scene, just return uninitializedBounds
  if ((ds_bounds[0] > ds_bounds[1]) && (ds_bounds[2] > ds_bounds[3]) &&
    (ds_bounds[4] > ds_bounds[5]))
  {
    return false;
  }
  // if the input is not conform to what the mapper expects (use vector
  // but no vector data), nothing will be mapped.
  // It make sense to return uninitialized bounds.

  svtkDataArray* scaleArray = this->GetScaleArray(ds);
  svtkDataArray* orientArray = this->GetOrientationArray(ds);
  // TODO:
  // 1. cumulative bbox of all the glyph
  // 2. scale it by scale factor and maximum scalar value (or vector mag)
  // 3. enlarge the input bbox half-way in each direction with the
  // glyphs bbox.

  double den = this->Range[1] - this->Range[0];
  if (den == 0.0)
  {
    den = 1.0;
  }

  if (!this->UseSourceTableTree && this->GetSource(0) == nullptr)
  {
    svtkPolyData* defaultSource = svtkPolyData::New();
    defaultSource->AllocateEstimate(0, 0, 1, 2, 0, 0, 0, 0);
    svtkPoints* defaultPoints = svtkPoints::New();
    defaultPoints->Allocate(6);
    defaultPoints->InsertNextPoint(0, 0, 0);
    defaultPoints->InsertNextPoint(1, 0, 0);
    svtkIdType defaultPointIds[2];
    defaultPointIds[0] = 0;
    defaultPointIds[1] = 1;
    defaultSource->SetPoints(defaultPoints);
    defaultSource->InsertNextCell(SVTK_LINE, 2, defaultPointIds);
    defaultSource->Delete();
    defaultSource = nullptr;
    defaultPoints->Delete();
    defaultPoints = nullptr;
  }

  // FB

  // Compute indexRange.
  svtkDataObjectTree* sourceTableTree = this->GetSourceTableTree();
  int numberOfSources = this->UseSourceTableTree ? getNumberOfChildren(sourceTableTree)
                                                 : this->GetNumberOfInputConnections(1);

  if (numberOfSources < 1)
  {
    return true; // just return the dataset bounds.
  }

  int indexRange[2] = { 0, 0 };
  svtkDataArray* indexArray = this->GetSourceIndexArray(ds);
  if (indexArray)
  {
    double range[2];
    indexArray->GetRange(range, -1);
    for (int i = 0; i < 2; i++)
    {
      indexRange[i] = static_cast<int>((range[i] - this->Range[0]) * numberOfSources / den);
      indexRange[i] = svtkMath::ClampValue(indexRange[i], 0, numberOfSources - 1);
    }
  }

  svtkBoundingBox bbox; // empty

  double xScaleRange[2] = { 1.0, 1.0 };
  double yScaleRange[2] = { 1.0, 1.0 };
  double zScaleRange[2] = { 1.0, 1.0 };

  if (scaleArray)
  {
    switch (this->ScaleMode)
    {
      case SCALE_BY_MAGNITUDE:
        scaleArray->GetRange(xScaleRange, -1);
        yScaleRange[0] = xScaleRange[0];
        yScaleRange[1] = xScaleRange[1];
        zScaleRange[0] = xScaleRange[0];
        zScaleRange[1] = xScaleRange[1];
        break;

      case SCALE_BY_COMPONENTS:
        scaleArray->GetRange(xScaleRange, 0);
        scaleArray->GetRange(yScaleRange, 1);
        scaleArray->GetRange(zScaleRange, 2);
        break;

      default:
        // NO_DATA_SCALING: do nothing, set variables to avoid warnings.
        break;
    }

    if (this->Clamping && this->ScaleMode != NO_DATA_SCALING)
    {
      xScaleRange[0] = svtkMath::ClampAndNormalizeValue(xScaleRange[0], this->Range);
      xScaleRange[1] = svtkMath::ClampAndNormalizeValue(xScaleRange[1], this->Range);
      yScaleRange[0] = svtkMath::ClampAndNormalizeValue(yScaleRange[0], this->Range);
      yScaleRange[1] = svtkMath::ClampAndNormalizeValue(yScaleRange[1], this->Range);
      zScaleRange[0] = svtkMath::ClampAndNormalizeValue(zScaleRange[0], this->Range);
      zScaleRange[1] = svtkMath::ClampAndNormalizeValue(zScaleRange[1], this->Range);
    }
  }

  if (this->UseSourceTableTree)
  {
    if (sourceTableTree)
    {
      auto sTTRange = svtk::Range(sourceTableTree);
      auto sTTIter = sTTRange.begin();

      // Advance to first indexed dataset:
      int idx = indexRange[0];
      std::advance(sTTIter, idx);

      // Add the bounds from the appropriate datasets:
      while (idx <= indexRange[1])
      {
        svtkDataObject* sourceDObj = *sTTIter;

        // The source table tree may have composite nodes:
        svtkCompositeDataSet* sourceCDS = svtkCompositeDataSet::SafeDownCast(sourceDObj);
        svtkCompositeDataIterator* sourceIter = nullptr;
        if (sourceCDS)
        {
          sourceIter = sourceCDS->NewIterator();
          sourceIter->SetSkipEmptyNodes(true);
          sourceIter->InitTraversal();
        }

        // Or, it may just have polydata:
        svtkPolyData* sourcePD = svtkPolyData::SafeDownCast(sourceDObj);

        for (;;)
        {
          // Extract the polydata from the composite dataset if it exists:
          if (sourceIter)
          {
            sourcePD = svtkPolyData::SafeDownCast(sourceIter->GetCurrentDataObject());
          }

          // Get the bounds of the current dataset:
          if (sourcePD)
          {
            double bounds[6];
            sourcePD->GetBounds(bounds);
            if (svtkMath::AreBoundsInitialized(bounds))
            {
              bbox.AddBounds(bounds);
            }
          }

          // Advance the composite source iterator if it exists:
          if (sourceIter)
          {
            sourceIter->GoToNextItem();
          }

          // If the sourceDObj is not composite, or we've exhausted the
          // iterator, break the loop.
          if (!sourceIter || sourceIter->IsDoneWithTraversal())
          {
            break;
          }
        }

        if (sourceIter)
        {
          sourceIter->Delete();
          sourceIter = nullptr;
        }

        // Move to the next node in the source table tree.
        ++sTTIter;
        ++idx;
      }
    }
  }
  else // non-source-table-tree table
  {
    int index = indexRange[0];
    while (index <= indexRange[1])
    {
      svtkPolyData* source = this->GetSource(index);
      // Make sure we're not indexing into empty glyph
      if (source != nullptr)
      {
        double bounds[6];
        source->GetBounds(bounds); // can be invalid/uninitialized
        if (svtkMath::AreBoundsInitialized(bounds))
        {
          bbox.AddBounds(bounds);
        }
      }
      ++index;
    }
  }

  if (this->Scaling)
  {
    svtkBoundingBox bbox2(bbox);
    bbox.Scale(xScaleRange[0], yScaleRange[0], zScaleRange[0]);
    bbox2.Scale(xScaleRange[1], yScaleRange[1], zScaleRange[1]);
    bbox.AddBox(bbox2);
    bbox.Scale(this->ScaleFactor, this->ScaleFactor, this->ScaleFactor);
  }

  if (bbox.IsValid())
  {
    double bounds[6];
    if (orientArray)
    {
      svtkBoundingBox bbox2(bbox);
      bbox2.Scale(-1.0, -1.0, -1.0);
      bbox.AddBox(bbox2);
      // bounding sphere.
      double l = bbox.GetDiagonalLength() / 2.0;
      bounds[0] = -l;
      bounds[1] = l;
      bounds[2] = -l;
      bounds[3] = l;
      bounds[4] = -l;
      bounds[5] = l;
    }
    else
    {
      bbox.GetBounds(bounds);
    }
    int j = 0;
    while (j < 6)
    {
      ds_bounds[j] += bounds[j];
      ++j;
    }
  }
  else
  {
    return false;
  }

  return true;
}

//-------------------------------------------------------------------------
double* svtkGlyph3DMapper::GetBounds()
{
  //  static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};
  svtkMath::UninitializeBounds(this->Bounds);

  // do we have an input
  if (!this->GetNumberOfInputConnections(0))
  {
    return this->Bounds;
  }
  if (!this->Static)
  {
    // For proper clipping, this would be this->Piece,
    // this->NumberOfPieces.
    // But that removes all benefites of streaming.
    // Update everything as a hack for paraview streaming.
    // This should not affect anything else, because no one uses this.
    // It should also render just the same.
    // Just remove this lie if we no longer need streaming in paraview :)

    // first get the bounds from the input
    this->Update();
  }

  svtkDataObject* dobj = this->GetInputDataObject(0, 0);
  svtkDataSet* ds = svtkDataSet::SafeDownCast(dobj);
  if (ds)
  {
    this->GetBoundsInternal(ds, this->Bounds);
    return this->Bounds;
  }

  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(dobj);
  if (!cd)
  {
    return this->Bounds;
  }

  svtkBoundingBox bbox;

  using Opts = svtk::CompositeDataSetOptions;
  for (svtkDataObject* dObj : svtk::Range(cd, Opts::SkipEmptyNodes))
  {
    ds = svtkDataSet::SafeDownCast(dObj);
    if (ds)
    {
      double tmpBounds[6];
      this->GetBoundsInternal(ds, tmpBounds);
      bbox.AddBounds(tmpBounds);
    }
  }
  bbox.GetBounds(this->Bounds);

  return this->Bounds;
}

//-------------------------------------------------------------------------
void svtkGlyph3DMapper::GetBounds(double bounds[6])
{
  this->Superclass::GetBounds(bounds);
}

// ---------------------------------------------------------------------------
void svtkGlyph3DMapper::Render(svtkRenderer*, svtkActor*)
{
  cerr << "Calling wrong render method!!\n";
}

//---------------------------------------------------------------------------
void svtkGlyph3DMapper::SetInputData(svtkDataObject* input)
{
  this->SetInputDataInternal(0, input);
}

//---------------------------------------------------------------------------
svtkIdType svtkGlyph3DMapper::GetMaxNumberOfLOD()
{
  return 0;
}
