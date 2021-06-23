/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleDrawPolygon.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleDrawPolygon.h"

#include "svtkCommand.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVectorOperators.h"

svtkStandardNewMacro(svtkInteractorStyleDrawPolygon);

//-----------------------------------------------------------------------------
class svtkInteractorStyleDrawPolygon::svtkInternal
{
public:
  std::vector<svtkVector2i> points;

  void AddPoint(const svtkVector2i& point) { this->points.push_back(point); }

  void AddPoint(int x, int y) { this->AddPoint(svtkVector2i(x, y)); }

  svtkVector2i GetPoint(svtkIdType index) const { return this->points[index]; }

  svtkIdType GetNumberOfPoints() const { return static_cast<svtkIdType>(this->points.size()); }

  void Clear() { this->points.clear(); }

  void DrawPixels(
    const svtkVector2i& StartPos, const svtkVector2i& EndPos, unsigned char* pixels, const int* size)
  {
    int x1 = StartPos.GetX(), x2 = EndPos.GetX();
    int y1 = StartPos.GetY(), y2 = EndPos.GetY();

    double x = x2 - x1;
    double y = y2 - y1;
    double length = sqrt(x * x + y * y);
    if (length == 0)
    {
      return;
    }
    double addx = x / length;
    double addy = y / length;

    x = x1;
    y = y1;
    int row, col;
    for (double i = 0; i < length; i += 1)
    {
      col = (int)x;
      row = (int)y;
      pixels[3 * (row * size[0] + col)] = 255 ^ pixels[3 * (row * size[0] + col)];
      pixels[3 * (row * size[0] + col) + 1] = 255 ^ pixels[3 * (row * size[0] + col) + 1];
      pixels[3 * (row * size[0] + col) + 2] = 255 ^ pixels[3 * (row * size[0] + col) + 2];
      x += addx;
      y += addy;
    }
  }
};

//----------------------------------------------------------------------------
svtkInteractorStyleDrawPolygon::svtkInteractorStyleDrawPolygon()
{
  this->Internal = new svtkInternal();
  this->StartPosition[0] = this->StartPosition[1] = 0;
  this->EndPosition[0] = this->EndPosition[1] = 0;
  this->Moving = 0;
  this->DrawPolygonPixels = true;
  this->PixelArray = svtkUnsignedCharArray::New();
}

//----------------------------------------------------------------------------
svtkInteractorStyleDrawPolygon::~svtkInteractorStyleDrawPolygon()
{
  this->PixelArray->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
std::vector<svtkVector2i> svtkInteractorStyleDrawPolygon::GetPolygonPoints()
{
  return this->Internal->points;
}

//----------------------------------------------------------------------------
void svtkInteractorStyleDrawPolygon::OnMouseMove()
{
  if (!this->Interactor || !this->Moving)
  {
    return;
  }

  this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
  this->EndPosition[1] = this->Interactor->GetEventPosition()[1];
  const int* size = this->Interactor->GetRenderWindow()->GetSize();
  if (this->EndPosition[0] > (size[0] - 1))
  {
    this->EndPosition[0] = size[0] - 1;
  }
  if (this->EndPosition[0] < 0)
  {
    this->EndPosition[0] = 0;
  }
  if (this->EndPosition[1] > (size[1] - 1))
  {
    this->EndPosition[1] = size[1] - 1;
  }
  if (this->EndPosition[1] < 0)
  {
    this->EndPosition[1] = 0;
  }

  svtkVector2i lastPoint = this->Internal->GetPoint(this->Internal->GetNumberOfPoints() - 1);
  svtkVector2i newPoint(this->EndPosition[0], this->EndPosition[1]);
  if ((lastPoint - newPoint).SquaredNorm() > 100)
  {
    this->Internal->AddPoint(newPoint);
    if (this->DrawPolygonPixels)
    {
      this->DrawPolygon();
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleDrawPolygon::OnLeftButtonDown()
{
  if (!this->Interactor)
  {
    return;
  }
  this->Moving = 1;

  svtkRenderWindow* renWin = this->Interactor->GetRenderWindow();

  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];

  this->PixelArray->Initialize();
  this->PixelArray->SetNumberOfComponents(3);
  const int* size = renWin->GetSize();
  this->PixelArray->SetNumberOfTuples(size[0] * size[1]);

  renWin->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 1, this->PixelArray);
  this->Internal->Clear();
  this->Internal->AddPoint(this->StartPosition[0], this->StartPosition[1]);
  this->InvokeEvent(svtkCommand::StartInteractionEvent);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleDrawPolygon::OnLeftButtonUp()
{
  if (!this->Interactor || !this->Moving)
  {
    return;
  }

  if (this->DrawPolygonPixels)
  {
    const int* size = this->Interactor->GetRenderWindow()->GetSize();
    unsigned char* pixels = this->PixelArray->GetPointer(0);
    this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0] - 1, size[1] - 1, pixels, 0);
    this->Interactor->GetRenderWindow()->Frame();
  }

  this->Moving = 0;
  this->InvokeEvent(svtkCommand::SelectionChangedEvent);
  this->InvokeEvent(svtkCommand::EndInteractionEvent);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleDrawPolygon::DrawPolygon()
{
  svtkNew<svtkUnsignedCharArray> tmpPixelArray;
  tmpPixelArray->DeepCopy(this->PixelArray);
  unsigned char* pixels = tmpPixelArray->GetPointer(0);
  const int* size = this->Interactor->GetRenderWindow()->GetSize();

  // draw each line segment
  for (svtkIdType i = 0; i < this->Internal->GetNumberOfPoints() - 1; i++)
  {
    const svtkVector2i& a = this->Internal->GetPoint(i);
    const svtkVector2i& b = this->Internal->GetPoint(i + 1);

    this->Internal->DrawPixels(a, b, pixels, size);
  }

  // draw a line from the end to the start
  if (this->Internal->GetNumberOfPoints() >= 3)
  {
    const svtkVector2i& start = this->Internal->GetPoint(0);
    const svtkVector2i& end = this->Internal->GetPoint(this->Internal->GetNumberOfPoints() - 1);

    this->Internal->DrawPixels(start, end, pixels, size);
  }

  this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0] - 1, size[1] - 1, pixels, 0);
  this->Interactor->GetRenderWindow()->Frame();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleDrawPolygon::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Moving: " << this->Moving << endl;
  os << indent << "DrawPolygonPixels: " << this->DrawPolygonPixels << endl;
  os << indent << "StartPosition: " << this->StartPosition[0] << "," << this->StartPosition[1]
     << endl;
  os << indent << "EndPosition: " << this->EndPosition[0] << "," << this->EndPosition[1] << endl;
}
