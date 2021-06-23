/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPieChartActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPieChartActor.h"

#include "svtkAxisActor2D.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFieldData.h"
#include "svtkGlyphSource2D.h"
#include "svtkLegendBoxActor.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkTrivialProducer.h"
#include "svtkUnsignedCharArray.h"
#include "svtkViewport.h"
#include "svtkWindow.h"

#include <string>
#include <vector>

svtkStandardNewMacro(svtkPieChartActor);

svtkCxxSetObjectMacro(svtkPieChartActor, LabelTextProperty, svtkTextProperty);
svtkCxxSetObjectMacro(svtkPieChartActor, TitleTextProperty, svtkTextProperty);

// PIMPL'd list of labels
class svtkPieceLabelArray : public std::vector<std::string>
{
};

class svtkPieChartActorConnection : public svtkAlgorithm
{
public:
  static svtkPieChartActorConnection* New();
  svtkTypeMacro(svtkPieChartActorConnection, svtkAlgorithm);

  svtkPieChartActorConnection() { this->SetNumberOfInputPorts(1); }
};

svtkStandardNewMacro(svtkPieChartActorConnection);

//----------------------------------------------------------------------------
// Instantiate object
svtkPieChartActor::svtkPieChartActor()
{
  // Actor2D positions
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.1, 0.1);
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.9, 0.8);
  this->Position2Coordinate->SetReferenceCoordinate(nullptr);

  this->ConnectionHolder = svtkPieChartActorConnection::New();

  this->ArrayNumber = 0;
  this->ComponentNumber = 0;
  this->TitleVisibility = 1;
  this->Title = nullptr;
  this->Labels = new svtkPieceLabelArray;
  this->PieceMappers = nullptr;
  this->PieceActors = nullptr;
  this->LabelTextProperty = svtkTextProperty::New();
  this->LabelTextProperty->SetFontSize(12);
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(0);
  this->LabelTextProperty->SetFontFamilyToArial();
  this->TitleTextProperty = svtkTextProperty::New();
  this->TitleTextProperty->ShallowCopy(this->LabelTextProperty);
  this->TitleTextProperty->SetFontSize(24);
  this->TitleTextProperty->SetBold(1);
  this->TitleTextProperty->SetItalic(0);
  this->TitleTextProperty->SetShadow(1);
  this->TitleTextProperty->SetFontFamilyToArial();
  this->LabelVisibility = 1;

  this->LegendVisibility = 1;

  this->LegendActor = svtkLegendBoxActor::New();
  this->LegendActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetReferenceCoordinate(nullptr);
  this->LegendActor->BorderOff();
  this->LegendActor->SetNumberOfEntries(100); // initial allocation
  this->LegendActor->SetPadding(2);
  this->LegendActor->ScalarVisibilityOff();
  this->GlyphSource = svtkGlyphSource2D::New();
  this->GlyphSource->SetGlyphTypeToNone();
  this->GlyphSource->DashOn();
  this->GlyphSource->FilledOff();
  this->GlyphSource->Update();

  this->PlotData = svtkPolyData::New();
  this->PlotMapper = svtkPolyDataMapper2D::New();
  this->PlotMapper->SetInputData(this->PlotData);
  this->PlotActor = svtkActor2D::New();
  this->PlotActor->SetMapper(this->PlotMapper);

  this->TitleMapper = svtkTextMapper::New();
  this->TitleActor = svtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();

  this->N = 0;
  this->Total = 0.0;
  this->Fractions = nullptr;

  this->WebData = svtkPolyData::New();
  this->WebMapper = svtkPolyDataMapper2D::New();
  this->WebMapper->SetInputData(this->WebData);
  this->WebActor = svtkActor2D::New();
  this->WebActor->SetMapper(this->WebMapper);

  this->LastPosition[0] = this->LastPosition[1] = this->LastPosition2[0] = this->LastPosition2[1] =
    0;

  this->P1[0] = this->P1[1] = this->P2[0] = this->P2[1] = 0.0;
}

//----------------------------------------------------------------------------
svtkPieChartActor::~svtkPieChartActor()
{
  this->ConnectionHolder->Delete();
  this->ConnectionHolder = nullptr;

  delete[] this->Title;
  this->Title = nullptr;

  delete this->Labels;
  this->SetLabelTextProperty(nullptr);
  this->SetTitleTextProperty(nullptr);

  this->LegendActor->Delete();
  this->GlyphSource->Delete();

  this->Initialize();

  this->TitleMapper->Delete();
  this->TitleMapper = nullptr;
  this->TitleActor->Delete();
  this->TitleActor = nullptr;

  this->WebData->Delete();
  this->WebMapper->Delete();
  this->WebActor->Delete();

  this->PlotData->Delete();
  this->PlotMapper->Delete();
  this->PlotActor->Delete();
}

//----------------------------------------------------------------------------
void svtkPieChartActor::SetInputConnection(svtkAlgorithmOutput* ao)
{
  this->ConnectionHolder->SetInputConnection(ao);
}

//----------------------------------------------------------------------------
void svtkPieChartActor::SetInputData(svtkDataObject* dobj)
{
  svtkTrivialProducer* tp = svtkTrivialProducer::New();
  tp->SetOutput(dobj);
  this->SetInputConnection(tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPieChartActor::GetInput()
{
  return this->ConnectionHolder->GetInputDataObject(0, 0);
}

//----------------------------------------------------------------------------
// Free-up axes and related stuff
void svtkPieChartActor::Initialize()
{
  if (this->PieceActors)
  {
    for (int i = 0; i < this->N; i++)
    {
      this->PieceMappers[i]->Delete();
      this->PieceActors[i]->Delete();
    }
    delete[] this->PieceMappers;
    this->PieceMappers = nullptr;
    delete[] this->PieceActors;
    this->PieceActors = nullptr;
  }

  this->N = 0;
  this->Total = 0.0;
  delete[] this->Fractions;
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int svtkPieChartActor::RenderOverlay(svtkViewport* viewport)
{
  int renderedSomething = 0;

  if (!this->BuildPlot(viewport))
  {
    return 0;
  }

  // Done rebuilding, render as appropriate.
  if (this->GetInput() == nullptr || this->N <= 0)
  {
    svtkErrorMacro(<< "Nothing to plot!");
    return 0;
  }

  if (this->TitleVisibility)
  {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
  }

  this->WebActor->SetProperty(this->GetProperty());
  renderedSomething += this->PlotActor->RenderOverlay(viewport);
  renderedSomething += this->WebActor->RenderOverlay(viewport);

  if (this->LabelVisibility)
  {
    for (int i = 0; i < this->N; i++)
    {
      renderedSomething += this->PieceActors[i]->RenderOverlay(viewport);
    }
  }

  if (this->LegendVisibility)
  {
    renderedSomething += this->LegendActor->RenderOverlay(viewport);
  }

  return renderedSomething;
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int svtkPieChartActor::RenderOpaqueGeometry(svtkViewport* viewport)
{
  int renderedSomething = 0;

  if (!this->BuildPlot(viewport))
  {
    return 0;
  }

  // Done rebuilding, render as appropriate.
  if (this->GetInput() == nullptr || this->N <= 0)
  {
    svtkErrorMacro(<< "Nothing to plot!");
    return 0;
  }

  if (this->TitleVisibility)
  {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
  }

  this->WebActor->SetProperty(this->GetProperty());
  renderedSomething += this->PlotActor->RenderOpaqueGeometry(viewport);
  renderedSomething += this->WebActor->RenderOpaqueGeometry(viewport);

  if (this->LabelVisibility)
  {
    for (int i = 0; i < this->N; i++)
    {
      renderedSomething += this->PieceActors[i]->RenderOpaqueGeometry(viewport);
    }
  }

  if (this->LegendVisibility)
  {
    renderedSomething += this->LegendActor->RenderOpaqueGeometry(viewport);
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkPieChartActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//-----------------------------------------------------------------------------
int svtkPieChartActor::BuildPlot(svtkViewport* viewport)
{
  // Initialize
  svtkDebugMacro(<< "Building pie chart plot");

  // Make sure input is up to date, and that the data is the correct shape to
  // plot.
  if (!this->GetInput())
  {
    svtkErrorMacro(<< "Nothing to plot!");
    return 0;
  }

  if (!this->TitleTextProperty)
  {
    svtkErrorMacro(<< "Need title text property to render plot");
    return 0;
  }
  if (!this->LabelTextProperty)
  {
    svtkErrorMacro(<< "Need label text property to render plot");
    return 0;
  }

  // Viewport change may not require rebuild
  int positionsHaveChanged = 0;
  if (viewport->GetMTime() > this->BuildTime ||
    (viewport->GetSVTKWindow() && viewport->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    int* lastPosition = this->PositionCoordinate->GetComputedViewportValue(viewport);
    int* lastPosition2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    if (lastPosition[0] != this->LastPosition[0] || lastPosition[1] != this->LastPosition[1] ||
      lastPosition2[0] != this->LastPosition2[0] || lastPosition2[1] != this->LastPosition2[1])
    {
      this->LastPosition[0] = lastPosition[0];
      this->LastPosition[1] = lastPosition[1];
      this->LastPosition2[0] = lastPosition2[0];
      this->LastPosition2[1] = lastPosition2[1];
      positionsHaveChanged = 1;
    }
  }

  // Check modified time to see whether we have to rebuild.
  this->ConnectionHolder->GetInputAlgorithm()->Update();

  if (positionsHaveChanged || this->GetMTime() > this->BuildTime ||
    this->GetInput()->GetMTime() > this->BuildTime ||
    this->LabelTextProperty->GetMTime() > this->BuildTime ||
    this->TitleTextProperty->GetMTime() > this->BuildTime)
  {
    svtkDebugMacro(<< "Rebuilding plot");

    // Build axes
    const int* size = viewport->GetSize();
    if (!this->PlaceAxes(viewport, size))
    {
      return 0;
    }

    this->BuildTime.Modified();
  } // If need to rebuild the plot

  return 1;
}

//----------------------------------------------------------------------------
int svtkPieChartActor::PlaceAxes(svtkViewport* viewport, const int* svtkNotUsed(size))
{
  svtkIdType i, j;
  svtkDataObject* input = this->GetInput();
  svtkFieldData* field = input->GetFieldData();
  double v = 0.0;

  this->Initialize();

  if (!field)
  {
    return 0;
  }

  // Retrieve the appropriate data array
  svtkDataArray* da = field->GetArray(this->ArrayNumber);
  if (!da)
  {
    return 0;
  }

  // Determine the number of independent variables
  this->N = da->GetNumberOfTuples();
  if (this->N <= 0 || this->N >= SVTK_ID_MAX)
  {
    this->N = 0;
    svtkErrorMacro(<< "No field data to plot");
    return 0;
  }

  // We need to loop over the field to determine the total
  this->Total = 0.0;
  this->Fractions = new double[this->N];
  for (i = 0; i < this->N; i++)
  {
    v = fabs(da->GetComponent(i, this->ComponentNumber));
    this->Fractions[i] = v;
    this->Total += v;
  }
  if (this->Total > 0.0)
  {
    double total = 0.0;
    for (i = 0; i < this->N; i++)
    {
      total += this->Fractions[i];
      this->Fractions[i] = total / this->Total;
    }
  }

  // Get the location of the corners of the box
  double* p1 = this->PositionCoordinate->GetComputedDoubleViewportValue(viewport);
  double* p2 = this->Position2Coordinate->GetComputedDoubleViewportValue(viewport);
  this->P1[0] = (p1[0] < p2[0] ? p1[0] : p2[0]);
  this->P1[1] = (p1[1] < p2[1] ? p1[1] : p2[1]);
  this->P2[0] = (p1[0] > p2[0] ? p1[0] : p2[0]);
  this->P2[1] = (p1[1] > p2[1] ? p1[1] : p2[1]);
  p1 = this->P1;
  p2 = this->P2;

  // Create the borders of the pie pieces.
  // Determine the center of the pie. Leave room for the title and the legend.
  double titleSpace = 0.0, legendSpace = 0.0;
  if (this->TitleVisibility)
  {
    titleSpace = 0.1;
  }
  if (this->LegendVisibility)
  {
    legendSpace = 0.15;
  }

  double d1 = p2[0] - legendSpace * (p2[0] - p1[0]) - p1[0];
  double d2 = p2[1] - titleSpace * (p2[1] - p1[1]) - p1[1];

  this->Center[0] = p1[0] + d1 / 2.0;
  this->Center[1] = p1[1] + d2 / 2.0;
  this->Center[2] = 0.0;
  this->Radius = (d1 < d2 ? d1 : d2);
  this->Radius /= 2.0;

  // Now generate the web points
  this->WebData->Initialize(); // remove old polydata, if any
  svtkPoints* webPts = svtkPoints::New();
  webPts->Allocate(this->N + 1);
  svtkCellArray* webLines = svtkCellArray::New();
  webLines->AllocateEstimate(this->N, 2);
  this->WebData->SetPoints(webPts);
  this->WebData->SetLines(webLines);
  svtkIdType ptId, pIds[2];
  double theta, x[3];
  x[2] = 0.0;

  // Specify the positions for the axes
  pIds[0] = webPts->InsertNextPoint(this->Center);
  for (i = 0; i < this->N; i++)
  {
    theta = this->Fractions[i] * 2.0 * svtkMath::Pi();
    x[0] = this->Center[0] + this->Radius * cos(theta);
    x[1] = this->Center[1] + this->Radius * sin(theta);
    pIds[1] = webPts->InsertNextPoint(x);
    webLines->InsertNextCell(2, pIds);
  }

  // Draw a bounding ring
  webLines->InsertNextCell(65);
  theta = 2.0 * svtkMath::Pi() / 64;
  for (j = 0; j < 65; j++)
  {
    x[0] = this->Center[0] + this->Radius * cos(j * theta);
    x[1] = this->Center[1] + this->Radius * sin(j * theta);
    ptId = webPts->InsertNextPoint(x);
    webLines->InsertCellPoint(ptId);
  }

  // Produce labels around the rim of the plot
  double thetaM;
  char label[1024];
  const char* str;
  int minFontSize = 1000, fontSize, tsize[2];
  if (this->LabelVisibility)
  {
    this->PieceActors = new svtkActor2D*[this->N];
    this->PieceMappers = new svtkTextMapper*[this->N];
    for (i = 0; i < this->N; i++)
    {
      thetaM = (i == 0 ? 0.0 : this->Fractions[i - 1] * 2.0 * svtkMath::Pi());
      theta = this->Fractions[i] * 2.0 * svtkMath::Pi();
      x[0] = this->Center[0] + (this->Radius + 5) * cos((theta + thetaM) / 2.0);
      x[1] = this->Center[1] + (this->Radius + 5) * sin((theta + thetaM) / 2.0);
      this->PieceMappers[i] = svtkTextMapper::New();
      if ((str = this->GetPieceLabel(i)) != nullptr)
      {
        this->PieceMappers[i]->SetInput(str);
      }
      else
      {
        snprintf(label, sizeof(label), "%d", static_cast<int>(i));
        this->PieceMappers[i]->SetInput(label);
      }
      this->PieceMappers[i]->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
      tsize[0] = static_cast<int>(0.15 * d1);
      tsize[1] = static_cast<int>(0.15 * d2);
      fontSize = this->PieceMappers[i]->SetConstrainedFontSize(viewport, tsize[0], tsize[1]);
      minFontSize = (fontSize < minFontSize ? fontSize : minFontSize);
      this->PieceActors[i] = svtkActor2D::New();
      this->PieceActors[i]->SetMapper(this->PieceMappers[i]);
      this->PieceActors[i]->GetPositionCoordinate()->SetCoordinateSystemToViewport();
      this->PieceActors[i]->SetPosition(x);
      // depending on the qudrant, the text is aligned differently
      if (x[0] >= this->Center[0] && x[1] >= this->Center[1])
      {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToLeft();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToBottom();
      }
      else if (x[0] < this->Center[0] && x[1] >= this->Center[1])
      {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToRight();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToBottom();
      }
      else if (x[0] < this->Center[0] && x[1] < this->Center[1])
      {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToRight();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToTop();
      }
      else if (x[0] >= this->Center[0] && x[1] < this->Center[1])
      {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToLeft();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToTop();
      }
    } // for all pieces of pie
    // Now reset font sizes to the same value
    for (i = 0; i < this->N; i++)
    {
      this->PieceMappers[i]->GetTextProperty()->SetFontSize(minFontSize);
    }
  }

  // Now generate the pie polygons
  this->PlotData->Initialize(); // remove old polydata, if any
  svtkPoints* pts = svtkPoints::New();
  pts->Allocate(this->N * 2);
  svtkCellArray* polys = svtkCellArray::New();
  svtkUnsignedCharArray* colors = svtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  this->PlotData->SetPoints(pts);
  this->PlotData->SetPolys(polys);
  this->PlotData->GetCellData()->SetScalars(colors);
  colors->Delete();

  double *color, delTheta;
  svtkIdType numDivs;
  polys->AllocateEstimate(this->N, 12);

  pIds[0] = pts->InsertNextPoint(this->Center);
  for (i = 0; i < this->N; i++)
  {
    thetaM = (i == 0 ? 0.0 : this->Fractions[i - 1] * 2.0 * svtkMath::Pi());
    theta = this->Fractions[i] * 2.0 * svtkMath::Pi();
    numDivs = static_cast<svtkIdType>(32 * (theta - thetaM) / svtkMath::Pi());
    numDivs = (numDivs < 2 ? 2 : numDivs);
    delTheta = (theta - thetaM) / numDivs;

    polys->InsertNextCell(numDivs + 2);
    polys->InsertCellPoint(pIds[0]);
    color = this->LegendActor->GetEntryColor(i);
    colors->InsertNextTuple3(255 * color[0], 255 * color[1], 255 * color[2]);
    this->LegendActor->SetEntrySymbol(i, this->GlyphSource->GetOutput());
    if ((str = this->GetPieceLabel(i)) != nullptr)
    {
      this->LegendActor->SetEntryString(i, str);
    }
    else
    {
      snprintf(label, sizeof(label), "%d", static_cast<int>(i));
      this->LegendActor->SetEntryString(i, label);
    }

    for (j = 0; j <= numDivs; j++)
    {
      theta = thetaM + j * delTheta;
      x[0] = this->Center[0] + this->Radius * cos(theta);
      x[1] = this->Center[1] + this->Radius * sin(theta);
      ptId = pts->InsertNextPoint(x);
      polys->InsertCellPoint(ptId);
    }
  }

  // Display the legend
  if (this->LegendVisibility)
  {
    this->LegendActor->GetProperty()->DeepCopy(this->GetProperty());
    this->LegendActor->GetPositionCoordinate()->SetValue(
      p1[0] + 0.85 * (p2[0] - p1[0]), p1[1] + 0.20 * (p2[1] - p1[1]));
    this->LegendActor->GetPosition2Coordinate()->SetValue(p2[0], p1[1] + 0.80 * (p2[1] - p1[1]));
  }

  // Build title
  this->TitleMapper->SetInput(this->Title);
  if (this->TitleTextProperty->GetMTime() > this->BuildTime)
  {
    // Shallow copy here since the justification is changed but we still
    // want to allow actors to share the same text property, and in that case
    // specifically allow the title and label text prop to be the same.
    this->TitleMapper->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
    this->TitleMapper->GetTextProperty()->SetJustificationToCentered();
  }

  // We could do some caching here, but hey, that's just the title
  tsize[0] = static_cast<int>(0.25 * d1);
  tsize[1] = static_cast<int>(0.15 * d2);
  this->TitleMapper->SetConstrainedFontSize(viewport, tsize[0], tsize[1]);

  this->TitleActor->GetPositionCoordinate()->SetValue(
    this->Center[0], this->Center[1] + this->Radius + tsize[1]);

  // Clean up
  webPts->Delete();
  webLines->Delete();
  pts->Delete();
  polys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void svtkPieChartActor::ReleaseGraphicsResources(svtkWindow* win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->LegendActor->ReleaseGraphicsResources(win);
  this->WebActor->ReleaseGraphicsResources(win);
  this->PlotActor->ReleaseGraphicsResources(win);
  for (int i = 0; this->PieceActors && i < this->N; i++)
  {
    this->PieceActors[i]->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
void svtkPieChartActor::SetPieceLabel(const int i, const char* label)
{
  if (i < 0)
  {
    return;
  }

  if (static_cast<unsigned int>(i) >= this->Labels->size())
  {
    this->Labels->resize(i + 1);
  }
  (*this->Labels)[i] = std::string(label);
  this->Modified();
}

//----------------------------------------------------------------------------
const char* svtkPieChartActor::GetPieceLabel(int i)
{
  if (i < 0 || static_cast<unsigned int>(i) >= this->Labels->size())
  {
    return nullptr;
  }

  return this->Labels->at(i).c_str();
}

//----------------------------------------------------------------------------
void svtkPieChartActor::SetPieceColor(int i, double r, double g, double b)
{
  this->LegendActor->SetEntryColor(i, r, g, b);
}

//----------------------------------------------------------------------------
double* svtkPieChartActor::GetPieceColor(int i)
{
  return this->LegendActor->GetEntryColor(i);
}

//----------------------------------------------------------------------------
void svtkPieChartActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: " << this->GetInput() << "\n";

  os << indent << "Title Visibility: " << (this->TitleVisibility ? "On\n" : "Off\n");

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";

  if (this->TitleTextProperty)
  {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Title Text Property: (none)\n";
  }

  os << indent << "Label Visibility: " << (this->LabelVisibility ? "On\n" : "Off\n");

  if (this->LabelTextProperty)
  {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Label Text Property: (none)\n";
  }

  os << indent << "Legend Visibility: " << (this->LegendVisibility ? "On\n" : "Off\n");

  os << indent << "Legend Actor: " << this->LegendActor << "\n";
  this->LegendActor->PrintSelf(os, indent.GetNextIndent());
}
