/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkXYPlotRepresentation.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Written by Philippe Pebay, Kitware SAS 2012

#include "svtkXYPlotRepresentation.h"

#include "svtkObjectFactory.h"
#include "svtkPropCollection.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"
#include "svtkXYPlotActor.h"

svtkStandardNewMacro(svtkXYPlotRepresentation);

//-----------------------------------------------------------------------------
svtkXYPlotRepresentation::svtkXYPlotRepresentation()
{
  //    this->PositionCoordinate->SetValue( 0.0, 0.0 );
  //    this->Position2Coordinate->SetValue( 0.7, 0.65 );

  this->XYPlotActor = nullptr;
  svtkXYPlotActor* actor = svtkXYPlotActor::New();
  this->SetXYPlotActor(actor);
  actor->Delete();

  this->ShowBorder = svtkBorderRepresentation::BORDER_ACTIVE;
  this->BWActor->VisibilityOff();
}

//-----------------------------------------------------------------------------
svtkXYPlotRepresentation::~svtkXYPlotRepresentation()
{
  this->SetXYPlotActor(nullptr);
}

//-----------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetXYPlotActor(svtkXYPlotActor* actor)
{
  if (this->XYPlotActor != actor)
  {
    svtkSmartPointer<svtkXYPlotActor> oldActor = this->XYPlotActor;
    svtkSetObjectBodyMacro(XYPlotActor, svtkXYPlotActor, actor);
  }
}

//-----------------------------------------------------------------------------
void svtkXYPlotRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XYPlotActor: " << this->XYPlotActor << endl;
}

//-----------------------------------------------------------------------------
void svtkXYPlotRepresentation::BuildRepresentation()
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetPosition(this->GetPosition());
    this->XYPlotActor->SetPosition2(this->GetPosition2());
  }

  this->Superclass::BuildRepresentation();
}

//-----------------------------------------------------------------------------
void svtkXYPlotRepresentation::WidgetInteraction(double eventPos[2])
{
  // Let superclass move things around.
  this->Superclass::WidgetInteraction(eventPos);
}

//-----------------------------------------------------------------------------
int svtkXYPlotRepresentation::GetVisibility()
{
  return this->XYPlotActor->GetVisibility();
}

//-----------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetVisibility(int vis)
{
  this->XYPlotActor->SetVisibility(vis);
}

//-----------------------------------------------------------------------------
void svtkXYPlotRepresentation::GetActors2D(svtkPropCollection* collection)
{
  if (this->XYPlotActor)
  {
    collection->AddItem(this->XYPlotActor);
  }
  this->Superclass::GetActors2D(collection);
}

//-----------------------------------------------------------------------------
void svtkXYPlotRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->ReleaseGraphicsResources(w);
  }
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int svtkXYPlotRepresentation::RenderOverlay(svtkViewport* w)
{
  int count = this->Superclass::RenderOverlay(w);
  if (this->XYPlotActor)
  {
    count += this->XYPlotActor->RenderOverlay(w);
  }
  return count;
}

//-------------------------------------------------------------------------
int svtkXYPlotRepresentation::RenderOpaqueGeometry(svtkViewport* w)
{
  int count = this->Superclass::RenderOpaqueGeometry(w);
  if (this->XYPlotActor)
  {
    count += this->XYPlotActor->RenderOpaqueGeometry(w);
  }
  return count;
}

//-------------------------------------------------------------------------
int svtkXYPlotRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  if (this->XYPlotActor)
  {
    count += this->XYPlotActor->RenderTranslucentPolygonalGeometry(w);
  }
  return count;
}

//-------------------------------------------------------------------------
svtkTypeBool svtkXYPlotRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  if (this->XYPlotActor)
  {
    result |= this->XYPlotActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetBorder(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetBorder(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitle(const char* title)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitle(title);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetXTitle(const char* title)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetXTitle(title);
  }
}

//-------------------------------------------------------------------------------
char* svtkXYPlotRepresentation::GetXTitle()
{
  if (this->XYPlotActor)
  {
    return this->XYPlotActor->GetXTitle();
  }
  return 0;
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetXRange(double xmin, double xmax)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetXRange(xmin, xmax);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetYTitle(const char* title)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetYTitle(title);
  }
}

//-------------------------------------------------------------------------------
char* svtkXYPlotRepresentation::GetYTitle()
{
  if (this->XYPlotActor)
  {
    return this->XYPlotActor->GetXTitle();
  }
  return 0;
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetYRange(double ymin, double ymax)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetYRange(ymin, ymax);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetYTitlePosition(int pos)
{
  if (this->XYPlotActor)
  {
    switch (pos)
    {
      case 0:
        this->XYPlotActor->SetYTitlePositionToTop();
        break;
      case 1:
        this->XYPlotActor->SetYTitlePositionToHCenter();
        break;
      case 2:
        this->XYPlotActor->SetYTitlePositionToVCenter();
        break;
    }
  }
}

//-------------------------------------------------------------------------------
int svtkXYPlotRepresentation::GetYTitlePosition() const
{
  if (this->XYPlotActor)
  {
    return this->XYPlotActor->GetYTitlePosition();
  }
  return 0;
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetXAxisColor(double r, double g, double b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetXAxisColor(r, g, b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetYAxisColor(double r, double g, double b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetYAxisColor(r, g, b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetXValues(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetXValues(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetLegend(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetLegend(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetLegendBorder(int b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetLegendBorder(b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetLegendBox(int b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetLegendBox(b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetLegendBoxColor(double r, double g, double b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetLegendBoxColor(r, g, b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetLegendPosition(double x, double y)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetLegendPosition(x, y);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetLegendPosition2(double x, double y)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetLegendPosition2(x, y);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetLineWidth(double w)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetLineWidth(w);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetPlotColor(int i, int r, int g, int b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetPlotColor(i, r / 255.0, g / 255.0, b / 255.0);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetPlotLines(int i)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetPlotLines(i);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetPlotPoints(int i)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetPlotPoints(i);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetPlotLabel(int i, const char* label)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetPlotLabel(i, label);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetPlotGlyphType(int curve, int glyph)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetPlotGlyphType(curve, glyph);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetGlyphSize(double x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetGlyphSize(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::AddUserCurvesPoint(double c, double x, double y)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->AddUserCurvesPoint(c, x, y);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::RemoveAllActiveCurves()
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->RemoveAllActiveCurves();
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleColor(double r, double g, double b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleColor(r, g, b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleFontFamily(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleFontFamily(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleBold(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleBold(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleItalic(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleItalic(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleShadow(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleShadow(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleFontSize(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleFontSize(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleJustification(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleJustification(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitleVerticalJustification(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitleVerticalJustification(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAdjustTitlePosition(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAdjustTitlePosition(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetTitlePosition(double x, double y)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetTitlePosition(x, y);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleColor(double r, double g, double b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleColor(r, g, b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleFontFamily(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleFontFamily(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleBold(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleBold(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleItalic(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleItalic(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleShadow(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleShadow(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleFontSize(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleFontSize(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleJustification(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleJustification(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisTitleVerticalJustification(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisTitleVerticalJustification(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelColor(double r, double g, double b)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelColor(r, g, b);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelFontFamily(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelFontFamily(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelBold(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelBold(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelItalic(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelItalic(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelShadow(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelShadow(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelFontSize(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelFontSize(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelJustification(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelJustification(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetAxisLabelVerticalJustification(int x)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetAxisLabelVerticalJustification(x);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetXLabelFormat(const char* arg)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetXLabelFormat(arg);
  }
}

//-------------------------------------------------------------------------------
void svtkXYPlotRepresentation::SetYLabelFormat(const char* arg)
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->SetYLabelFormat(arg);
  }
}
