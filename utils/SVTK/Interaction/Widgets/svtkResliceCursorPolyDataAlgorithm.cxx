/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorPolyDataAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResliceCursorPolyDataAlgorithm.h"

#include "svtkBox.h"
#include "svtkCellArray.h"
#include "svtkClipPolyData.h"
#include "svtkCutter.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLinearExtrusionFilter.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkResliceCursor.h"
#include "svtkSmartPointer.h"
#include <algorithm>
#include <cmath>

svtkStandardNewMacro(svtkResliceCursorPolyDataAlgorithm);
svtkCxxSetObjectMacro(svtkResliceCursorPolyDataAlgorithm, ResliceCursor, svtkResliceCursor);

//---------------------------------------------------------------------------
svtkResliceCursorPolyDataAlgorithm::svtkResliceCursorPolyDataAlgorithm()
{
  this->ResliceCursor = nullptr;
  this->ReslicePlaneNormal = svtkResliceCursorPolyDataAlgorithm::XAxis;
  this->Cutter = svtkCutter::New();
  this->Box = svtkBox::New();
  this->ClipWithBox = svtkClipPolyData::New();
  this->Extrude = 0;
  this->ExtrusionFilter1 = svtkLinearExtrusionFilter::New();
  this->ExtrusionFilter2 = svtkLinearExtrusionFilter::New();
  this->ExtrusionFilter2->SetInputConnection(this->ExtrusionFilter1->GetOutputPort());

  for (int i = 0; i < 6; i++)
  {
    this->SliceBounds[i] = 0;
  }
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(4);

  for (int i = 0; i < 2; i++)
  {
    this->ThickAxes[i] = svtkPolyData::New();

    svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
    svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();

    this->ThickAxes[i]->SetPoints(points);
    this->ThickAxes[i]->SetLines(lines);
  }
}

//---------------------------------------------------------------------------
svtkResliceCursorPolyDataAlgorithm::~svtkResliceCursorPolyDataAlgorithm()
{
  this->SetResliceCursor(nullptr);
  this->Cutter->Delete();
  this->Box->Delete();
  this->ClipWithBox->Delete();
  this->ExtrusionFilter1->Delete();
  this->ExtrusionFilter2->Delete();

  for (int i = 0; i < 2; i++)
  {
    this->ThickAxes[i]->Delete();
  }
}

//---------------------------------------------------------------------------
void svtkResliceCursorPolyDataAlgorithm::BuildResliceSlabAxisTopology()
{
  for (int i = 0; i < 2; i++)
  {
    const int nPoints = this->GetResliceCursor()->GetHole() ? 8 : 4;
    this->ThickAxes[i]->GetPoints()->SetNumberOfPoints(nPoints);

    this->ThickAxes[i]->GetLines()->Reset();

    svtkIdType ptIds[2];
    for (int j = 0; j < nPoints / 2; j++)
    {
      ptIds[0] = 2 * j;
      ptIds[1] = 2 * j + 1;
      this->ThickAxes[i]->GetLines()->InsertNextCell(2, ptIds);
    }
  }
}

//---------------------------------------------------------------------------
svtkPolyData* svtkResliceCursorPolyDataAlgorithm::GetCenterlineAxis1()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(0));
}

//---------------------------------------------------------------------------
svtkPolyData* svtkResliceCursorPolyDataAlgorithm::GetCenterlineAxis2()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//---------------------------------------------------------------------------
svtkPolyData* svtkResliceCursorPolyDataAlgorithm::GetThickSlabAxis1()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(2));
}

//---------------------------------------------------------------------------
svtkPolyData* svtkResliceCursorPolyDataAlgorithm::GetThickSlabAxis2()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(3));
}

//---------------------------------------------------------------------------
int svtkResliceCursorPolyDataAlgorithm::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  if (!this->ResliceCursor)
  {
    svtkErrorMacro(<< "Reslice Cursor not set !");
    return -1;
  }

  this->BuildResliceSlabAxisTopology();

  // Cut the reslice cursor with the plane on which we are viewing.

  const int axis1 = this->GetAxis1();
  const int axis2 = this->GetAxis2();

  this->CutAndClip(
    this->ResliceCursor->GetCenterlineAxisPolyData(axis1), this->GetCenterlineAxis1());
  this->CutAndClip(
    this->ResliceCursor->GetCenterlineAxisPolyData(axis2), this->GetCenterlineAxis2());

  if (this->ResliceCursor->GetThickMode())
  {
    this->GetSlabPolyData(axis1, this->GetPlaneAxis1(), this->ThickAxes[0]);
    this->CutAndClip(this->ThickAxes[0], this->GetThickSlabAxis1());

    this->GetSlabPolyData(axis2, this->GetPlaneAxis2(), this->ThickAxes[1]);
    this->CutAndClip(this->ThickAxes[1], this->GetThickSlabAxis2());
  }

  return 1;
}

//---------------------------------------------------------------------------
void svtkResliceCursorPolyDataAlgorithm ::GetSlabPolyData(int axis, int planeAxis, svtkPolyData* pd)
{
  double normal[3], thicknessDirection[3];
  this->ResliceCursor->GetPlane(this->ReslicePlaneNormal)->GetNormal(normal);

  double* axisVector = this->ResliceCursor->GetAxis(axis);
  svtkMath::Cross(normal, axisVector, thicknessDirection);
  svtkMath::Normalize(thicknessDirection);

  const double thickness = this->ResliceCursor->GetThickness()[planeAxis];

  svtkPolyData* cpd = this->ResliceCursor->GetCenterlineAxisPolyData(axis);

  svtkPoints* pts = pd->GetPoints();

  double p[3], pPlus[3], pMinus[3];
  const int nPoints = cpd->GetNumberOfPoints();

  // Set the slab points
  for (int i = 0; i < nPoints; i++)
  {
    cpd->GetPoint(i, p);
    for (int j = 0; j < 3; j++)
    {
      pPlus[j] = p[j] + thickness * thicknessDirection[j];
      pMinus[j] = p[j] - thickness * thicknessDirection[j];
    }
    pts->SetPoint(i, pPlus);
    pts->SetPoint(nPoints + i, pMinus);
  }

  pd->Modified();
}

//---------------------------------------------------------------------------
int svtkResliceCursorPolyDataAlgorithm::GetAxis1()
{
  if (this->ReslicePlaneNormal == 2)
  {
    return 1;
  }
  if (this->ReslicePlaneNormal == 1)
  {
    return 2;
  }
  return 2;
}

//---------------------------------------------------------------------------
int svtkResliceCursorPolyDataAlgorithm::GetAxis2()
{
  if (this->ReslicePlaneNormal == 2)
  {
    return 0;
  }
  if (this->ReslicePlaneNormal == 1)
  {
    return 0;
  }
  return 1;
}

//---------------------------------------------------------------------------
int svtkResliceCursorPolyDataAlgorithm::GetPlaneAxis1()
{
  if (this->ReslicePlaneNormal == 2)
  {
    return 0;
  }
  if (this->ReslicePlaneNormal == 1)
  {
    return 0;
  }
  return 1;
}

//---------------------------------------------------------------------------
int svtkResliceCursorPolyDataAlgorithm::GetPlaneAxis2()
{
  if (this->ReslicePlaneNormal == 2)
  {
    return 1;
  }
  if (this->ReslicePlaneNormal == 1)
  {
    return 2;
  }
  return 2;
}

//---------------------------------------------------------------------------
int svtkResliceCursorPolyDataAlgorithm::GetOtherPlaneForAxis(int p)
{
  for (int i = 0; i < 3; i++)
  {
    if (i != p && i != this->ReslicePlaneNormal)
    {
      return i;
    }
  }

  return -1;
}

//---------------------------------------------------------------------------
void svtkResliceCursorPolyDataAlgorithm ::CutAndClip(svtkPolyData* input, svtkPolyData* output)
{
  this->ClipWithBox->SetClipFunction(this->Box);
  this->ClipWithBox->GenerateClipScalarsOff();
  this->ClipWithBox->GenerateClippedOutputOff();
  this->Box->SetBounds(this->ResliceCursor->GetImage()->GetBounds());

  double s[3];
  this->ResliceCursor->GetImage()->GetSpacing(s);
  const double smax = std::max(std::max(s[0], s[1]), s[2]);
  this->ExtrusionFilter1->SetScaleFactor(smax);
  this->ExtrusionFilter2->SetScaleFactor(smax);

  this->ClipWithBox->SetInputData(input);
  this->ClipWithBox->Update();
  this->ExtrusionFilter1->SetInputData(input);

  double normal[3];
  this->ResliceCursor->GetPlane(this->ReslicePlaneNormal)->GetNormal(normal);
  this->ExtrusionFilter1->SetVector(normal);
  this->ExtrusionFilter2->SetVector(-normal[0], -normal[1], -normal[2]);
  // std::cout << normal[0] << " " << normal[1] << " " << normal[2] << std::endl;

  this->ExtrusionFilter2->Update();

  output->DeepCopy(this->ExtrusionFilter2->GetOutput());
}

//-------------------------------------------------------------------------
svtkMTimeType svtkResliceCursorPolyDataAlgorithm::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->ResliceCursor)
  {
    svtkMTimeType time;
    time = this->ResliceCursor->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }
  return mTime;
}

//---------------------------------------------------------------------------
void svtkResliceCursorPolyDataAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ResliceCursor: " << this->ResliceCursor << "\n";
  if (this->ResliceCursor)
  {
    this->ResliceCursor->PrintSelf(os, indent);
  }
  os << indent << "Cutter: " << this->Cutter << "\n";
  if (this->Cutter)
  {
    this->Cutter->PrintSelf(os, indent);
  }
  os << indent << "ExtrusionFilter1: " << this->ExtrusionFilter1 << "\n";
  if (this->ExtrusionFilter1)
  {
    this->ExtrusionFilter1->PrintSelf(os, indent);
  }
  os << indent << "ExtrusionFilter2: " << this->ExtrusionFilter2 << "\n";
  if (this->ExtrusionFilter2)
  {
    this->ExtrusionFilter2->PrintSelf(os, indent);
  }
  os << indent << "ReslicePlaneNormal: " << this->ReslicePlaneNormal << endl;
  os << indent << "Extrude: " << this->Extrude << endl;

  // this->SliceBounds;
  // this->ThickAxes[2];
  // this->Box;
  // this->ClipWithBox;
  // this->SlicePlane;
}
