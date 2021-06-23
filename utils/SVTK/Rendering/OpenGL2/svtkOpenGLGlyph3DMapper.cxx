/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGlyph3DMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLGlyph3DMapper.h"

#include "svtkActor.h"
#include "svtkBitArray.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataSetRange.h"
#include "svtkDataObjectTree.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkHardwareSelector.h"
#include "svtkMath.h"
#include "svtkMatrix3x3.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLGlyph3DHelper.h"
#include "svtkProperty.h"
#include "svtkQuaternion.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTransform.h"
#include "svtkUnsignedCharArray.h"

#include <map>

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

svtkDataObject* getChildDataObject(svtkDataObjectTree* tree, int child)
{
  svtkDataObject* result = nullptr;
  if (tree)
  {
    svtkDataObjectTreeIterator* it = tree->NewTreeIterator();
    it->SetTraverseSubTree(false);
    it->SetVisitOnlyLeaves(false);
    it->InitTraversal();
    for (int i = 0; i < child; ++i)
    {
      it->GoToNextItem();
    }
    result = it->GetCurrentDataObject();
    it->Delete();
  }
  return result;
}
}

class svtkOpenGLGlyph3DMappersvtkColorMapper : public svtkMapper
{
public:
  svtkTypeMacro(svtkOpenGLGlyph3DMappersvtkColorMapper, svtkMapper);
  static svtkOpenGLGlyph3DMappersvtkColorMapper* New();
  void Render(svtkRenderer*, svtkActor*) override {}
  svtkUnsignedCharArray* GetColors() { return this->Colors; }
};

svtkStandardNewMacro(svtkOpenGLGlyph3DMappersvtkColorMapper);

class svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry
{
public:
  std::vector<svtkIdType> PickIds;
  std::vector<unsigned char> Colors;
  std::vector<float> Matrices;       // transposed
  std::vector<float> NormalMatrices; // transposed
  svtkTimeStamp BuildTime;
  // May be polydata or composite dataset:
  svtkDataObject* DataObject;
  // maps composite dataset flat index to polydatamapper. Key = -1 for polydata
  // DataObject.
  typedef std::map<int, svtkOpenGLGlyph3DHelper*> MapperMap;
  MapperMap Mappers;
  int NumberOfPoints;

  svtkOpenGLGlyph3DMapperEntry()
  {
    this->NumberOfPoints = 0;
    this->DataObject = nullptr;
  };
  ~svtkOpenGLGlyph3DMapperEntry()
  {
    this->ClearMappers();
    if (this->DataObject)
    {
      this->DataObject->Delete();
    }
  };
  void ClearMappers()
  {
    for (MapperMap::iterator it = this->Mappers.begin(); it != this->Mappers.end(); ++it)
    {
      it->second->Delete();
    }
    this->Mappers.clear();
  }
};

class svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray
{
public:
  std::vector<svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry*> Entries;
  svtkTimeStamp BuildTime;
  svtkOpenGLGlyph3DMapperSubArray() = default;
  ~svtkOpenGLGlyph3DMapperSubArray() { this->ClearEntries(); };
  void ClearEntries()
  {
    std::vector<svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry*>::iterator miter =
      this->Entries.begin();
    for (; miter != this->Entries.end(); ++miter)
    {
      delete *miter;
    }
    this->Entries.clear();
  }
};

class svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperArray
{
public:
  std::map<const svtkDataSet*, svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray*> Entries;
  ~svtkOpenGLGlyph3DMapperArray()
  {
    std::map<const svtkDataSet*, svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray*>::iterator
      miter = this->Entries.begin();
    for (; miter != this->Entries.end(); ++miter)
    {
      delete miter->second;
    }
    this->Entries.clear();
  };
};

svtkStandardNewMacro(svtkOpenGLGlyph3DMapper);

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapper()
{
  this->GlyphValues = new svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperArray();
  this->ColorMapper = svtkOpenGLGlyph3DMappersvtkColorMapper::New();
}

// ---------------------------------------------------------------------------
svtkOpenGLGlyph3DMapper::~svtkOpenGLGlyph3DMapper()
{
  this->ColorMapper->Delete();

  delete this->GlyphValues;
  this->GlyphValues = nullptr;
}

// ---------------------------------------------------------------------------
// Description:
// Send mapper ivars to sub-mapper.
// \pre mapper_exists: mapper!=0
void svtkOpenGLGlyph3DMapper::CopyInformationToSubMapper(svtkOpenGLGlyph3DHelper* mapper)
{
  assert("pre: mapper_exists" && mapper != nullptr);
  mapper->SetStatic(this->Static);
  mapper->ScalarVisibilityOff();
  // not used
  mapper->SetClippingPlanes(this->ClippingPlanes);

  mapper->SetResolveCoincidentTopology(this->GetResolveCoincidentTopology());
  mapper->SetResolveCoincidentTopologyZShift(this->GetResolveCoincidentTopologyZShift());

  double f, u;
  this->GetRelativeCoincidentTopologyPolygonOffsetParameters(f, u);
  mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(f, u);
  this->GetRelativeCoincidentTopologyLineOffsetParameters(f, u);
  mapper->SetRelativeCoincidentTopologyLineOffsetParameters(f, u);
  this->GetRelativeCoincidentTopologyPointOffsetParameter(u);
  mapper->SetRelativeCoincidentTopologyPointOffsetParameter(u);

  // ResolveCoincidentTopologyPolygonOffsetParameters is static
  mapper->SetResolveCoincidentTopologyPolygonOffsetFaces(
    this->GetResolveCoincidentTopologyPolygonOffsetFaces());

  if (static_cast<svtkIdType>(this->LODs.size()) > this->GetMaxNumberOfLOD())
  {
    svtkWarningMacro(<< "too many LODs are defined, "
                    << (static_cast<svtkIdType>(this->LODs.size()) - this->GetMaxNumberOfLOD())
                    << " last defined LODs are discarded.");
    this->LODs.resize(this->GetMaxNumberOfLOD());
  }

  mapper->SetLODs(this->LODs);
  mapper->SetLODColoring(this->LODColoring);
}

void svtkOpenGLGlyph3DMapper::SetupColorMapper()
{
  this->ColorMapper->ShallowCopy(this);
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void svtkOpenGLGlyph3DMapper::Render(svtkRenderer* ren, svtkActor* actor)
{
  svtkOpenGLClearErrorMacro();

  this->SetupColorMapper();

  svtkHardwareSelector* selector = ren->GetSelector();

  if (selector)
  {
    selector->BeginRenderProp();
  }

  svtkDataObject* inputDO = this->GetInputDataObject(0, 0);

  // Check input for consistency
  //
  // Create a default source, if no source is specified.
  if (!this->UseSourceTableTree && this->GetSource(0) == nullptr)
  {
    svtkPolyData* defaultSource = svtkPolyData::New();
    defaultSource->AllocateEstimate(0, 0, 1, 2, 0, 0, 0, 0);
    svtkPoints* defaultPoints = svtkPoints::New();
    defaultPoints->Allocate(6);
    defaultPoints->InsertNextPoint(0., 0., 0.);
    defaultPoints->InsertNextPoint(1., 0., 0.);
    svtkIdType defaultPointIds[2];
    defaultPointIds[0] = 0;
    defaultPointIds[1] = 1;
    defaultSource->SetPoints(defaultPoints);
    defaultSource->InsertNextCell(SVTK_LINE, 2, defaultPointIds);
    this->SetSourceData(defaultSource);
    defaultSource->Delete();
    defaultSource = nullptr;
    defaultPoints->Delete();
    defaultPoints = nullptr;
  }

  // Check that source configuration is sane:
  svtkDataObjectTree* sourceTableTree = this->GetSourceTableTree();
  int numSourceDataSets = this->GetNumberOfInputConnections(1);
  if (this->UseSourceTableTree)
  {
    if (numSourceDataSets > 1)
    {
      svtkErrorMacro("UseSourceTableTree is true, but multiple source datasets "
                    "are set.");
      return;
    }
    if (!sourceTableTree)
    {
      svtkErrorMacro("UseSourceTableTree is true, but the source dataset is "
                    "not a svtkDataObjectTree.");
      return;
    }
    svtkDataObjectTreeIterator* it = sourceTableTree->NewTreeIterator();
    it->SetTraverseSubTree(false);
    it->SetVisitOnlyLeaves(false);
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      svtkDataObject* node = it->GetCurrentDataObject();
      if (!node->IsA("svtkPolyData") && !node->IsA("svtkCompositeDataSet"))
      {
        svtkErrorMacro("The source table tree must only contain svtkPolyData "
                      "or svtkCompositeDataSet children, but found a "
          << node->GetClassName() << ".");
        it->Delete();
        return;
      }
    }
    it->Delete();
  }
  else
  {
    for (int i = 0; i < numSourceDataSets; ++i)
    {
      if (!this->GetSource(i))
      {
        svtkErrorMacro("Source input at index " << i
                                               << " not set, or not "
                                                  "svtkPolyData.");
        return;
      }
    }
  }

  // Render the input dataset or every dataset in the input composite dataset.
  this->BlockMTime = this->BlockAttributes ? this->BlockAttributes->GetMTime() : 0;
  svtkDataSet* ds = svtkDataSet::SafeDownCast(inputDO);
  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(inputDO);
  if (ds)
  {
    this->Render(ren, actor, ds);
  }
  else if (cd)
  {
    svtkNew<svtkActor> blockAct;
    svtkNew<svtkProperty> blockProp;
    blockAct->ShallowCopy(actor);
    blockProp->DeepCopy(blockAct->GetProperty());
    blockAct->SetProperty(blockProp.GetPointer());
    double origColor[4];
    blockProp->GetColor(origColor);

    using Opts = svtk::CompositeDataSetOptions;
    for (auto node : svtk::Range(cd, Opts::SkipEmptyNodes))
    {
      auto curIndex = node.GetFlatIndex();
      auto currentObj = node.GetDataObject();

      // Skip invisible blocks and unpickable ones when performing selection:
      bool blockVis =
        (this->BlockAttributes && this->BlockAttributes->HasBlockVisibility(currentObj))
        ? this->BlockAttributes->GetBlockVisibility(currentObj)
        : true;
      bool blockPick =
        (this->BlockAttributes && this->BlockAttributes->HasBlockPickability(currentObj))
        ? this->BlockAttributes->GetBlockPickability(currentObj)
        : true;
      if (!blockVis || (selector && !blockPick))
      {
        continue;
      }
      ds = svtkDataSet::SafeDownCast(currentObj);
      if (ds)
      {
        if (selector)
        {
          selector->RenderCompositeIndex(curIndex);
        }
        else if (this->BlockAttributes && this->BlockAttributes->HasBlockColor(currentObj))
        {
          double color[3];
          this->BlockAttributes->GetBlockColor(currentObj, color);
          blockProp->SetColor(color);
        }
        else
        {
          blockProp->SetColor(origColor);
        }
        this->Render(ren, blockAct.GetPointer(), ds);
      }
    }
  }

  if (selector)
  {
    selector->EndRenderProp();
  }

  svtkOpenGLCheckErrorMacro("Failed after Render");

  this->UpdateProgress(1.0);
}

// ---------------------------------------------------------------------------
void svtkOpenGLGlyph3DMapper::Render(svtkRenderer* ren, svtkActor* actor, svtkDataSet* dataset)
{
  svtkIdType numPts = dataset->GetNumberOfPoints();
  if (numPts < 1)
  {
    svtkDebugMacro(<< "No points to glyph!");
    return;
  }

  // make sure we have an entry for this dataset
  svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray* subarray;
  bool rebuild = false;
  typedef std::map<const svtkDataSet*,
    svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray*>::iterator GVIter;
  GVIter found = this->GlyphValues->Entries.find(dataset);
  if (found == this->GlyphValues->Entries.end())
  {
    subarray = new svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray();
    this->GlyphValues->Entries.insert(std::make_pair(dataset, subarray));
    rebuild = true;
  }
  else
  {
    subarray = found->second;
  }

  // make sure we have a subentry for each source
  svtkDataObjectTree* sourceTableTree = this->GetSourceTableTree();
  int sTTSize = getNumberOfChildren(sourceTableTree);
  int numSourceDataSets = this->GetNumberOfInputConnections(1);
  size_t numberOfSources = this->UseSourceTableTree ? sTTSize : numSourceDataSets;
  bool numberOfSourcesChanged = false;
  if (numberOfSources != subarray->Entries.size())
  {
    subarray->ClearEntries();
    for (size_t cc = 0; cc < numberOfSources; cc++)
    {
      subarray->Entries.push_back(new svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry());
    }
    numberOfSourcesChanged = true;
  }

  // make sure sources are up to date
  svtkDataObjectTreeIterator* sTTIter = nullptr;
  if (sourceTableTree)
  {
    sTTIter = sourceTableTree->NewTreeIterator();
    sTTIter->SetTraverseSubTree(false);
    sTTIter->SetVisitOnlyLeaves(false);
    sTTIter->InitTraversal();
  }
  for (size_t cc = 0; cc < subarray->Entries.size(); cc++)
  {
    svtkDataObject* s = this->UseSourceTableTree ? sTTIter->GetCurrentDataObject()
                                                : this->GetSource(static_cast<int>(cc));

    svtkOpenGLGlyph3DMapperEntry* entry = subarray->Entries[cc];
    svtkDataObject* ss = entry->DataObject;
    if (ss && !ss->IsA(s->GetClassName()))
    {
      ss->Delete();
      ss = nullptr;
    }
    if (!ss)
    {
      ss = s->NewInstance();
      entry->DataObject = ss;
    }
    if (numberOfSourcesChanged || s->GetMTime() > ss->GetMTime() ||
      this->GetMTime() > entry->BuildTime)
    {
      ss->ShallowCopy(s);
      entry->ClearMappers();
    }

    // Create/update the helper mappers:
    svtkCompositeDataIterator* cdsIter = nullptr;
    if (svtkCompositeDataSet* cds = svtkCompositeDataSet::SafeDownCast(ss))
    {
      cdsIter = cds->NewIterator();
      cdsIter->InitTraversal();
    }

    for (;;)
    {
      svtkOpenGLGlyph3DHelper* mapper = nullptr;

      int mapperIdx = cdsIter ? static_cast<int>(cdsIter->GetCurrentFlatIndex()) : -1;
      svtkOpenGLGlyph3DMapperEntry::MapperMap::iterator mapIter = entry->Mappers.find(mapperIdx);
      if (mapIter == entry->Mappers.end())
      {
        mapper = svtkOpenGLGlyph3DHelper::New();
        entry->Mappers.insert(std::make_pair(mapperIdx, mapper));
      }
      else
      {
        mapper = mapIter->second;
      }
      this->CopyInformationToSubMapper(mapper);

      if (cdsIter)
      {
        cdsIter->GoToNextItem();
      }

      if (!cdsIter || cdsIter->IsDoneWithTraversal())
      {
        break;
      }
    }

    if (cdsIter)
    {
      cdsIter->Delete();
      cdsIter = nullptr;
    }

    if (sTTIter)
    {
      sTTIter->GoToNextItem();
    }
  }
  if (sTTIter)
  {
    sTTIter->Delete();
    sTTIter = nullptr;
  }

  // rebuild all entries for this DataSet if it
  // has been modified
  if (subarray->BuildTime < dataset->GetMTime() || subarray->BuildTime < this->GetMTime() ||
    subarray->BuildTime < this->BlockMTime)
  {
    rebuild = true;
  }

  // get the mask array
  svtkBitArray* maskArray = nullptr;
  if (this->Masking)
  {
    maskArray = svtkArrayDownCast<svtkBitArray>(this->GetMaskArray(dataset));
    if (maskArray == nullptr)
    {
      svtkDebugMacro(<< "masking is enabled but there is no mask array. Ignore masking.");
    }
    else
    {
      if (maskArray->GetNumberOfComponents() != 1)
      {
        svtkErrorMacro(" expecting a mask array with one component, getting "
          << maskArray->GetNumberOfComponents() << " components.");
        return;
      }
    }
  }

  // rebuild all sources for this dataset
  if (rebuild)
  {
    this->RebuildStructures(subarray, numPts, actor, dataset, maskArray);
  }

  // for each subarray
  for (size_t cc = 0; cc < subarray->Entries.size(); cc++)
  {
    svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry* entry = subarray->Entries[cc];
    if (entry->NumberOfPoints <= 0)
    {
      continue;
    }

    svtkDataObject* dObj = entry->DataObject;
    svtkPolyData* pd = svtkPolyData::SafeDownCast(dObj);
    svtkCompositeDataSet* cds = pd ? nullptr : svtkCompositeDataSet::SafeDownCast(dObj);

    svtkCompositeDataIterator* cdsIter = nullptr;
    if (cds)
    {
      cdsIter = cds->NewIterator();
      cdsIter->InitTraversal();
    }

    // Either render the polydata, or loop through the composite dataset and
    // render each polydata leaf:
    for (;;)
    {
      int mapperIdx = -1;
      if (cdsIter)
      {
        pd = svtkPolyData::SafeDownCast(cdsIter->GetCurrentDataObject());
        mapperIdx = cdsIter->GetCurrentFlatIndex();
        cdsIter->GoToNextItem();
      }

      if (pd && pd->GetNumberOfPoints() > 0)
      {
        svtkOpenGLGlyph3DHelper* gh = entry->Mappers[mapperIdx];
        gh->CurrentInput = pd;
        gh->GlyphRender(ren, actor, entry->NumberOfPoints, entry->Colors, entry->Matrices,
          entry->NormalMatrices, entry->PickIds, subarray->BuildTime, this->CullingAndLOD);
      }

      if (!cdsIter || cdsIter->IsDoneWithTraversal())
      {
        break;
      }
    } // end composite glyph iteration

    if (cdsIter)
    {
      cdsIter->Delete();
      cdsIter = nullptr;
    }
  } // end entries

  svtkOpenGLCheckErrorMacro("failed after Render");
}

// ---------------------------------------------------------------------------
void svtkOpenGLGlyph3DMapper::RebuildStructures(
  svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray* subarray, svtkIdType numPts,
  svtkActor* actor, svtkDataSet* dataset, svtkBitArray* maskArray)
{
  double den = this->Range[1] - this->Range[0];
  if (den == 0.0)
  {
    den = 1.0;
  }

  unsigned char color[4];
  {
    const double* actorColor = actor->GetProperty()->GetColor();
    for (int i = 0; i != 3; ++i)
    {
      color[i] = static_cast<unsigned char>(actorColor[i] * 255. + 0.5);
    }
    color[3] = static_cast<unsigned char>(actor->GetProperty()->GetOpacity() * 255. + 0.5);
  }

  svtkDataArray* orientArray = this->GetOrientationArray(dataset);
  if (orientArray != nullptr)
  {
    if ((this->OrientationMode == ROTATION || this->OrientationMode == DIRECTION) &&
      orientArray->GetNumberOfComponents() != 3)
    {
      svtkErrorMacro(" expecting an orientation array with 3 components, getting "
        << orientArray->GetNumberOfComponents() << " components.");
      return;
    }
    else if (this->OrientationMode == QUATERNION && orientArray->GetNumberOfComponents() != 4)
    {
      svtkErrorMacro(" expecting an orientation array with 4 components, getting "
        << orientArray->GetNumberOfComponents() << " components.");
      return;
    }
  }

  svtkDataArray* indexArray = this->GetSourceIndexArray(dataset);
  svtkDataArray* scaleArray = this->GetScaleArray(dataset);
  svtkDataArray* selectionArray = this->GetSelectionIdArray(dataset);

  /// FIXME: Didn't handle the premultiplycolorswithalpha aspect...
  this->ColorMapper->SetInputDataObject(dataset);
  this->ColorMapper->MapScalars(actor->GetProperty()->GetOpacity());
  svtkUnsignedCharArray* colors =
    ((svtkOpenGLGlyph3DMappersvtkColorMapper*)this->ColorMapper)->GetColors();
  // Traverse all Input points, transforming Source points

  int numEntries = (int)subarray->Entries.size();

  // how many points for each source
  int* numPointsPerSource = new int[numEntries];
  std::fill(numPointsPerSource, numPointsPerSource + numEntries, 0);
  if (numEntries > 1 && indexArray)
  {
    // loop over every point
    int index = 0;
    for (svtkIdType inPtId = 0; inPtId < numPts; inPtId++)
    {
      if (maskArray && maskArray->GetValue(inPtId) == 0)
      {
        continue;
      }

      // Compute index into table of glyphs
      double value =
        svtkMath::Norm(indexArray->GetTuple(inPtId), indexArray->GetNumberOfComponents());
      index = static_cast<int>(value);
      index = svtkMath::ClampValue(index, 0, numEntries - 1);
      numPointsPerSource[index]++;
    }
  }
  else
  {
    numPointsPerSource[0] = numPts;
  }

  // for each entry start with a reasonable allocation
  for (size_t cc = 0; cc < subarray->Entries.size(); cc++)
  {
    svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry* entry = subarray->Entries[cc];
    entry->PickIds.resize(numPointsPerSource[cc]);
    entry->Colors.resize(numPointsPerSource[cc] * 4);
    entry->Matrices.resize(numPointsPerSource[cc] * 16);
    entry->NormalMatrices.resize(numPointsPerSource[cc] * 9);
    entry->NumberOfPoints = 0;
    entry->BuildTime.Modified();
  }
  delete[] numPointsPerSource;

  // loop over every point and fill structures
  int index = 0;
  svtkDataObjectTree* sourceTableTree = this->GetSourceTableTree();

  // cache sources to improve performances
  std::vector<svtkDataObject*> sourceCache(numEntries);
  for (svtkIdType i = 0; i < numEntries; i++)
  {
    sourceCache[i] =
      this->UseSourceTableTree ? getChildDataObject(sourceTableTree, i) : this->GetSource(i);
  }

  double trans[16];
  double normalTrans[9];

  for (svtkIdType inPtId = 0; inPtId < numPts; inPtId++)
  {
    if (!(inPtId % 10000))
    {
      this->UpdateProgress(static_cast<double>(inPtId) / static_cast<double>(numPts));
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    if (maskArray && maskArray->GetValue(inPtId) == 0)
    {
      continue;
    }

    // Compute index into table of glyphs
    if (indexArray)
    {
      double value =
        svtkMath::Norm(indexArray->GetTuple(inPtId), indexArray->GetNumberOfComponents());
      index = static_cast<int>(value);
      index = svtkMath::ClampValue(index, 0, numEntries - 1);
    }

    // source can be null.
    svtkDataObject* source = sourceCache[index];

    // Make sure we're not indexing into empty glyph
    if (source)
    {
      svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry* entry = subarray->Entries[index];

      entry->Colors[entry->NumberOfPoints * 4] = color[0];
      entry->Colors[entry->NumberOfPoints * 4 + 1] = color[1];
      entry->Colors[entry->NumberOfPoints * 4 + 2] = color[2];
      entry->Colors[entry->NumberOfPoints * 4 + 3] = color[3];

      double scalex = 1.0;
      double scaley = 1.0;
      double scalez = 1.0;
      // Get the scalar and vector data
      if (scaleArray)
      {
        double* tuple = scaleArray->GetTuple(inPtId);
        switch (this->ScaleMode)
        {
          case SCALE_BY_MAGNITUDE:
            scalex = scaley = scalez = svtkMath::Norm(tuple, scaleArray->GetNumberOfComponents());
            break;
          case SCALE_BY_COMPONENTS:
            if (scaleArray->GetNumberOfComponents() != 3)
            {
              svtkErrorMacro("Cannot scale by components since " << scaleArray->GetName()
                                                                << " does not have 3 components.");
            }
            else
            {
              scalex = tuple[0];
              scaley = tuple[1];
              scalez = tuple[2];
            }
            break;
          case NO_DATA_SCALING:
          default:
            break;
        }

        // Clamp data scale if enabled
        if (this->Clamping && this->ScaleMode != NO_DATA_SCALING)
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
      }
      scalex *= this->ScaleFactor;
      scaley *= this->ScaleFactor;
      scalez *= this->ScaleFactor;

      // Now begin copying/transforming glyph
      svtkMatrix4x4::Identity(trans);
      svtkMatrix3x3::Identity(normalTrans);

      // translate Source to Input point
      double x[3];
      dataset->GetPoint(inPtId, x);
      trans[3] = x[0];
      trans[7] = x[1];
      trans[11] = x[2];

      if (orientArray)
      {
        double orientation[4];
        orientArray->GetTuple(inPtId, orientation);

        double rotMatrix[3][3];
        svtkQuaterniond quaternion;

        switch (this->OrientationMode)
        {
          case ROTATION:
          {
            double angle = svtkMath::RadiansFromDegrees(orientation[2]);
            svtkQuaterniond qz(cos(0.5 * angle), 0.0, 0.0, sin(0.5 * angle));

            angle = svtkMath::RadiansFromDegrees(orientation[0]);
            svtkQuaterniond qx(cos(0.5 * angle), sin(0.5 * angle), 0.0, 0.0);

            angle = svtkMath::RadiansFromDegrees(orientation[1]);
            svtkQuaterniond qy(cos(0.5 * angle), 0.0, sin(0.5 * angle), 0.0);

            quaternion = qz * qx * qy;
          }
          break;

          case DIRECTION:
            if (orientation[1] == 0.0 && orientation[2] == 0.0)
            {
              if (orientation[0] < 0) // just flip x if we need to
              {
                quaternion.Set(0.0, 0.0, 1.0, 0.0);
              }
            }
            else
            {
              double vMag = svtkMath::Norm(orientation);
              double vNew[3];
              vNew[0] = (orientation[0] + vMag) / 2.0;
              vNew[1] = orientation[1] / 2.0;
              vNew[2] = orientation[2] / 2.0;

              double f = 1.0 / sqrt(vNew[0] * vNew[0] + vNew[1] * vNew[1] + vNew[2] * vNew[2]);
              vNew[0] *= f;
              vNew[1] *= f;
              vNew[2] *= f;

              quaternion.Set(0.0, vNew[0], vNew[1], vNew[2]);
            }
            break;

          case QUATERNION:
            quaternion.Set(orientation);
            break;
        }

        quaternion.ToMatrix3x3(rotMatrix);

        for (int i = 0; i < 3; i++)
        {
          for (int j = 0; j < 3; j++)
          {
            trans[4 * i + j] = rotMatrix[i][j];
            normalTrans[3 * i + j] = rotMatrix[j][i]; // transpose
          }
        }
      }

      // Set pickid
      // Use selectionArray value or glyph point ID.
      svtkIdType selectionId = inPtId;
      if (this->UseSelectionIds)
      {
        if (selectionArray == nullptr || selectionArray->GetNumberOfTuples() == 0)
        {
          svtkWarningMacro(<< "UseSelectionIds is true, but selection array"
                             " is invalid. Ignoring selection array.");
        }
        else
        {
          selectionId = static_cast<svtkIdType>(*selectionArray->GetTuple(inPtId));
        }
      }
      entry->PickIds[entry->NumberOfPoints] = selectionId;

      if (colors)
      {
        colors->GetTypedTuple(inPtId, &(entry->Colors[entry->NumberOfPoints * 4]));
      }

      // scale data if appropriate
      if (this->Scaling)
      {
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

        for (int i = 0; i < 3; i++)
        {
          // inverse of normal matrix is directly computed with inverse scale
          trans[4 * i] *= scalex;
          normalTrans[i] /= scalex;
          trans[4 * i + 1] *= scaley;
          normalTrans[i + 3] /= scaley;
          trans[4 * i + 2] *= scalez;
          normalTrans[i + 6] /= scalez;
        }
      }

      float* matrices = &entry->Matrices[entry->NumberOfPoints * 16];
      float* normalMatrices = &entry->NormalMatrices[entry->NumberOfPoints * 9];

      for (int i = 0; i < 4; i++)
      {
        for (int j = 0; j < 4; j++)
        {
          matrices[i * 4 + j] = trans[j * 4 + i];
        }
      }

      for (int i = 0; i < 3; i++)
      {
        for (int j = 0; j < 3; j++)
        {
          normalMatrices[i * 3 + j] = normalTrans[i * 3 + j];
        }
      }
      entry->NumberOfPoints++;
    }
  }

  subarray->BuildTime.Modified();
}

// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void svtkOpenGLGlyph3DMapper::ReleaseGraphicsResources(svtkWindow* window)
{
  if (this->GlyphValues)
  {
    std::map<const svtkDataSet*, svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperSubArray*>::iterator
      miter = this->GlyphValues->Entries.begin();
    for (; miter != this->GlyphValues->Entries.end(); ++miter)
    {
      std::vector<svtkOpenGLGlyph3DMapper::svtkOpenGLGlyph3DMapperEntry*>::iterator miter2 =
        miter->second->Entries.begin();
      for (; miter2 != miter->second->Entries.end(); ++miter2)
      {
        svtkOpenGLGlyph3DMapperEntry::MapperMap::iterator miter3 = (*miter2)->Mappers.begin();
        for (; miter3 != (*miter2)->Mappers.end(); ++miter3)
        {
          miter3->second->ReleaseGraphicsResources(window);
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
svtkIdType svtkOpenGLGlyph3DMapper::GetMaxNumberOfLOD()
{
#ifndef GL_ES_VERSION_3_0
  if (!GLEW_ARB_gpu_shader5 || !GLEW_ARB_transform_feedback3)
  {
    return 0;
  }

  GLint streams, maxsize;
  glGetIntegerv(GL_MAX_VERTEX_STREAMS, &streams);
  glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &maxsize);
  maxsize /=
    32; // each stream size can be 29 bytes (16 for transform matrix, 9 for normal, 4 for color)

  svtkIdType maxstreams = static_cast<svtkIdType>(std::min(streams, maxsize));
  return maxstreams - 1;
#else
  return 0;
#endif
}

//---------------------------------------------------------------------------
void svtkOpenGLGlyph3DMapper::SetNumberOfLOD(svtkIdType nb)
{
  this->LODs.resize(nb, { 0.f, 0.f });
}

//---------------------------------------------------------------------------
void svtkOpenGLGlyph3DMapper::SetLODDistanceAndTargetReduction(
  svtkIdType index, float distance, float targetReduction)
{
  if (index < static_cast<svtkIdType>(this->LODs.size()))
  {
    this->LODs[index] = { svtkMath::Max(0.f, distance),
      svtkMath::ClampValue(targetReduction, 0.f, 1.f) };
  }
}

// ----------------------------------------------------------------------------
void svtkOpenGLGlyph3DMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
