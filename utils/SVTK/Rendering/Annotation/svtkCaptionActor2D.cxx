/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCaptionActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCaptionActor2D.h"

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkGlyph2D.h"
#include "svtkGlyph3D.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkTrivialProducer.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkCaptionActor2D);

svtkCxxSetObjectMacro(svtkCaptionActor2D, CaptionTextProperty, svtkTextProperty);

class svtkCaptionActor2DConnection : public svtkAlgorithm
{
public:
  static svtkCaptionActor2DConnection* New();
  svtkTypeMacro(svtkCaptionActor2DConnection, svtkAlgorithm);

  svtkCaptionActor2DConnection() { this->SetNumberOfInputPorts(1); }
};

svtkStandardNewMacro(svtkCaptionActor2DConnection);

//----------------------------------------------------------------------------
svtkCaptionActor2D::svtkCaptionActor2D()
{
  // Positioning information
  this->AttachmentPointCoordinate = svtkCoordinate::New();
  this->AttachmentPointCoordinate->SetCoordinateSystemToWorld();
  this->AttachmentPointCoordinate->SetValue(0.0, 0.0, 0.0);

  this->PositionCoordinate->SetCoordinateSystemToDisplay();
  this->PositionCoordinate->SetReferenceCoordinate(this->AttachmentPointCoordinate);
  this->PositionCoordinate->SetValue(static_cast<double>(10), static_cast<double>(10));

  // This sets up the Position2Coordinate
  this->svtkActor2D::SetWidth(0.25);
  this->svtkActor2D::SetHeight(0.10);

  this->Border = 1;
  this->Leader = 1;
  this->AttachEdgeOnly = 0;
  this->ThreeDimensionalLeader = 1;
  this->LeaderGlyphSize = 0.025;
  this->MaximumLeaderGlyphSize = 20;

  this->LeaderGlyphConnectionHolder = svtkCaptionActor2DConnection::New();

  // Control font properties
  this->Padding = 3;

  this->CaptionTextProperty = svtkTextProperty::New();
  this->CaptionTextProperty->SetBold(1);
  this->CaptionTextProperty->SetItalic(1);
  this->CaptionTextProperty->SetShadow(1);
  this->CaptionTextProperty->SetFontFamily(SVTK_ARIAL);
  this->CaptionTextProperty->SetJustification(SVTK_TEXT_LEFT);
  this->CaptionTextProperty->SetVerticalJustification(SVTK_TEXT_BOTTOM);

  // What is actually drawn
  this->TextActor = svtkTextActor::New();
  this->TextActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->TextActor->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);
  this->TextActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->TextActor->GetPosition2Coordinate()->SetReferenceCoordinate(nullptr);
  this->TextActor->SetTextScaleModeToProp();
  this->TextActor->SetTextProperty(this->CaptionTextProperty);

  this->BorderPolyData = svtkPolyData::New();
  svtkPoints* pts = svtkPoints::New();
  pts->SetNumberOfPoints(4);
  this->BorderPolyData->SetPoints(pts);
  pts->Delete();
  svtkCellArray* border = svtkCellArray::New();
  border->InsertNextCell(5);
  border->InsertCellPoint(0);
  border->InsertCellPoint(1);
  border->InsertCellPoint(2);
  border->InsertCellPoint(3);
  border->InsertCellPoint(0);
  this->BorderPolyData->SetLines(border);
  border->Delete();

  this->BorderMapper = svtkPolyDataMapper2D::New();
  this->BorderMapper->SetInputData(this->BorderPolyData);
  this->BorderActor = svtkActor2D::New();
  this->BorderActor->SetMapper(this->BorderMapper);

  // Set border mapper coordinate system to Display.
  svtkCoordinate* coord = svtkCoordinate::New();
  coord->SetCoordinateSystemToDisplay();
  this->BorderMapper->SetTransformCoordinate(coord);
  coord->Delete();

  // This is for glyphing the head of the leader
  // A single point with a vector for glyph orientation
  this->HeadPolyData = svtkPolyData::New();
  pts = svtkPoints::New();
  pts->SetNumberOfPoints(1);
  this->HeadPolyData->SetPoints(pts);
  pts->Delete();
  svtkDoubleArray* vecs = svtkDoubleArray::New();
  vecs->SetNumberOfComponents(3);
  vecs->SetNumberOfTuples(1);
  this->HeadPolyData->GetPointData()->SetVectors(vecs);
  vecs->Delete();

  // This is the leader (line) from the attachment point to the caption
  this->LeaderPolyData = svtkPolyData::New();
  pts = svtkPoints::New();
  pts->SetNumberOfPoints(2);
  this->LeaderPolyData->SetPoints(pts);
  pts->Delete();
  svtkCellArray* leader = svtkCellArray::New();
  leader->InsertNextCell(2);
  leader->InsertCellPoint(0);
  leader->InsertCellPoint(1); // at the attachment point
  this->LeaderPolyData->SetLines(leader);
  leader->Delete();

  // Used to generate the glyph on the leader head
  this->HeadGlyph = svtkGlyph3D::New();
  this->HeadGlyph->SetInputData(this->HeadPolyData);
  this->HeadGlyph->SetScaleModeToDataScalingOff();
  this->HeadGlyph->SetScaleFactor(0.1);

  // Appends the leader and the glyph head
  this->AppendLeader = svtkAppendPolyData::New();
  this->AppendLeader->AddInputData(this->LeaderPolyData);
  this->AppendLeader->AddInputConnection(this->HeadGlyph->GetOutputPort());

  // Used to transform from world to other coordinate systems
  this->MapperCoordinate2D = svtkCoordinate::New();
  this->MapperCoordinate2D->SetCoordinateSystemToWorld();

  // If 2D leader is used, then use this mapper/actor combination
  this->LeaderMapper2D = svtkPolyDataMapper2D::New();
  this->LeaderMapper2D->SetTransformCoordinate(this->MapperCoordinate2D);
  this->LeaderActor2D = svtkActor2D::New();
  this->LeaderActor2D->SetMapper(this->LeaderMapper2D);

  // If 3D leader is used, then use this mapper/actor combination
  this->LeaderMapper3D = svtkPolyDataMapper::New();
  this->LeaderActor3D = svtkActor::New();
  this->LeaderActor3D->SetMapper(this->LeaderMapper3D);
}

//----------------------------------------------------------------------------
svtkCaptionActor2D::~svtkCaptionActor2D()
{
  this->AttachmentPointCoordinate->Delete();

  this->TextActor->Delete();

  this->LeaderGlyphConnectionHolder->Delete();
  this->LeaderGlyphConnectionHolder = nullptr;

  this->BorderPolyData->Delete();
  this->BorderMapper->Delete();
  this->BorderActor->Delete();

  this->HeadPolyData->Delete();
  this->LeaderPolyData->Delete();
  this->HeadGlyph->Delete();
  this->AppendLeader->Delete();

  this->MapperCoordinate2D->Delete();

  this->LeaderMapper2D->Delete();
  this->LeaderActor2D->Delete();

  this->LeaderMapper3D->Delete();
  this->LeaderActor3D->Delete();

  this->SetCaptionTextProperty(nullptr);
}

//----------------------------------------------------------------------------
void svtkCaptionActor2D::SetLeaderGlyphConnection(svtkAlgorithmOutput* ao)
{
  this->LeaderGlyphConnectionHolder->SetInputConnection(ao);
}

//----------------------------------------------------------------------------
void svtkCaptionActor2D::SetLeaderGlyphData(svtkPolyData* leader)
{
  svtkTrivialProducer* tp = svtkTrivialProducer::New();
  tp->SetOutput(leader);
  this->SetLeaderGlyphConnection(tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
svtkPolyData* svtkCaptionActor2D::GetLeaderGlyph()
{
  if (this->LeaderGlyphConnectionHolder->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(this->LeaderGlyphConnectionHolder->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
void svtkCaptionActor2D::SetCaption(const char* caption)
{
  this->TextActor->SetInput(caption);
}

//----------------------------------------------------------------------------
char* svtkCaptionActor2D::GetCaption()
{
  return this->TextActor->GetInput();
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void svtkCaptionActor2D::ReleaseGraphicsResources(svtkWindow* win)
{
  this->TextActor->ReleaseGraphicsResources(win);
  this->BorderActor->ReleaseGraphicsResources(win);
  this->LeaderActor2D->ReleaseGraphicsResources(win);
  this->LeaderActor3D->ReleaseGraphicsResources(win);
}

int svtkCaptionActor2D::RenderOverlay(svtkViewport* viewport)
{
  int renderedSomething = 0;

  renderedSomething += this->TextActor->RenderOverlay(viewport);

  if (this->Border)
  {
    renderedSomething += this->BorderActor->RenderOverlay(viewport);
  }

  if (this->Leader)
  {
    if (this->ThreeDimensionalLeader)
    {
      renderedSomething += this->LeaderActor3D->RenderOverlay(viewport);
    }
    else
    {
      renderedSomething += this->LeaderActor2D->RenderOverlay(viewport);
    }
  }

  return renderedSomething;
}

//----------------------------------------------------------------------------
int svtkCaptionActor2D::RenderOpaqueGeometry(svtkViewport* viewport)
{
  // Build the caption (almost always needed so we don't check mtime)
  svtkDebugMacro(<< "Rebuilding caption");

  // compute coordinates and set point values
  //
  double *w1, *w2;
  int *x1, *x2, *x3;
  double p1[4], p2[4], p3[4];
  x1 = this->AttachmentPointCoordinate->GetComputedDisplayValue(viewport);
  x2 = this->PositionCoordinate->GetComputedDisplayValue(viewport);
  x3 = this->Position2Coordinate->GetComputedDisplayValue(viewport);
  p1[0] = (double)x1[0];
  p1[1] = (double)x1[1];
  p1[2] = 0.0;
  p2[0] = (double)x2[0];
  p2[1] = (double)x2[1];
  p2[2] = p1[2];
  p3[0] = (double)x3[0];
  p3[1] = (double)x3[1];
  p3[2] = p1[2];

  // Set up the scaled text - take into account the padding
  this->TextActor->SetTextProperty(this->CaptionTextProperty);
  this->TextActor->GetPositionCoordinate()->SetValue(
    p2[0] + this->Padding, p2[1] + this->Padding, 0.0);
  this->TextActor->GetPosition2Coordinate()->SetValue(
    p3[0] - this->Padding, p3[1] - this->Padding, 0.0);

  // Define the border
  svtkPoints* pts = this->BorderPolyData->GetPoints();
  pts->SetPoint(0, p2);
  pts->SetPoint(1, p3[0], p2[1], p1[2]);
  pts->SetPoint(2, p3[0], p3[1], p1[2]);
  pts->SetPoint(3, p2[0], p3[1], p1[2]);

  // Define the leader. Have to find the closest point from the
  // border to the attachment point. We look at the four vertices
  // and four edge centers.
  double d2, minD2, pt[3], minPt[3];
  minD2 = SVTK_DOUBLE_MAX;

  minPt[0] = p2[0];
  minPt[1] = p2[1];

  pt[0] = p2[0];
  pt[1] = p2[1];
  pt[2] = minPt[2] = 0.0;
  if (!this->AttachEdgeOnly && (d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  pt[0] = (p2[0] + p3[0]) / 2.0;
  if ((d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  pt[0] = p3[0];
  if (!this->AttachEdgeOnly && (d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  pt[1] = (p2[1] + p3[1]) / 2.0;
  if ((d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  pt[1] = p3[1];
  if (!this->AttachEdgeOnly && (d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  pt[0] = (p2[0] + p3[0]) / 2.0;
  if ((d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  pt[0] = p2[0];
  if (!this->AttachEdgeOnly && (d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  pt[1] = (p2[1] + p3[1]) / 2.0;
  if ((d2 = svtkMath::Distance2BetweenPoints(p1, pt)) < minD2)
  {
    minD2 = d2;
    minPt[0] = pt[0];
    minPt[1] = pt[1];
  }

  // Set the leader coordinates in appropriate coordinate system
  // The pipeline is connected differently depending on the dimension
  // and availability of a leader head.
  if (this->Leader)
  {
    pts = this->LeaderPolyData->GetPoints();

    w1 = this->AttachmentPointCoordinate->GetComputedWorldValue(viewport);
    viewport->SetWorldPoint(w1[0], w1[1], w1[2], 1.0);
    viewport->WorldToView();
    viewport->GetViewPoint(p1);

    // minPt is in display coordinates and it is OK
    double val[3];
    val[0] = minPt[0];
    val[1] = minPt[1];
    val[2] = 0;
    // convert to view
    viewport->DisplayToNormalizedDisplay(val[0], val[1]);
    viewport->NormalizedDisplayToViewport(val[0], val[1]);
    viewport->ViewportToNormalizedViewport(val[0], val[1]);
    viewport->NormalizedViewportToView(val[0], val[1], val[2]);

    // use the zvalue from the attach point
    val[2] = p1[2];
    viewport->SetViewPoint(val);
    viewport->ViewToWorld();
    double w3[4];
    viewport->GetWorldPoint(w3);
    if (w3[3] != 0.0)
    {
      w3[0] /= w3[3];
      w3[1] /= w3[3];
      w3[2] /= w3[3];
    }
    w2 = w3;

    pts->SetPoint(0, w1);
    pts->SetPoint(1, w2);
    this->HeadPolyData->GetPoints()->SetPoint(0, w1);
    this->HeadPolyData->GetPointData()->GetVectors()->SetTuple3(
      0, w1[0] - w2[0], w1[1] - w2[1], w1[2] - w2[2]);

    pts->Modified();
    this->HeadPolyData->Modified();
  }

  if (this->GetLeaderGlyph())
  {
    this->LeaderGlyphConnectionHolder->GetInputAlgorithm()->Update();

    // compute the scale
    double length = this->GetLeaderGlyph()->GetLength();
    const int* sze = viewport->GetSize();
    int numPixels = static_cast<int>(
      this->LeaderGlyphSize * sqrt(static_cast<double>(sze[0] * sze[0] + sze[1] * sze[1])));
    numPixels =
      (numPixels > this->MaximumLeaderGlyphSize ? this->MaximumLeaderGlyphSize : numPixels);

    // determine the number of units length per pixel
    viewport->SetDisplayPoint(sze[0] / 2, sze[1] / 2, 0);
    viewport->DisplayToWorld();
    viewport->GetWorldPoint(p1);
    if (p1[3] != 0.0)
    {
      p1[0] /= p1[3];
      p1[1] /= p1[3];
      p1[2] /= p1[3];
    }

    viewport->SetDisplayPoint(sze[0] / 2 + 1, sze[1] / 2 + 1, 0);
    viewport->DisplayToWorld();
    viewport->GetWorldPoint(p2);
    if (p2[3] != 0.0)
    {
      p2[0] /= p2[3];
      p2[1] /= p2[3];
      p2[2] /= p2[3];
    }

    // Arbitrary 1.5 factor makes up for the use of "diagonals" in length
    // calculations; otherwise the scale factor tends to be too small
    double sf = 1.5 * numPixels * sqrt(svtkMath::Distance2BetweenPoints(p1, p2)) / length;

    svtkDebugMacro(<< "Scale factor: " << sf);

    this->HeadGlyph->SetSourceData(this->GetLeaderGlyph());
    this->HeadGlyph->SetScaleFactor(sf);

    this->LeaderMapper2D->SetInputConnection(this->AppendLeader->GetOutputPort());
    this->LeaderMapper3D->SetInputConnection(this->AppendLeader->GetOutputPort());
    this->AppendLeader->Update();
  }
  else
  {
    this->LeaderMapper2D->SetInputData(this->LeaderPolyData);
    this->LeaderMapper3D->SetInputData(this->LeaderPolyData);
  }

  // assign properties
  //
  this->BorderActor->SetProperty(this->GetProperty());
  this->LeaderActor2D->SetProperty(this->GetProperty());
  this->LeaderActor3D->GetProperty()->SetColor(this->GetProperty()->GetColor());
  // Copy the property into the text actor and reset the color -- otherwise the
  // text shadow will be colored the same as the text.
  this->TextActor->GetProperty()->DeepCopy(this->GetProperty());
  this->TextActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  this->TextActor->GetProperty()->SetOpacity(1.0);

  // Okay we are ready to render something
  int renderedSomething = 0;
  renderedSomething += this->TextActor->RenderOpaqueGeometry(viewport);
  if (this->Border)
  {
    renderedSomething += this->BorderActor->RenderOpaqueGeometry(viewport);
  }

  if (this->Leader)
  {
    if (this->ThreeDimensionalLeader)
    {
      renderedSomething += this->LeaderActor3D->RenderOpaqueGeometry(viewport);
    }
    else
    {
      renderedSomething += this->LeaderActor2D->RenderOpaqueGeometry(viewport);
    }
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkCaptionActor2D::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
void svtkCaptionActor2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Text Actor: " << this->TextActor << "\n";
  if (this->CaptionTextProperty)
  {
    os << indent << "Caption Text Property:\n";
    this->CaptionTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Caption Text Property: (none)\n";
  }

  os << indent << "Caption: ";
  if (this->TextActor->GetInput())
  {
    os << this->TextActor->GetInput() << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Leader: " << (this->Leader ? "On\n" : "Off\n");
  os << indent << "Three Dimensional Leader: " << (this->ThreeDimensionalLeader ? "On\n" : "Off\n");
  os << indent << "Leader Glyph Size: " << this->LeaderGlyphSize << "\n";
  os << indent << "MaximumLeader Glyph Size: " << this->MaximumLeaderGlyphSize << "\n";
  if (!this->GetLeaderGlyph())
  {
    os << indent << "Leader Glyph: (none)\n";
  }
  else
  {
    os << indent << "Leader Glyph: (" << this->GetLeaderGlyph() << ")\n";
  }
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "AttachEdgeOnly: " << (this->AttachEdgeOnly ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void svtkCaptionActor2D::ShallowCopy(svtkProp* prop)
{
  svtkCaptionActor2D* a = svtkCaptionActor2D::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetCaption(a->GetCaption());
    this->SetAttachmentPoint(a->GetAttachmentPoint());
    this->SetBorder(a->GetBorder());
    this->SetLeader(a->GetLeader());
    this->SetThreeDimensionalLeader(a->GetThreeDimensionalLeader());
    if (a->LeaderGlyphConnectionHolder->GetNumberOfInputConnections(0) < 1)
    {
      this->SetLeaderGlyphConnection(nullptr);
    }
    else
    {
      this->SetLeaderGlyphConnection(a->LeaderGlyphConnectionHolder->GetInputConnection(0, 0));
    }
    this->SetLeaderGlyphSize(a->GetLeaderGlyphSize());
    this->SetMaximumLeaderGlyphSize(a->GetMaximumLeaderGlyphSize());
    this->SetPadding(a->GetPadding());
    this->SetCaptionTextProperty(a->GetCaptionTextProperty());
  }

  // Now do superclass
  this->svtkActor2D::ShallowCopy(prop);
}
