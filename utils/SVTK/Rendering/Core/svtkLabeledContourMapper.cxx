/*==============================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabeledContourMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#include "svtkLabeledContourMapper.h"

#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCoordinate.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTextActor3D.h"
#include "svtkTextProperty.h"
#include "svtkTextPropertyCollection.h"
#include "svtkTextRenderer.h"
#include "svtkTimerLog.h"
#include "svtkTransform.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"
#include "svtkWindow.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <vector>

//------------------------------------------------------------------------------
struct LabelMetric
{
  bool Valid;
  double Value;
  svtkTextProperty* TProp;
  std::string Text;
  // These measure the pixel size of the text texture:
  svtkTuple<int, 4> BoundingBox;
  svtkTuple<int, 2> Dimensions;
};

//------------------------------------------------------------------------------
struct LabelInfo
{
  // Position in actor space:
  svtkVector3d Position;

  // Orientation (normalized, world space)
  svtkVector3d RightW; // Left --> Right
  svtkVector3d UpW;    // Bottom --> Top

  // Orientation (normalized in world space, represented in actor space)
  svtkVector3d RightA; // Left --> Right
  svtkVector3d UpA;    // Bottom --> Top

  // Corner locations (actor space):
  svtkVector3d TLa;
  svtkVector3d TRa;
  svtkVector3d BRa;
  svtkVector3d BLa;

  // Corner location (display space):
  svtkVector2i TLd;
  svtkVector2i TRd;
  svtkVector2i BRd;
  svtkVector2i BLd;

  // Factor to scale the text actor by:
  double ScaleDisplayToActor;
};

namespace
{

// Circular iterator through a svtkTextPropertyCollection -----------------------
class TextPropLoop
{
  svtkTextPropertyCollection* TProps;

public:
  TextPropLoop(svtkTextPropertyCollection* col)
    : TProps(col)
  {
    TProps->InitTraversal();
  }

  svtkTextProperty* Next()
  {
    // The input checks should fail if this is the case:
    assert("No text properties set! Prerender check failed!" && TProps->GetNumberOfItems() != 0);

    svtkTextProperty* result = TProps->GetNextItem();
    if (!result)
    {
      TProps->InitTraversal();
      result = TProps->GetNextItem();
      assert("Text property traversal error." && result != nullptr);
    }
    return result;
  }
};

//------------------------------------------------------------------------------
double calculateSmoothness(double pathLength, double distance)
{
  return (pathLength - distance) / distance;
}

} // end anon namespace

//------------------------------------------------------------------------------
struct svtkLabeledContourMapper::Private
{
  // One entry per isoline.
  std::vector<LabelMetric> LabelMetrics;

  // One LabelInfo per label groups by isoline.
  std::vector<std::vector<LabelInfo> > LabelInfos;

  // Info for calculating display coordinates:
  svtkTuple<double, 16> AMVP;               // actor-model-view-projection matrix
  svtkTuple<double, 16> ActorMatrix;        // Actor model matrix
  svtkTuple<double, 16> InverseActorMatrix; // Inverse Actor model matrix
  svtkTuple<double, 4> ViewPort;            // viewport
  svtkTuple<double, 4> NormalizedViewPort;  // see svtkViewport::ViewToNormalizedVP
  svtkTuple<int, 2> WindowSize;
  svtkTuple<int, 2> ViewPortSize;
  svtkTuple<double, 2> DisplayOffset;
  svtkTuple<double, 4> ViewportBounds;

  // Needed to orient the labels
  svtkVector3d CameraRight;
  svtkVector3d CameraUp;
  svtkVector3d CameraForward;

  // Render times:
  double PrepareTime;
  double RenderTime;

  // Only want to print the stencil warning once:
  bool AlreadyWarnedAboutStencils;

  // Project coordinates. Note that the vector objects must be unique.
  void ActorToWorld(const svtkVector3d& actor, svtkVector3d& world) const;
  void WorldToActor(const svtkVector3d& world, svtkVector3d& actor) const;
  void ActorToDisplay(const svtkVector3d& actor, svtkVector2i& display) const;
  void ActorToDisplay(const svtkVector3d& actor, svtkVector2d& display) const;

  // Camera axes:
  bool SetViewInfo(svtkRenderer* ren, svtkActor* act);

  // Visibility test (display space):
  template <typename ScalarType>
  bool PixelIsVisible(const svtkVector2<ScalarType>& dispCoord) const;

  bool LineCanBeLabeled(
    svtkPoints* points, svtkIdType numIds, const svtkIdType* ids, const LabelMetric& metrics);

  // Determine the first smooth position on the line defined by ids that is
  // 1.2x the length of the label (in display coordinates).
  // The position will be no less than skipDistance along the line from the
  // starting location. This can be used to ensure that labels are placed a
  // minimum distance apart.
  bool NextLabel(svtkPoints* points, svtkIdType& numIds, const svtkIdType*& ids,
    const LabelMetric& metrics, LabelInfo& info, double targetSmoothness, double skipDistance);

  // Configure the text actor:
  bool BuildLabel(svtkTextActor3D* actor, const LabelMetric& metric, const LabelInfo& info);

  // Compute the scaling factor and corner info for the label
  void ComputeLabelInfo(LabelInfo& info, const LabelMetric& metrics);

  // Test if the display quads overlap:
  bool TestOverlap(const LabelInfo& a, const LabelInfo& b);
};

//------------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkLabeledContourMapper);

//------------------------------------------------------------------------------
svtkLabeledContourMapper::svtkLabeledContourMapper()
{
  this->SkipDistance = 0.;
  this->LabelVisibility = true;
  this->TextActors = nullptr;
  this->NumberOfTextActors = 0;
  this->NumberOfUsedTextActors = 0;

  this->StencilQuads = nullptr;
  this->StencilQuadsSize = 0;
  this->StencilQuadIndices = nullptr;
  this->StencilQuadIndicesSize = 0;

  this->TextProperties = svtkSmartPointer<svtkTextPropertyCollection>::New();
  svtkNew<svtkTextProperty> defaultTProp;
  this->TextProperties->AddItem(defaultTProp);

  this->Internal = new svtkLabeledContourMapper::Private();
  this->Internal->PrepareTime = 0.0;
  this->Internal->RenderTime = 0.0;
  this->Internal->AlreadyWarnedAboutStencils = false;

  this->Reset();
}

//------------------------------------------------------------------------------
svtkLabeledContourMapper::~svtkLabeledContourMapper()
{
  this->FreeStencilQuads();
  this->FreeTextActors();

  delete this->Internal;
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::Render(svtkRenderer* ren, svtkActor* act)
{
  if (svtkRenderWindow* renderWindow = ren->GetRenderWindow())
  {
    // Is the viewport's RenderWindow capturing GL2PS-special props?
    if (renderWindow->GetCapturingGL2PSSpecialProps())
    {
      ren->CaptureGL2PSSpecialProp(act);
    }
  }

  // Make sure input data is synced
  if (svtkAlgorithm* inputAlgorithm = this->GetInputAlgorithm())
  {
    inputAlgorithm->Update();
  }

  if (!this->CheckInputs(ren))
  {
    return;
  }

  if (!this->LabelVisibility)
  {
    this->RenderPolyData(ren, act);
    return;
  }

  if (this->CheckRebuild(ren, act))
  {
    double startPrep = svtkTimerLog::GetUniversalTime();

    this->Reset();

    if (!this->PrepareRender(ren, act))
    {
      return;
    }

    if (!this->PlaceLabels())
    {
      return;
    }

    if (!this->ResolveLabels())
    {
      return;
    }

    if (!this->CreateLabels(act))
    {
      return;
    }

    if (!this->BuildStencilQuads())
    {
      return;
    }

    this->Internal->PrepareTime = svtkTimerLog::GetUniversalTime() - startPrep;
    this->LabelBuildTime.Modified();
  }

  double startRender = svtkTimerLog::GetUniversalTime();

  if (!this->ApplyStencil(ren, act))
  {
    return;
  }

  if (!this->RenderPolyData(ren, act))
  {
    this->RemoveStencil(ren);
    return;
  }

  if (!this->RemoveStencil(ren))
  {
    return;
  }

  if (!this->RenderLabels(ren, act))
  {
    return;
  }

  this->Internal->RenderTime = svtkTimerLog::GetUniversalTime() - startRender;
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::SetInputData(svtkPolyData* input)
{
  this->SetInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
svtkPolyData* svtkLabeledContourMapper::GetInput()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//------------------------------------------------------------------------------
double* svtkLabeledContourMapper::GetBounds()
{
  if (this->GetNumberOfInputConnections(0) == 0)
  {
    svtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    this->ComputeBounds();
    return this->Bounds;
  }
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::GetBounds(double bounds[])
{
  this->Superclass::GetBounds(bounds);
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::SetTextProperty(svtkTextProperty* tprop)
{
  if (this->TextProperties->GetNumberOfItems() != 1 ||
    this->TextProperties->GetItemAsObject(0) != tprop)
  {
    this->TextProperties->RemoveAllItems();
    this->TextProperties->AddItem(tprop);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::SetTextProperties(svtkTextPropertyCollection* coll)
{
  if (coll != this->TextProperties)
  {
    this->TextProperties = coll;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
svtkTextPropertyCollection* svtkLabeledContourMapper::GetTextProperties()
{
  return this->TextProperties;
}

//------------------------------------------------------------------------------
svtkDoubleArray* svtkLabeledContourMapper::GetTextPropertyMapping()
{
  return this->TextPropertyMapping;
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::SetTextPropertyMapping(svtkDoubleArray* mapping)
{
  if (this->TextPropertyMapping != mapping)
  {
    this->TextPropertyMapping = mapping;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::ReleaseGraphicsResources(svtkWindow* win)
{
  this->PolyDataMapper->ReleaseGraphicsResources(win);
  for (svtkIdType i = 0; i < this->NumberOfTextActors; ++i)
  {
    this->TextActors[i]->ReleaseGraphicsResources(win);
  }
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::ComputeBounds()
{
  this->GetInput()->GetBounds(this->Bounds);
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SkipDistance: " << this->SkipDistance << "\n"
     << indent << "LabelVisibility: " << (this->LabelVisibility ? "On\n" : "Off\n") << indent
     << "NumberOfTextActors: " << this->NumberOfTextActors << "\n"
     << indent << "NumberOfUsedTextActors: " << this->NumberOfUsedTextActors << "\n"
     << indent << "StencilQuadsSize: " << this->StencilQuadsSize << "\n"
     << indent << "StencilQuadIndicesSize: " << this->StencilQuadIndicesSize << "\n"
     << indent << "BuildTime: " << this->LabelBuildTime.GetMTime() << "\n"
     << indent << "PolyDataMapper:\n";
  this->PolyDataMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TextProperties:\n";
  this->TextProperties->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TextPropertyMapping:";
  if (this->TextPropertyMapping)
  {
    os << "\n";
    this->TextPropertyMapping->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << " (nullptr)\n";
  }
}

//------------------------------------------------------------------------------
int svtkLabeledContourMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::Reset()
{
  this->Internal->LabelMetrics.clear();
  this->Internal->LabelInfos.clear();

  this->TextProperties->InitTraversal();
  while (svtkTextProperty* tprop = this->TextProperties->GetNextItem())
  {
    tprop->SetJustificationToCentered();
    tprop->SetVerticalJustificationToCentered();
  }
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::CheckInputs(svtkRenderer* ren)
{
  svtkPolyData* input = this->GetInput();
  if (!input)
  {
    svtkErrorMacro(<< "No input data!");
    return false;
  }

  if (!input->GetPoints())
  {
    svtkErrorMacro(<< "No points in dataset!");
    return false;
  }

  if (!input->GetPointData())
  {
    svtkErrorMacro(<< "No point data in dataset!");
    return false;
  }

  svtkCellArray* lines = input->GetLines();
  if (!lines)
  {
    svtkErrorMacro(<< "No lines in dataset!");
    return false;
  }

  svtkDataArray* scalars = input->GetPointData()->GetScalars();
  if (!scalars)
  {
    svtkErrorMacro(<< "No scalars in dataset!");
    return false;
  }

  svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
  if (!tren)
  {
    svtkErrorMacro(<< "Text renderer unavailable.");
    return false;
  }

  if (this->TextProperties->GetNumberOfItems() == 0)
  {
    svtkErrorMacro(<< "No text properties set!");
    return false;
  }

  // Print a warning if stenciling is not enabled:
  svtkRenderWindow* win = ren->GetRenderWindow();
  if (!this->Internal->AlreadyWarnedAboutStencils && win)
  {
    if (win->GetStencilCapable() == 0)
    {
      svtkWarningMacro(<< "Stenciling is not enabled in the render window. "
                         "Isoline labels will have artifacts. To fix this, "
                         "call svtkRenderWindow::StencilCapableOn().");
      this->Internal->AlreadyWarnedAboutStencils = true;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::CheckRebuild(svtkRenderer*, svtkActor* act)
{
  // Get the highest mtime for the text properties:
  svtkMTimeType tPropMTime = this->TextProperties->GetMTime();
  this->TextProperties->InitTraversal();
  while (svtkTextProperty* tprop = this->TextProperties->GetNextItem())
  {
    tPropMTime = std::max(tPropMTime, tprop->GetMTime());
  }

  // Are we out of date?
  if (this->LabelBuildTime.GetMTime() < this->GetInput()->GetMTime() ||
    this->LabelBuildTime.GetMTime() < tPropMTime)
  {
    return true;
  }

  // Is there enough time allocated? (i.e. is this not an interactive render?)
  if (act->GetAllocatedRenderTime() >= (this->Internal->RenderTime + this->Internal->PrepareTime))
  {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::PrepareRender(svtkRenderer* ren, svtkActor* act)
{
  if (!this->Internal->SetViewInfo(ren, act))
  {
    return false;
  }

  // Already checked that these exist in CheckInputs()
  svtkPolyData* input = this->GetInput();
  svtkCellArray* lines = input->GetLines();
  svtkDataArray* scalars = input->GetPointData()->GetScalars();
  svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
  if (!tren)
  {
    svtkErrorMacro(<< "Text renderer unavailable.");
    return false;
  }

  // Maps scalar values to text properties:
  typedef std::map<double, svtkTextProperty*> LabelPropertyMapType;
  LabelPropertyMapType labelMap;

  // Initialize with the user-requested mapping, if it exists.
  if (this->TextPropertyMapping != nullptr)
  {
    svtkDoubleArray::Iterator valIt = this->TextPropertyMapping->Begin();
    svtkDoubleArray::Iterator valItEnd = this->TextPropertyMapping->End();
    TextPropLoop tprops(this->TextProperties);
    for (; valIt != valItEnd; ++valIt)
    {
      labelMap.insert(std::make_pair(*valIt, tprops.Next()));
    }
  }

  // Create the list of metrics, but no text property information yet.
  svtkIdType numPts;
  const svtkIdType* ids;
  for (lines->InitTraversal(); lines->GetNextCell(numPts, ids);)
  {
    this->Internal->LabelMetrics.push_back(LabelMetric());
    LabelMetric& metric = this->Internal->LabelMetrics.back();
    if (!(metric.Valid = (numPts > 0)))
    {
      // Mark as invalid and skip if there are no points.
      continue;
    }
    metric.Value = scalars->GetComponent(ids[0], 0);
    metric.Value = std::fabs(metric.Value) > 1e-6 ? metric.Value : 0.0;
    std::ostringstream str;
    str << metric.Value;
    metric.Text = str.str();

    // Beware future maintainers: The following line of code has been carefully
    // crafted to reach a zen-like harmony of compatibility between various
    // compilers that have differing syntactic requirements for creating a
    // pair containing a nullptr:
    // - Pedantically strict C++11 compilers (e.g. MSVC 2012) will not compile:
    //     std::make_pair<double, X*>(someDouble, nullptr);
    //   or any make_pair call with explicit template args and value arguments,
    //   as the signature expects an rvalue.

    // The value will be replaced in the next loop:
    labelMap.insert(
      std::pair<double, svtkTextProperty*>(metric.Value, static_cast<svtkTextProperty*>(nullptr)));
  }

  // Now that all present scalar values are known, assign text properties:
  TextPropLoop tprops(this->TextProperties);
  typedef LabelPropertyMapType::iterator LabelPropertyMapIter;
  for (LabelPropertyMapIter it = labelMap.begin(), itEnd = labelMap.end(); it != itEnd; ++it)
  {
    if (!it->second) // Skip if initialized from TextPropertyMapping
    {
      it->second = tprops.Next();
    }
  }

  // Update metrics with appropriate text info:
  typedef std::vector<LabelMetric>::iterator MetricsIter;
  for (MetricsIter it = this->Internal->LabelMetrics.begin(),
                   itEnd = this->Internal->LabelMetrics.end();
       it != itEnd; ++it)
  {
    if (!it->Valid)
    {
      continue;
    }

    // Look up text property for the scalar value:
    LabelPropertyMapIter tpropIt = labelMap.find(it->Value);
    assert("No text property assigned for scalar value." && tpropIt != labelMap.end());
    it->TProp = tpropIt->second;

    // Assign bounding box/dims.
    if (!tren->GetBoundingBox(
          it->TProp, it->Text, it->BoundingBox.GetData(), svtkTextActor3D::GetRenderedDPI()))
    {
      svtkErrorMacro(<< "Error calculating bounding box for string '" << it->Text << "'.");
      return false;
    }
    it->Dimensions[0] = it->BoundingBox[1] - it->BoundingBox[0] + 1;
    it->Dimensions[1] = it->BoundingBox[3] - it->BoundingBox[2] + 1;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::PlaceLabels()
{
  svtkPolyData* input = this->GetInput();
  svtkPoints* points = input->GetPoints();
  svtkCellArray* lines = input->GetLines();

  // Progression of smoothness tolerances to use.
  std::vector<double> tols;
  tols.push_back(0.010);
  tols.push_back(0.025);
  tols.push_back(0.050);
  tols.push_back(0.100);
  tols.push_back(0.200);
  tols.push_back(0.300);

  typedef std::vector<LabelMetric>::const_iterator MetricsIterator;
  MetricsIterator metric = this->Internal->LabelMetrics.begin();

  // Identify smooth parts of the isoline for labeling
  svtkIdType numIds;
  const svtkIdType* origIds;
  this->Internal->LabelInfos.reserve(this->Internal->LabelMetrics.size());
  for (lines->InitTraversal(); lines->GetNextCell(numIds, origIds); ++metric)
  {
    assert(metric != this->Internal->LabelMetrics.end());

    this->Internal->LabelInfos.push_back(std::vector<LabelInfo>());

    // Test if it is possible to place a label (e.g. the line is big enough
    // to not be completely obscured)
    if (this->Internal->LineCanBeLabeled(points, numIds, origIds, *metric))
    {
      std::vector<LabelInfo>& infos = this->Internal->LabelInfos.back();
      LabelInfo info;
      // If no labels are found, increase the tolerance:
      for (std::vector<double>::const_iterator it = tols.begin(), itEnd = tols.end();
           it != itEnd && infos.empty(); ++it)
      {
        svtkIdType nIds = numIds;
        const svtkIdType* ids = origIds;
        while (this->Internal->NextLabel(points, nIds, ids, *metric, info, *it, this->SkipDistance))
        {
          infos.push_back(info);
        }
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::ResolveLabels()
{
  typedef std::vector<LabelInfo>::iterator InnerIterator;
  typedef std::vector<std::vector<LabelInfo> >::iterator OuterIterator;

  bool removedA = false;
  bool removedB = false;

  OuterIterator outerA = this->Internal->LabelInfos.begin();
  OuterIterator outerEnd = this->Internal->LabelInfos.end();
  while (outerA != outerEnd)
  {
    InnerIterator innerA = outerA->begin();
    InnerIterator innerAEnd = outerA->end();
    while (innerA != innerAEnd)
    {
      removedA = false;
      OuterIterator outerB = outerA;
      while (!removedA && outerB != outerEnd)
      {
        InnerIterator innerB = outerA == outerB ? innerA + 1 : outerB->begin();
        InnerIterator innerBEnd = outerB->end();
        while (!removedA && innerB != innerBEnd)
        {
          removedB = false;
          // Does innerA overlap with innerB?
          if (this->Internal->TestOverlap(*innerA, *innerB))
          {
            // Remove the label that has the most labels for its isoline:
            if (outerA->size() > outerB->size())
            {
              // Remove innerA
              innerA = outerA->erase(innerA);
              innerAEnd = outerA->end();
              removedA = true;
            }
            else
            {
              // Remove innerB
              // Need to update A's iterators if outerA == outerB
              if (outerA == outerB)
              {
                // We know that aIdx < bIdx, so removing B won't change
                // the position of A:
                size_t aIdx = innerA - outerA->begin();
                innerB = outerB->erase(innerB);
                innerBEnd = outerB->end();
                innerA = outerA->begin() + aIdx;
                innerAEnd = outerA->end();
              }
              else
              {
                innerB = outerB->erase(innerB);
                innerBEnd = outerB->end();
              }
              removedB = true;
            }
          }
          // Erase will increment B if we removed it.
          if (!removedB)
          {
            ++innerB;
          }
        }
        ++outerB;
      }
      // Erase will increment A if we removed it.
      if (!removedA)
      {
        ++innerA;
      }
    }
    ++outerA;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::CreateLabels(svtkActor*)
{
  typedef std::vector<LabelMetric> MetricVector;
  typedef std::vector<LabelInfo> InfoVector;

  std::vector<InfoVector>::const_iterator outerLabels = this->Internal->LabelInfos.begin();
  std::vector<InfoVector>::const_iterator outerLabelsEnd = this->Internal->LabelInfos.end();

  // count the number of labels:
  svtkIdType numLabels = 0;
  while (outerLabels != outerLabelsEnd)
  {
    numLabels += static_cast<svtkIdType>((outerLabels++)->size());
  }

  if (!this->AllocateTextActors(numLabels))
  {
    svtkErrorMacro(<< "Error while allocating text actors.");
    return false;
  }

  outerLabels = this->Internal->LabelInfos.begin();
  MetricVector::const_iterator metrics = this->Internal->LabelMetrics.begin();
  MetricVector::const_iterator metricsEnd = this->Internal->LabelMetrics.end();
  svtkTextActor3D** actor = this->TextActors;
  svtkTextActor3D** actorEnd = this->TextActors + this->NumberOfUsedTextActors;

  while (metrics != metricsEnd && outerLabels != outerLabelsEnd && actor != actorEnd)
  {
    for (InfoVector::const_iterator label = outerLabels->begin(), labelEnd = outerLabels->end();
         label != labelEnd; ++label)
    {
      this->Internal->BuildLabel(*actor, *metrics, *label);
      ++actor;
    }
    ++metrics;
    ++outerLabels;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::ApplyStencil(svtkRenderer*, svtkActor*)
{
  // Handled in backend override.
  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::RenderPolyData(svtkRenderer* ren, svtkActor* act)
{
  this->PolyDataMapper->SetInputConnection(this->GetInputConnection(0, 0));
  this->PolyDataMapper->Render(ren, act);
  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::RemoveStencil(svtkRenderer*)
{
  // Handled in backend override.
  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::RenderLabels(svtkRenderer* ren, svtkActor*)
{
  for (svtkIdType i = 0; i < this->NumberOfUsedTextActors; ++i)
  {
    // Needed for GL2PS capture:
    this->TextActors[i]->RenderOpaqueGeometry(ren);
    // Actually draw:
    this->TextActors[i]->RenderTranslucentPolygonalGeometry(ren);
  }
  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::AllocateTextActors(svtkIdType num)
{
  if (num != this->NumberOfUsedTextActors)
  {
    if (this->NumberOfTextActors < num || this->NumberOfTextActors > 2 * num)
    {
      this->FreeTextActors();

      // Leave some room to grow:
      this->NumberOfTextActors = num * 1.2;

      this->TextActors = new svtkTextActor3D*[this->NumberOfTextActors];
      for (svtkIdType i = 0; i < this->NumberOfTextActors; ++i)
      {
        this->TextActors[i] = svtkTextActor3D::New();
      }
    }

    this->NumberOfUsedTextActors = num;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::FreeTextActors()
{
  for (svtkIdType i = 0; i < this->NumberOfTextActors; ++i)
  {
    this->TextActors[i]->Delete();
  }

  delete[] this->TextActors;
  this->TextActors = nullptr;
  this->NumberOfTextActors = 0;
  this->NumberOfUsedTextActors = 0;
  return true;
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::FreeStencilQuads()
{
  if (this->StencilQuads)
  {
    delete[] this->StencilQuads;
    this->StencilQuads = nullptr;
    this->StencilQuadsSize = 0;

    delete[] this->StencilQuadIndices;
    this->StencilQuadIndices = nullptr;
    this->StencilQuadIndicesSize = 0;
  }
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::BuildStencilQuads()
{
  svtkIdType quadCount = this->NumberOfUsedTextActors * 12;
  svtkIdType idxCount = this->NumberOfUsedTextActors * 6;
  if (quadCount != this->StencilQuadsSize)
  {
    this->FreeStencilQuads();
    this->StencilQuads = new float[quadCount];
    this->StencilQuadsSize = quadCount;
    this->StencilQuadIndices = new unsigned int[idxCount];
    this->StencilQuadIndicesSize = idxCount;
  }

  unsigned int qIndex = 0; // quad array index
  unsigned int iIndex = 0; // index array index
  unsigned int eIndex = 0; // index array element

  typedef std::vector<LabelInfo>::const_iterator InnerIterator;
  typedef std::vector<std::vector<LabelInfo> >::const_iterator OuterIterator;

  for (OuterIterator out = this->Internal->LabelInfos.begin(),
                     outEnd = this->Internal->LabelInfos.end();
       out != outEnd; ++out)
  {
    for (InnerIterator in = out->begin(), inEnd = out->end(); in != inEnd; ++in)
    {
      this->StencilQuads[qIndex + 0] = static_cast<float>(in->TLa[0]);
      this->StencilQuads[qIndex + 1] = static_cast<float>(in->TLa[1]);
      this->StencilQuads[qIndex + 2] = static_cast<float>(in->TLa[2]);
      this->StencilQuads[qIndex + 3] = static_cast<float>(in->TRa[0]);
      this->StencilQuads[qIndex + 4] = static_cast<float>(in->TRa[1]);
      this->StencilQuads[qIndex + 5] = static_cast<float>(in->TRa[2]);
      this->StencilQuads[qIndex + 6] = static_cast<float>(in->BRa[0]);
      this->StencilQuads[qIndex + 7] = static_cast<float>(in->BRa[1]);
      this->StencilQuads[qIndex + 8] = static_cast<float>(in->BRa[2]);
      this->StencilQuads[qIndex + 9] = static_cast<float>(in->BLa[0]);
      this->StencilQuads[qIndex + 10] = static_cast<float>(in->BLa[1]);
      this->StencilQuads[qIndex + 11] = static_cast<float>(in->BLa[2]);

      this->StencilQuadIndices[iIndex + 0] = eIndex + 0;
      this->StencilQuadIndices[iIndex + 1] = eIndex + 1;
      this->StencilQuadIndices[iIndex + 2] = eIndex + 2;
      this->StencilQuadIndices[iIndex + 3] = eIndex + 0;
      this->StencilQuadIndices[iIndex + 4] = eIndex + 2;
      this->StencilQuadIndices[iIndex + 5] = eIndex + 3;

      qIndex += 12;
      iIndex += 6;
      eIndex += 4;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::Private::ActorToWorld(const svtkVector3d& in, svtkVector3d& out) const
{
  const svtkTuple<double, 16>& x = this->ActorMatrix;
  double w;
  out[0] = in[0] * x[0] + in[1] * x[1] + in[2] * x[2] + x[3];
  out[1] = in[0] * x[4] + in[1] * x[5] + in[2] * x[6] + x[7];
  out[2] = in[0] * x[8] + in[1] * x[9] + in[2] * x[10] + x[11];
  w = in[0] * x[12] + in[1] * x[13] + in[2] * x[14] + x[15];
  out = out * (1. / w);
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::Private::WorldToActor(const svtkVector3d& in, svtkVector3d& out) const
{
  const svtkTuple<double, 16>& x = this->InverseActorMatrix;
  double w;
  out[0] = in[0] * x[0] + in[1] * x[1] + in[2] * x[2] + x[3];
  out[1] = in[0] * x[4] + in[1] * x[5] + in[2] * x[6] + x[7];
  out[2] = in[0] * x[8] + in[1] * x[9] + in[2] * x[10] + x[11];
  w = in[0] * x[12] + in[1] * x[13] + in[2] * x[14] + x[15];
  out = out * (1. / w);
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::Private::ActorToDisplay(
  const svtkVector3d& actor, svtkVector2i& out) const
{
  svtkVector2d v;
  this->ActorToDisplay(actor, v);
  out = svtkVector2i(v.Cast<int>().GetData());
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::Private::ActorToDisplay(
  const svtkVector3d& actor, svtkVector2d& v) const
{
  // This is adapted from svtkCoordinate's world to display conversion. We
  // reimplement it here for efficiency.

  // svtkRenderer::WorldToView (AMVP includes the actor matrix, too)
  const svtkTuple<double, 16>& x = this->AMVP;
  double w;
  v[0] = actor[0] * x[0] + actor[1] * x[1] + actor[2] * x[2] + x[3];
  v[1] = actor[0] * x[4] + actor[1] * x[5] + actor[2] * x[6] + x[7];
  w = actor[0] * x[12] + actor[1] * x[13] + actor[2] * x[14] + x[15];
  v = v * (1. / w);

  // svtkViewport::ViewToNormalizedViewport
  v[0] = this->NormalizedViewPort[0] +
    ((v[0] + 1.) / 2.) * (this->NormalizedViewPort[2] - this->NormalizedViewPort[0]);
  v[1] = this->NormalizedViewPort[1] +
    ((v[1] + 1.) / 2.) * (this->NormalizedViewPort[3] - this->NormalizedViewPort[1]);
  v[0] = (v[0] - this->ViewPort[0]) / (this->ViewPort[2] - this->ViewPort[0]);
  v[1] = (v[1] - this->ViewPort[1]) / (this->ViewPort[3] - this->ViewPort[1]);

  // svtkViewport::NormalizedViewportToViewport
  v[0] *= this->ViewPortSize[0] - 1.;
  v[1] *= this->ViewPortSize[1] - 1.;

  // svtkViewport::ViewportToNormalizedDisplay
  // svtkViewport::NormalizedDisplayToDisplay
  v[0] += this->DisplayOffset[0];
  v[1] += this->DisplayOffset[1];
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::Private::SetViewInfo(svtkRenderer* ren, svtkActor* act)
{
  svtkCamera* cam = ren->GetActiveCamera();
  if (!cam)
  {
    svtkGenericWarningMacro(<< "No active camera on renderer.");
    return false;
  }

  svtkMatrix4x4* mat = cam->GetModelViewTransformMatrix();
  this->CameraRight.Set(mat->GetElement(0, 0), mat->GetElement(0, 1), mat->GetElement(0, 2));
  this->CameraUp.Set(mat->GetElement(1, 0), mat->GetElement(1, 1), mat->GetElement(1, 2));
  this->CameraForward.Set(mat->GetElement(2, 0), mat->GetElement(2, 1), mat->GetElement(2, 2));

  // figure out the same aspect ratio used by the render engine
  // (see svtkOpenGLCamera::Render())
  int lowerLeft[2];
  int usize, vsize;
  double aspect1[2];
  double aspect2[2];
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);
  ren->ComputeAspect();
  ren->GetAspect(aspect1);
  ren->svtkViewport::ComputeAspect();
  ren->svtkViewport::GetAspect(aspect2);
  double aspectModification = (aspect1[0] * aspect2[1]) / (aspect1[1] * aspect2[0]);
  double aspect = aspectModification * usize / vsize;

  // Get the mvp (mcdc) matrix
  double mvp[16];
  mat = cam->GetCompositeProjectionTransformMatrix(aspect, -1, 1);
  svtkMatrix4x4::DeepCopy(mvp, mat);

  // Apply the actor's matrix:
  svtkMatrix4x4::DeepCopy(this->ActorMatrix.GetData(), act->GetMatrix());
  svtkMatrix4x4::Multiply4x4(mvp, this->ActorMatrix.GetData(), this->AMVP.GetData());

  svtkMatrix4x4::Invert(this->ActorMatrix.GetData(), this->InverseActorMatrix.GetData());

  if (svtkWindow* win = ren->GetSVTKWindow())
  {
    const int* size = win->GetSize();
    this->WindowSize[0] = size[0];
    this->WindowSize[1] = size[1];

    size = ren->GetSize();
    this->ViewPortSize[0] = size[0];
    this->ViewPortSize[1] = size[1];

    ren->GetViewport(this->ViewPort.GetData());

    double* tvport = win->GetTileViewport();
    this->NormalizedViewPort[0] = std::max(this->ViewPort[0], tvport[0]);
    this->NormalizedViewPort[1] = std::max(this->ViewPort[1], tvport[1]);
    this->NormalizedViewPort[2] = std::min(this->ViewPort[2], tvport[2]);
    this->NormalizedViewPort[3] = std::min(this->ViewPort[3], tvport[3]);

    this->ViewportBounds[0] = this->ViewPort[0] * this->WindowSize[0];
    this->ViewportBounds[1] = this->ViewPort[2] * this->WindowSize[0];
    this->ViewportBounds[2] = this->ViewPort[1] * this->WindowSize[1];
    this->ViewportBounds[3] = this->ViewPort[3] * this->WindowSize[1];

    this->DisplayOffset[0] = static_cast<double>(this->ViewportBounds[0]) + 0.5;
    this->DisplayOffset[1] = static_cast<double>(this->ViewportBounds[2]) + 0.5;
  }
  else
  {
    svtkGenericWarningMacro(<< "No render window present.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::Private::LineCanBeLabeled(
  svtkPoints* points, svtkIdType numIds, const svtkIdType* ids, const LabelMetric& metrics)
{
  svtkTuple<int, 4> bbox(0);
  svtkVector3d actorCoord;
  svtkVector2i displayCoord;
  if (numIds > 0)
  {
    do
    {
      points->GetPoint(*(ids++), actorCoord.GetData());
      this->ActorToDisplay(actorCoord, displayCoord);
      --numIds;
    } while (numIds > 0 && !this->PixelIsVisible(displayCoord));

    if (!this->PixelIsVisible(displayCoord))
    {
      // No visible points
      return false;
    }

    bbox[0] = displayCoord.GetX();
    bbox[1] = displayCoord.GetX();
    bbox[2] = displayCoord.GetY();
    bbox[3] = displayCoord.GetY();
  }
  while (numIds-- > 0)
  {
    points->GetPoint(*(ids++), actorCoord.GetData());
    this->ActorToDisplay(actorCoord, displayCoord);
    if (this->PixelIsVisible(displayCoord))
    {
      bbox[0] = std::min(bbox[0], displayCoord.GetX());
      bbox[1] = std::max(bbox[1], displayCoord.GetX());
      bbox[2] = std::min(bbox[2], displayCoord.GetY());
      bbox[3] = std::max(bbox[3], displayCoord.GetY());
    }
  }

  // Must be at least twice the label length in at least one direction:
  return (
    metrics.Dimensions[0] * 2 < bbox[1] - bbox[0] || metrics.Dimensions[0] * 2 < bbox[3] - bbox[2]);
}

//------------------------------------------------------------------------------
template <typename ScalarType>
bool svtkLabeledContourMapper::Private::PixelIsVisible(const svtkVector2<ScalarType>& dispCoord) const
{
  return (dispCoord.GetX() >= this->ViewportBounds[0] &&
    dispCoord.GetX() <= this->ViewportBounds[1] && dispCoord.GetY() >= this->ViewportBounds[2] &&
    dispCoord.GetY() <= this->ViewportBounds[3]);
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::Private::NextLabel(svtkPoints* points, svtkIdType& numIds,
  const svtkIdType*& ids, const LabelMetric& metrics, LabelInfo& info, double targetSmoothness,
  double skipDistance)
{
  if (numIds < 3)
  {
    return false;
  }

  // First point in this call to NextLabel (index into ids).
  svtkIdType firstIdx = 0;
  svtkVector3d firstPoint;
  svtkVector2d firstPointDisplay;
  points->GetPoint(ids[firstIdx], firstPoint.GetData());
  this->ActorToDisplay(firstPoint, firstPointDisplay);

  // Start of current smooth run (index into ids).
  svtkIdType startIdx = 0;
  svtkVector3d startPoint;
  svtkVector2d startPointDisplay;
  points->GetPoint(ids[startIdx], startPoint.GetData());
  this->ActorToDisplay(startPoint, startPointDisplay);

  // Accumulated length of segments since startId
  std::vector<double> segmentLengths;
  double rAccum = 0.;

  // Straight-line distance from start --> previous
  double rPrevStraight = 0.;

  // Straight-line distance from start --> current
  double rStraight = 0.;

  // Straight-line distance from prev --> current
  double rSegment = 0.;

  // Minimum length of a smooth segment in display space
  const double minLength = 1.2 * metrics.Dimensions[0];

  // Vector of segment prev --> current
  svtkVector2d segment(0, 0);

  // Vector of segment start --> current
  svtkVector2d prevStraight(0, 0);
  svtkVector2d straight(0, 0);

  // Smoothness of start --> current
  double smoothness = 0;

  // Account for skip distance:
  while (segment.Norm() < skipDistance)
  {
    ++startIdx;
    points->GetPoint(ids[startIdx], startPoint.GetData());
    this->ActorToDisplay(startPoint, startPointDisplay);

    segment = startPointDisplay - firstPointDisplay;
  }

  // Find the first visible point
  while (startIdx + 1 < numIds && !this->PixelIsVisible(startPointDisplay))
  {
    ++startIdx;
    points->GetPoint(ids[startIdx], startPoint.GetData());
    this->ActorToDisplay(startPoint, startPointDisplay);
  }

  // Start point in current segment.
  svtkVector3d prevPoint = startPoint;
  svtkVector2d prevPointDisplay = startPointDisplay;

  // End point of current segment (index into ids).
  svtkIdType curIdx = startIdx + 1;
  svtkVector3d curPoint = prevPoint;
  svtkVector2d curPointDisplay = prevPointDisplay;

  while (curIdx < numIds)
  {
    // Copy cur --> prev
    prevPoint = curPoint;
    prevPointDisplay = curPointDisplay;
    prevStraight = straight;
    rPrevStraight = rStraight;

    // Update current:
    points->GetPoint(ids[curIdx], curPoint.GetData());
    this->ActorToDisplay(curPoint, curPointDisplay);

    // Calculate lengths and smoothness.
    segment = curPointDisplay - prevPointDisplay;
    straight = curPointDisplay - startPointDisplay;
    rSegment = segment.Norm();
    rStraight = straight.Norm();
    segmentLengths.push_back(rSegment);
    rAccum += rSegment;
    if (rStraight == 0.0)
    {
      ++curIdx;
      continue;
    }
    smoothness = calculateSmoothness(rAccum, rStraight);

    // Are we still dealing with a reasonably smooth line?
    // The first check tests if we've traveled far enough to get a fair estimate
    // of smoothness.
    if (rAccum < 10. || smoothness <= targetSmoothness)
    {
      // Advance to the next point:
      ++curIdx;
      continue;
    }
    else
    {
      // The line is no longer smooth "enough". Was start --> previous long
      // enough (twice label width)?
      if (rPrevStraight >= minLength)
      {
        // We have a winner!
        break;
      }
      else
      {
        // This startIdx won't work. On to the next visible startIdx.
        do
        {
          ++startIdx;
          points->GetPoint(ids[startIdx], startPoint.GetData());
          this->ActorToDisplay(startPoint, startPointDisplay);
        } while (startIdx < numIds && !this->PixelIsVisible(startPointDisplay));

        prevPoint = startPoint;
        prevPointDisplay = startPointDisplay;
        curPoint = startPoint;
        curPointDisplay = startPointDisplay;
        curIdx = startIdx + 1;
        rAccum = 0.;
        rPrevStraight = 0.;
        segmentLengths.clear();
        continue;
      }
    }
  }

  // Was the last segment ok?
  if (rPrevStraight >= minLength)
  {
    // The final index of the segment:
    svtkIdType endIdx = curIdx - 1;

    // The direction of the text.
    svtkVector3d prevPointWorld;
    svtkVector3d startPointWorld;
    this->ActorToWorld(prevPoint, prevPointWorld);
    this->ActorToWorld(startPoint, startPointWorld);
    info.RightW = (prevPointWorld - startPointWorld).Normalized();
    // Ensure the text reads left->right:
    if (info.RightW.Dot(this->CameraRight) < 0.)
    {
      info.RightW = -info.RightW;
    }

    // The up vector. Cross the forward direction with the orientation and
    // ensure that the result vector is in the same hemisphere as CameraUp
    info.UpW = info.RightW.Compare(this->CameraForward, 10e-10)
      ? this->CameraUp
      : info.RightW.Cross(this->CameraForward).Normalized();
    if (info.UpW.Dot(this->CameraUp) < 0.)
    {
      info.UpW = -info.UpW;
    }

    // Walk through the segment lengths to find where the center is for label
    // placement:
    double targetLength = rPrevStraight * 0.5;
    rAccum = 0.;
    size_t endIdxOffset = 1;
    for (; endIdxOffset <= segmentLengths.size(); ++endIdxOffset)
    {
      rSegment = segmentLengths[endIdxOffset - 1];
      double tmp = rAccum + rSegment;
      if (tmp > targetLength)
      {
        break;
      }
      rAccum = tmp;
    }
    targetLength -= rAccum;
    points->GetPoint(ids[startIdx + endIdxOffset - 1], prevPoint.GetData());
    points->GetPoint(ids[startIdx + endIdxOffset], curPoint.GetData());
    svtkVector3d offset = curPoint - prevPoint;
    double rSegmentActor = offset.Normalize();
    offset = offset * (targetLength * rSegmentActor / rSegment);
    info.Position = prevPoint + offset;

    this->ComputeLabelInfo(info, metrics);

    // Update the cell array:
    ids += endIdx;
    numIds -= endIdx;

    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
bool svtkLabeledContourMapper::Private::BuildLabel(
  svtkTextActor3D* actor, const LabelMetric& metric, const LabelInfo& info)

{
  assert(metric.Valid);
  actor->SetInput(metric.Text.c_str());
  actor->SetTextProperty(metric.TProp);
  actor->SetPosition(const_cast<double*>(info.Position.GetData()));

  svtkNew<svtkTransform> xform;
  xform->PostMultiply();

  xform->Translate((-info.Position).GetData());

  xform->Scale(info.ScaleDisplayToActor, info.ScaleDisplayToActor, info.ScaleDisplayToActor);

  svtkVector3d forward = info.UpA.Cross(info.RightA);
  double rot[16];
  rot[4 * 0 + 0] = info.RightA[0];
  rot[4 * 1 + 0] = info.RightA[1];
  rot[4 * 2 + 0] = info.RightA[2];
  rot[4 * 3 + 0] = 0;
  rot[4 * 0 + 1] = info.UpA[0];
  rot[4 * 1 + 1] = info.UpA[1];
  rot[4 * 2 + 1] = info.UpA[2];
  rot[4 * 3 + 1] = 0;
  rot[4 * 0 + 2] = forward[0];
  rot[4 * 1 + 2] = forward[1];
  rot[4 * 2 + 2] = forward[2];
  rot[4 * 3 + 2] = 0;
  rot[4 * 0 + 3] = 0;
  rot[4 * 1 + 3] = 0;
  rot[4 * 2 + 3] = 0;
  rot[4 * 3 + 3] = 1;
  xform->Concatenate(rot);

  xform->Translate(info.Position.GetData());
  actor->SetUserTransform(xform);

  return true;
}

//------------------------------------------------------------------------------
void svtkLabeledContourMapper::Private::ComputeLabelInfo(LabelInfo& info, const LabelMetric& metrics)
{
  // Convert the right and up vectors into actor space:
  svtkVector3d worldPosition;
  this->ActorToWorld(info.Position, worldPosition);

  svtkVector3d endW = worldPosition + info.RightW;
  svtkVector3d endA;
  this->WorldToActor(endW, endA);
  info.RightA = endA - info.Position;

  endW = worldPosition + info.UpW;
  this->WorldToActor(endW, endA);
  info.UpA = endA - info.Position;

  // Compute scaling factor. Use the Up vector for deltas as we know it is
  // perpendicular to the view axis:
  svtkVector3d delta = info.UpA * (0.5 * metrics.Dimensions[0]);
  svtkVector3d leftActor = info.Position - delta;
  svtkVector3d rightActor = info.Position + delta;
  svtkVector2d leftDisplay;
  svtkVector2d rightDisplay;
  this->ActorToDisplay(leftActor, leftDisplay);
  this->ActorToDisplay(rightActor, rightDisplay);
  info.ScaleDisplayToActor =
    static_cast<double>(metrics.Dimensions[0]) / (rightDisplay - leftDisplay).Norm();

  // Compute the corners of the quad. Actor coordinates are used to create the
  // stencil, display coordinates are used to detect collisions.
  // Note that we make this a little bigger (4px) than a tight bbox to give a
  // little breathing room around the text.
  svtkVector3d halfWidth =
    ((0.5 * metrics.Dimensions[0] + 2) * info.ScaleDisplayToActor) * info.RightA;
  svtkVector3d halfHeight =
    ((0.5 * metrics.Dimensions[1] + 2) * info.ScaleDisplayToActor) * info.UpA;
  info.TLa = info.Position + halfHeight - halfWidth;
  info.TRa = info.Position + halfHeight + halfWidth;
  info.BRa = info.Position - halfHeight + halfWidth;
  info.BLa = info.Position - halfHeight - halfWidth;
  this->ActorToDisplay(info.TLa, info.TLd);
  this->ActorToDisplay(info.TRa, info.TRd);
  this->ActorToDisplay(info.BRa, info.BRd);
  this->ActorToDisplay(info.BLa, info.BLd);
}

// Anonymous namespace for some TestOverlap helpers:
namespace
{

// Rotates the vector by -90 degrees.
void perp(svtkVector2i& vec)
{
  std::swap(vec[0], vec[1]);
  vec[1] = -vec[1];
}

// Project all points in other onto the line (point + t * direction).
// Return true if t is positive for all points in other (e.g. all points in
// 'other' are outside the polygon containing 'point').
bool allOutside(const svtkVector2i& point, const svtkVector2i& direction, const LabelInfo& other)
{
  svtkVector2i testVector;

  testVector = other.TLd - point;
  if (direction.Dot(testVector) <= 0)
  {
    return false;
  }

  testVector = other.TRd - point;
  if (direction.Dot(testVector) <= 0)
  {
    return false;
  }

  testVector = other.BRd - point;
  if (direction.Dot(testVector) <= 0)
  {
    return false;
  }

  testVector = other.BLd - point;
  if (direction.Dot(testVector) <= 0)
  {
    return false;
  }

  return true;
}

// Generate a vector pointing out from each edge of the rectangle. Do this
// by traversing the corners counter-clockwise and using the perp() function.
// Use allOutside() to determine whether the other polygon is outside the edge.
// Return true if the axis separates the polygons.
bool testAxis(const LabelInfo& poly, const svtkVector2i& edgeStart, const svtkVector2i& edgeEnd)
{
  // Vector pointing out of polygon:
  svtkVector2i direction = edgeEnd - edgeStart;
  perp(direction);

  return allOutside(edgeStart, direction, poly);
}

} // end anon namespace

//------------------------------------------------------------------------------
// Implements axis separation method for detecting polygon intersection.
// Ref: http://www.geometrictools.com/Documentation/MethodOfSeparatingAxes.pdf
// In essence, look for an axis that separates the two rectangles.
// Return true if overlap occurs.
bool svtkLabeledContourMapper::Private::TestOverlap(const LabelInfo& a, const LabelInfo& b)
{
  // Note that the order of the points matters, must be CCW to get the correct
  // perpendicular vector:
  return !(testAxis(a, b.TLd, b.BLd) || testAxis(a, b.BLd, b.BRd) || testAxis(a, b.BRd, b.TRd) ||
    testAxis(a, b.TRd, b.TLd) || testAxis(b, a.TLd, a.BLd) || testAxis(b, a.BLd, a.BRd) ||
    testAxis(b, a.BRd, a.TRd) || testAxis(b, a.TRd, a.TLd));
}
