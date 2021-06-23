/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewTheme.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkViewTheme.h"

#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkTextProperty.h"

svtkStandardNewMacro(svtkViewTheme);
svtkCxxSetObjectMacro(svtkViewTheme, PointLookupTable, svtkScalarsToColors);
svtkCxxSetObjectMacro(svtkViewTheme, CellLookupTable, svtkScalarsToColors);
svtkCxxSetObjectMacro(svtkViewTheme, PointTextProperty, svtkTextProperty);
svtkCxxSetObjectMacro(svtkViewTheme, CellTextProperty, svtkTextProperty);

svtkViewTheme::svtkViewTheme()
{
  this->PointSize = 5;
  this->LineWidth = 1;

  this->PointColor[0] = this->PointColor[1] = this->PointColor[2] = 1;
  this->PointOpacity = 1;
  svtkLookupTable* plut = svtkLookupTable::New();
  plut->SetHueRange(0.667, 0);
  plut->SetSaturationRange(1, 1);
  plut->SetValueRange(1, 1);
  plut->SetAlphaRange(1, 1);
  plut->Build();
  this->PointLookupTable = plut;

  this->CellColor[0] = this->CellColor[1] = this->CellColor[2] = 1;
  this->CellOpacity = 0.5;
  svtkLookupTable* clut = svtkLookupTable::New();
  clut->SetHueRange(0.667, 0);
  clut->SetSaturationRange(0.5, 1);
  clut->SetValueRange(0.5, 1);
  clut->SetAlphaRange(0.5, 1);
  clut->Build();
  this->CellLookupTable = clut;

  this->OutlineColor[0] = this->OutlineColor[1] = this->OutlineColor[2] = 0;

  this->SelectedPointColor[0] = this->SelectedPointColor[2] = 1;
  this->SelectedPointColor[1] = 0;
  this->SelectedPointOpacity = 1;
  this->SelectedCellColor[0] = this->SelectedCellColor[2] = 1;
  this->SelectedCellColor[1] = 0;
  this->SelectedCellOpacity = 1;

  this->BackgroundColor[0] = this->BackgroundColor[1] = this->BackgroundColor[2] = 0.0;
  this->BackgroundColor2[0] = this->BackgroundColor2[1] = this->BackgroundColor2[2] = 0.3;

  this->ScalePointLookupTable = true;
  this->ScaleCellLookupTable = true;

  this->PointTextProperty = svtkTextProperty::New();
  this->PointTextProperty->SetColor(1, 1, 1);
  this->PointTextProperty->BoldOn();
  this->PointTextProperty->SetJustificationToCentered();
  this->PointTextProperty->SetVerticalJustificationToCentered();
  this->PointTextProperty->SetFontSize(12);
  this->CellTextProperty = svtkTextProperty::New();
  this->CellTextProperty->SetColor(0.7, 0.7, 0.7);
  this->CellTextProperty->BoldOn();
  this->CellTextProperty->SetJustificationToCentered();
  this->CellTextProperty->SetVerticalJustificationToCentered();
  this->CellTextProperty->SetFontSize(10);
}

svtkViewTheme::~svtkViewTheme()
{
  if (this->CellLookupTable)
  {
    this->CellLookupTable->Delete();
  }
  if (this->PointLookupTable)
  {
    this->PointLookupTable->Delete();
  }
  if (this->CellTextProperty)
  {
    this->CellTextProperty->Delete();
  }
  if (this->PointTextProperty)
  {
    this->PointTextProperty->Delete();
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetPointHueRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetHueRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetPointHueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetHueRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetPointHueRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetHueRange();
  }
  return nullptr;
}

void svtkViewTheme::GetPointHueRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetHueRange(mn, mx);
  }
}

void svtkViewTheme::GetPointHueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetHueRange(rng);
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetPointSaturationRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetSaturationRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetPointSaturationRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetSaturationRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetPointSaturationRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetSaturationRange();
  }
  return nullptr;
}

void svtkViewTheme::GetPointSaturationRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetSaturationRange(mn, mx);
  }
}

void svtkViewTheme::GetPointSaturationRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetSaturationRange(rng);
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetPointValueRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetValueRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetPointValueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetValueRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetPointValueRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetValueRange();
  }
  return nullptr;
}

void svtkViewTheme::GetPointValueRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetValueRange(mn, mx);
  }
}

void svtkViewTheme::GetPointValueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetValueRange(rng);
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetPointAlphaRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetAlphaRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetPointAlphaRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetAlphaRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetPointAlphaRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetAlphaRange();
  }
  return nullptr;
}

void svtkViewTheme::GetPointAlphaRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetAlphaRange(mn, mx);
  }
}

void svtkViewTheme::GetPointAlphaRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetAlphaRange(rng);
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetCellHueRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetHueRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetCellHueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetHueRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetCellHueRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetHueRange();
  }
  return nullptr;
}

void svtkViewTheme::GetCellHueRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetHueRange(mn, mx);
  }
}

void svtkViewTheme::GetCellHueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetHueRange(rng);
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetCellSaturationRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetSaturationRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetCellSaturationRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetSaturationRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetCellSaturationRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetSaturationRange();
  }
  return nullptr;
}

void svtkViewTheme::GetCellSaturationRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetSaturationRange(mn, mx);
  }
}

void svtkViewTheme::GetCellSaturationRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetSaturationRange(rng);
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetCellValueRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetValueRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetCellValueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetValueRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetCellValueRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetValueRange();
  }
  return nullptr;
}

void svtkViewTheme::GetCellValueRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetValueRange(mn, mx);
  }
}

void svtkViewTheme::GetCellValueRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetValueRange(rng);
  }
}

//---------------------------------------------------------------------------
void svtkViewTheme::SetCellAlphaRange(double mn, double mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetAlphaRange(mn, mx);
    lut->Build();
  }
}

void svtkViewTheme::SetCellAlphaRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetAlphaRange(rng);
    lut->Build();
  }
}

double* svtkViewTheme::GetCellAlphaRange()
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetAlphaRange();
  }
  return nullptr;
}

void svtkViewTheme::GetCellAlphaRange(double& mn, double& mx)
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetAlphaRange(mn, mx);
  }
}

void svtkViewTheme::GetCellAlphaRange(double rng[2])
{
  if (svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetAlphaRange(rng);
  }
}

void svtkViewTheme::SetVertexLabelColor(double r, double g, double b)
{
  if (this->PointTextProperty)
  {
    this->PointTextProperty->SetColor(r, g, b);
  }
}

double* svtkViewTheme::GetVertexLabelColor()
{
  return this->PointTextProperty ? this->PointTextProperty->GetColor() : nullptr;
}

void svtkViewTheme::SetEdgeLabelColor(double r, double g, double b)
{
  if (this->CellTextProperty)
  {
    this->CellTextProperty->SetColor(r, g, b);
  }
}

double* svtkViewTheme::GetEdgeLabelColor()
{
  return this->CellTextProperty ? this->CellTextProperty->GetColor() : nullptr;
}

svtkViewTheme* svtkViewTheme::CreateOceanTheme()
{
  svtkViewTheme* theme = svtkViewTheme::New();

  theme->SetPointSize(7);
  theme->SetLineWidth(3);

  theme->SetBackgroundColor(.8, .8, .8);
  theme->SetBackgroundColor2(1, 1, 1);
  theme->GetPointTextProperty()->SetColor(0, 0, 0);
  theme->GetCellTextProperty()->SetColor(.2, .2, .2);

  theme->SetPointColor(0.5, 0.5, 0.5);
  theme->SetPointHueRange(0.667, 0);
  theme->SetPointSaturationRange(1, 1);
  theme->SetPointValueRange(0.75, 0.75);

  theme->SetCellColor(0.25, 0.25, 0.25);
  theme->SetCellOpacity(0.5);
  theme->SetCellHueRange(0.667, 0);
  theme->SetCellAlphaRange(0.75, 1);
  theme->SetCellValueRange(0.75, .75);
  theme->SetCellSaturationRange(1, 1);

  theme->SetOutlineColor(0, 0, 0);

  theme->SetSelectedPointColor(.9, .4, .9);
  theme->SetSelectedCellColor(.8, .3, .8);

  return theme;
}

svtkViewTheme* svtkViewTheme::CreateNeonTheme()
{
  svtkViewTheme* theme = svtkViewTheme::New();

  theme->SetPointSize(7);
  theme->SetLineWidth(3);

  theme->SetBackgroundColor(.2, .2, .4);
  theme->SetBackgroundColor2(.1, .1, .2);
  theme->GetPointTextProperty()->SetColor(1, 1, 1);
  theme->GetCellTextProperty()->SetColor(.7, .7, .7);

  theme->SetPointColor(0.5, 0.5, 0.6);
  theme->SetPointHueRange(0.6, 0);
  theme->SetPointSaturationRange(1, 1);
  theme->SetPointValueRange(1, 1);

  theme->SetCellColor(0.5, 0.5, 0.7);
  theme->SetCellOpacity(0.5);
  theme->SetCellHueRange(0.57, 0);
  theme->SetCellAlphaRange(0.75, 1);
  theme->SetCellValueRange(0.75, 1);
  theme->SetCellSaturationRange(1, 1);

  theme->SetOutlineColor(0, 0, 0);

  theme->SetSelectedPointColor(.9, .4, .9);
  theme->SetSelectedCellColor(.8, .3, .8);

  return theme;
}

svtkViewTheme* svtkViewTheme::CreateMellowTheme()
{
  svtkViewTheme* theme = svtkViewTheme::New();

  theme->SetPointSize(7);
  theme->SetLineWidth(2);

  theme->SetBackgroundColor(0.3, 0.3, 0.25); // Darker Tan
  theme->SetBackgroundColor2(0.6, 0.6, 0.5); // Tan
  theme->GetPointTextProperty()->SetColor(1, 1, 1);
  theme->GetCellTextProperty()->SetColor(.7, .7, 1);

  theme->SetPointColor(0.0, 0.0, 1.0);
  theme->SetPointHueRange(0.667, 0);

  theme->SetCellColor(0.25, 0.25, 0.25);
  theme->SetCellOpacity(0.4);
  theme->SetCellHueRange(0.667, 0);
  theme->SetCellAlphaRange(0.4, 1);
  theme->SetCellValueRange(0.5, 1);
  theme->SetCellSaturationRange(0.5, 1);

  theme->SetOutlineColor(0, 0, 0);

  theme->SetSelectedPointColor(1, 1, 1);
  theme->SetSelectedCellColor(0, 0, 0);

  return theme;
}

bool svtkViewTheme::LookupMatchesPointTheme(svtkScalarsToColors* s2c)
{
  if (!s2c)
  {
    return false;
  }
  svtkLookupTable* lut = svtkLookupTable::SafeDownCast(s2c);
  if (!lut)
  {
    return false;
  }
  if (lut->GetHueRange()[0] == this->GetPointHueRange()[0] &&
    lut->GetHueRange()[1] == this->GetPointHueRange()[1] &&
    lut->GetSaturationRange()[0] == this->GetPointSaturationRange()[0] &&
    lut->GetSaturationRange()[1] == this->GetPointSaturationRange()[1] &&
    lut->GetValueRange()[0] == this->GetPointValueRange()[0] &&
    lut->GetValueRange()[1] == this->GetPointValueRange()[1] &&
    lut->GetAlphaRange()[0] == this->GetPointAlphaRange()[0] &&
    lut->GetAlphaRange()[1] == this->GetPointAlphaRange()[1])
  {
    return true;
  }
  return false;
}

bool svtkViewTheme::LookupMatchesCellTheme(svtkScalarsToColors* s2c)
{
  if (!s2c)
  {
    return false;
  }
  svtkLookupTable* lut = svtkLookupTable::SafeDownCast(s2c);
  if (!lut)
  {
    return false;
  }
  if (lut->GetHueRange()[0] == this->GetCellHueRange()[0] &&
    lut->GetHueRange()[1] == this->GetCellHueRange()[1] &&
    lut->GetSaturationRange()[0] == this->GetCellSaturationRange()[0] &&
    lut->GetSaturationRange()[1] == this->GetCellSaturationRange()[1] &&
    lut->GetValueRange()[0] == this->GetCellValueRange()[0] &&
    lut->GetValueRange()[1] == this->GetCellValueRange()[1] &&
    lut->GetAlphaRange()[0] == this->GetCellAlphaRange()[0] &&
    lut->GetAlphaRange()[1] == this->GetCellAlphaRange()[1])
  {
    return true;
  }
  return false;
}

void svtkViewTheme::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PointSize: " << this->PointSize << endl;
  os << indent << "LineWidth: " << this->LineWidth << endl;
  os << indent << "PointColor: " << this->PointColor[0] << "," << this->PointColor[1] << ","
     << this->PointColor[2] << endl;
  os << indent << "PointOpacity: " << this->PointOpacity << endl;
  os << indent << "CellColor: " << this->CellColor[0] << "," << this->CellColor[1] << ","
     << this->CellColor[2] << endl;
  os << indent << "CellOpacity: " << this->CellOpacity << endl;
  os << indent << "OutlineColor: " << this->OutlineColor[0] << "," << this->OutlineColor[1] << ","
     << this->OutlineColor[2] << endl;
  os << indent << "SelectedPointColor: " << this->SelectedPointColor[0] << ","
     << this->SelectedPointColor[1] << "," << this->SelectedPointColor[2] << endl;
  os << indent << "SelectedPointOpacity: " << this->SelectedPointOpacity << endl;
  os << indent << "SelectedCellColor: " << this->SelectedCellColor[0] << ","
     << this->SelectedCellColor[1] << "," << this->SelectedCellColor[2] << endl;
  os << indent << "SelectedCellOpacity: " << this->SelectedCellOpacity << endl;
  os << indent << "BackgroundColor: " << this->BackgroundColor[0] << "," << this->BackgroundColor[1]
     << "," << this->BackgroundColor[2] << endl;
  os << indent << "BackgroundColor2: " << this->BackgroundColor2[0] << ","
     << this->BackgroundColor2[1] << "," << this->BackgroundColor2[2] << endl;
  os << indent << "PointLookupTable: " << (this->PointLookupTable ? "" : "(none)") << endl;
  if (this->PointLookupTable)
  {
    this->PointLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "CellLookupTable: " << (this->CellLookupTable ? "" : "(none)") << endl;
  if (this->CellLookupTable)
  {
    this->CellLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "PointTextProperty: " << (this->PointTextProperty ? "" : "(none)") << endl;
  if (this->PointTextProperty)
  {
    this->PointTextProperty->PrintSelf(os, indent.GetNextIndent());
    os << indent << "VertexLabelColor: " << this->PointTextProperty->GetColor()[0] << ","
       << this->PointTextProperty->GetColor()[1] << "," << this->PointTextProperty->GetColor()[2]
       << endl;
  }
  os << indent << "CellTextProperty: " << (this->CellTextProperty ? "" : "(none)") << endl;
  if (this->CellTextProperty)
  {
    this->CellTextProperty->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelColor: " << this->CellTextProperty->GetColor()[0] << ","
       << this->CellTextProperty->GetColor()[1] << "," << this->CellTextProperty->GetColor()[2]
       << endl;
  }
  os << indent << "ScalePointLookupTable: " << this->ScalePointLookupTable << endl;
  os << indent << "ScaleCellLookupTable: " << this->ScaleCellLookupTable << endl;
}
