/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResliceCursorRepresentation.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkHandleRepresentation.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapToWindowLevelColors.h"
#include "svtkImageMapper3D.h"
#include "svtkImageReslice.h"
#include "svtkInteractorObserver.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPlaneSource.h"
#include "svtkPointHandleRepresentation2D.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkResliceCursor.h"
#include "svtkResliceCursorPolyDataAlgorithm.h"
#include "svtkScalarsToColors.h"
#include "svtkTextActor.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkTexture.h"
#include "svtkWindow.h"

#include <sstream>

svtkCxxSetObjectMacro(svtkResliceCursorRepresentation, ColorMap, svtkImageMapToColors);

//----------------------------------------------------------------------
svtkResliceCursorRepresentation::svtkResliceCursorRepresentation()
{
  this->ManipulationMode = None;
  this->Modifier = 0;
  this->Tolerance = 5;
  this->ShowReslicedImage = 1;
  this->RestrictPlaneToVolume = 1;
  this->OriginalWindow = 1.0;
  this->OriginalLevel = 0.5;
  this->InitialWindow = 1.0;
  this->InitialLevel = 0.5;
  this->CurrentWindow = 1.0;
  this->CurrentLevel = 0.5;

  this->ThicknessTextProperty = svtkTextProperty::New();
  this->ThicknessTextProperty->SetBold(1);
  this->ThicknessTextProperty->SetItalic(1);
  this->ThicknessTextProperty->SetShadow(1);
  this->ThicknessTextProperty->SetFontFamilyToArial();
  this->ThicknessTextMapper = svtkTextMapper::New();
  this->ThicknessTextMapper->SetTextProperty(this->ThicknessTextProperty);
  this->ThicknessTextMapper->SetInput("0.0");
  this->ThicknessTextActor = svtkActor2D::New();
  this->ThicknessTextActor->SetMapper(this->ThicknessTextMapper);
  this->ThicknessTextActor->VisibilityOff();

  this->Reslice = nullptr;
  this->CreateDefaultResliceAlgorithm();
  this->PlaneSource = svtkPlaneSource::New();

  this->ThicknessLabelFormat = new char[6];
  snprintf(this->ThicknessLabelFormat, 6, "%s", "%0.3g");

  this->ResliceAxes = svtkMatrix4x4::New();
  this->NewResliceAxes = svtkMatrix4x4::New();
  this->LookupTable = nullptr;
  this->ColorMap = svtkImageMapToColors::New();
  this->Texture = svtkTexture::New();
  this->Texture->SetInputConnection(this->ColorMap->GetOutputPort());
  this->Texture->SetInterpolate(1);
  this->TexturePlaneActor = svtkActor::New();

  this->LookupTable = this->CreateDefaultLookupTable();

  this->ColorMap->SetLookupTable(this->LookupTable);
  this->ColorMap->SetOutputFormatToRGBA();
  this->ColorMap->PassAlphaToOutputOn();

  svtkPolyDataMapper* texturePlaneMapper = svtkPolyDataMapper::New();
  texturePlaneMapper->SetInputConnection(this->PlaneSource->GetOutputPort());
  texturePlaneMapper->SetResolveCoincidentTopologyToPolygonOffset();

  this->Texture->SetQualityTo32Bit();
  this->Texture->SetColorMode(SVTK_COLOR_MODE_DEFAULT);
  this->Texture->SetInterpolate(1);
  this->Texture->RepeatOff();
  this->Texture->SetLookupTable(this->LookupTable);

  this->TexturePlaneActor->SetMapper(texturePlaneMapper);
  this->TexturePlaneActor->SetTexture(this->Texture);
  this->TexturePlaneActor->PickableOn();
  texturePlaneMapper->Delete();

  this->UseImageActor = false;
  this->ImageActor = svtkImageActor::New();
  this->ImageActor->GetMapper()->SetInputConnection(this->ColorMap->GetOutputPort());

  // Represent the text: annotation for cursor position and W/L

  this->DisplayText = 1;
  this->TextActor = svtkTextActor::New();
  this->GenerateText();
}

//----------------------------------------------------------------------
svtkResliceCursorRepresentation::~svtkResliceCursorRepresentation()
{
  this->ThicknessTextProperty->Delete();
  this->ThicknessTextMapper->Delete();
  this->ThicknessTextActor->Delete();
  this->SetThicknessLabelFormat(nullptr);
  this->ImageActor->Delete();
  if (this->Reslice)
  {
    this->Reslice->Delete();
  }
  this->PlaneSource->Delete();
  this->ResliceAxes->Delete();
  this->NewResliceAxes->Delete();
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  this->ColorMap->Delete();
  this->Texture->Delete();
  this->TexturePlaneActor->Delete();
  this->TextActor->Delete();
}

//----------------------------------------------------------------------
void svtkResliceCursorRepresentation::SetLookupTable(svtkScalarsToColors* l)
{
  svtkSetObjectBodyMacro(LookupTable, svtkScalarsToColors, l);
  this->LookupTable = l;
  if (this->ColorMap)
  {
    this->ColorMap->SetLookupTable(this->LookupTable);
  }
}

//----------------------------------------------------------------------
char* svtkResliceCursorRepresentation::GetThicknessLabelText()
{
  return this->ThicknessTextMapper->GetInput();
}

//----------------------------------------------------------------------
double* svtkResliceCursorRepresentation::GetThicknessLabelPosition()
{
  return this->ThicknessTextActor->GetPosition();
}

//----------------------------------------------------------------------
void svtkResliceCursorRepresentation::GetThicknessLabelPosition(double pos[3])
{
  this->ThicknessTextActor->GetPositionCoordinate()->GetValue(pos);
}

//----------------------------------------------------------------------
void svtkResliceCursorRepresentation::GetWorldThicknessLabelPosition(double pos[3])
{
  double viewportPos[3], worldPos[4];
  pos[0] = pos[1] = pos[2] = 0.0;
  if (!this->Renderer)
  {
    svtkErrorMacro("GetWorldLabelPosition: no renderer!");
    return;
  }

  this->ThicknessTextActor->GetPositionCoordinate()->GetValue(viewportPos);
  this->Renderer->ViewportToNormalizedViewport(viewportPos[0], viewportPos[1]);
  this->Renderer->NormalizedViewportToView(viewportPos[0], viewportPos[1], viewportPos[2]);
  this->Renderer->SetViewPoint(viewportPos);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(worldPos);

  if (worldPos[3] != 0.0)
  {
    pos[0] = worldPos[0] / worldPos[3];
    pos[1] = worldPos[1] / worldPos[3];
    pos[2] = worldPos[2] / worldPos[3];
  }
  else
  {
    svtkErrorMacro("GetWorldLabelPosition: world position at index 3 is 0, not dividing by 0");
  }
}

//----------------------------------------------------------------------
void svtkResliceCursorRepresentation::SetManipulationMode(int m)
{
  this->ManipulationMode = m;
}

//----------------------------------------------------------------------
void svtkResliceCursorRepresentation::BuildRepresentation()
{
  this->Reslice->SetInputData(this->GetResliceCursor()->GetImage());

  this->TexturePlaneActor->SetVisibility(
    this->GetResliceCursor()->GetImage() ? (this->ShowReslicedImage && !this->UseImageActor) : 0);
  this->ImageActor->SetVisibility(
    this->GetResliceCursor()->GetImage() ? (this->ShowReslicedImage && this->UseImageActor) : 0);

  // Update the reslice plane if the plane is being manipulated
  if (this->GetManipulationMode() != WindowLevelling)
  {
    this->UpdateReslicePlane();
  }

  this->ImageActor->SetDisplayExtent(this->ColorMap->GetOutput()->GetExtent());

  // Update any text annotations
  this->ManageTextDisplay();
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::InitializeReslicePlane()
{
  if (!this->GetResliceCursor()->GetImage())
  {
    return;
  }

  // this->GetResliceCursor()->GetImage()->UpdateInformation();

  // Initialize the reslice plane origins. Offset should be zero within
  // this function here.

  this->ComputeReslicePlaneOrigin();

  // Finally reset the camera to whatever orientation they were staring in

  this->ResetCamera();
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::ResetCamera()
{

  // Reset the camera back to the default and the focal point to where
  // the cursor center is

  if (this->Renderer)
  {
    double center[3], camPos[3], n[3];
    this->GetResliceCursor()->GetCenter(center);
    this->Renderer->GetActiveCamera()->SetFocalPoint(center);

    const int normalAxis = this->GetCursorAlgorithm()->GetReslicePlaneNormal();
    this->GetResliceCursor()->GetPlane(normalAxis)->GetNormal(n);
    svtkMath::Add(center, n, camPos);
    this->Renderer->GetActiveCamera()->SetPosition(camPos);

    // Reset the camera in response to changes.
    this->Renderer->ResetCamera();
    this->Renderer->ResetCameraClippingRange();
  }
}

//----------------------------------------------------------------------------
// This is the first axis of the reslice on the currently resliced plane
//
void svtkResliceCursorRepresentation::GetVector1(double v1[3])
{
  // From the initial view up vector, compute its cross product with the
  // current plane normal. This is Vector1. Then Vector2 is the cross product
  // of Vector1 and the normal.

  double v2[3];
  double* p2 = this->PlaneSource->GetPoint2();
  double* o = this->PlaneSource->GetOrigin();

  // Vector p2 -> o
  svtkMath::Subtract(p2, o, v2);

  const int planeOrientation = this->GetCursorAlgorithm()->GetReslicePlaneNormal();
  svtkPlane* plane = this->GetResliceCursor()->GetPlane(planeOrientation);

  double planeNormal[3];
  plane->GetNormal(planeNormal);

  svtkMath::Cross(v2, planeNormal, v1);
  svtkMath::Normalize(v1);
}

//----------------------------------------------------------------------------
// This is the second axis of the reslice on the currently resliced plane
// It is orthogonal to v1 and to the plane normal. Note that this is not the
// same as the reslice cursor's axes, which need not be orthogonal to each
// other. The goal of vector1 and vector2 is to compute the X and Y axes of
// the resliced plane.
//
void svtkResliceCursorRepresentation::GetVector2(double v2[3])
{
  const int planeOrientation = this->GetCursorAlgorithm()->GetReslicePlaneNormal();
  svtkPlane* plane = this->GetResliceCursor()->GetPlane(planeOrientation);
  double planeNormal[3];
  plane->GetNormal(planeNormal);

  double v1[3];
  this->GetVector1(v1);

  svtkMath::Cross(planeNormal, v1, v2);
  svtkMath::Normalize(v2);
}

//----------------------------------------------------------------------
// Compute the origin of the reslice plane prior to transformations.
//
void svtkResliceCursorRepresentation::ComputeReslicePlaneOrigin()
{
  double bounds[6];
  this->GetResliceCursor()->GetImage()->GetBounds(bounds);

  double center[3], imageCenter[3], offset[3];
  this->GetResliceCursor()->GetCenter(center);
  this->GetResliceCursor()->GetImage()->GetCenter(imageCenter);

  // Offset based on the center of the image and how far from it the
  // reslice cursor is. This allows us to capture the whole image even
  // if we resliced in awkward places.

  for (int i = 0; i < 3; i++)
  {
    offset[i] = -fabs(center[i] - imageCenter[i]);
  }

  // Now resize the plane based on these offsets.

  const int planeOrientation = this->GetCursorAlgorithm()->GetReslicePlaneNormal();

  // Now set the size of the plane based on the location of the cursor so as to
  // at least completely cover the viewed region

  if (planeOrientation == 1)
  {
    this->PlaneSource->SetOrigin(bounds[0] + offset[0], center[1], bounds[4] + offset[2]);
    this->PlaneSource->SetPoint1(bounds[1] - offset[0], center[1], bounds[4] + offset[2]);
    this->PlaneSource->SetPoint2(bounds[0] + offset[0], center[1], bounds[5] - offset[2]);
  }
  else if (planeOrientation == 2)
  {
    this->PlaneSource->SetOrigin(bounds[0] + offset[0], bounds[2] + offset[1], center[2]);
    this->PlaneSource->SetPoint1(bounds[1] - offset[0], bounds[2] + offset[1], center[2]);
    this->PlaneSource->SetPoint2(bounds[0] + offset[0], bounds[3] - offset[1], center[2]);
  }
  else if (planeOrientation == 0)
  {
    this->PlaneSource->SetOrigin(center[0], bounds[2] + offset[1], bounds[4] + offset[2]);
    this->PlaneSource->SetPoint1(center[0], bounds[3] - offset[1], bounds[4] + offset[2]);
    this->PlaneSource->SetPoint2(center[0], bounds[2] + offset[1], bounds[5] - offset[2]);
  }
}

//----------------------------------------------------------------------
void svtkResliceCursorRepresentation::UpdateReslicePlane()
{
  if (!this->GetResliceCursor()->GetImage() || !this->TexturePlaneActor->GetVisibility())
  {
    return;
  }

  // Reinitialize the reslice plane.. We will recompute everything here.
  if (this->PlaneSource->GetPoint1()[0] == 0.5 && this->PlaneSource->GetOrigin()[0] == -0.5)
  {
    this->InitializeReslicePlane();
  }

  // Calculate appropriate pixel spacing for the reslicing
  //
  // this->GetResliceCursor()->UpdateInformation();
  double spacing[3];
  this->GetResliceCursor()->GetImage()->GetSpacing(spacing);
  double origin[3];
  this->GetResliceCursor()->GetImage()->GetOrigin(origin);
  int extent[6];
  this->GetResliceCursor()->GetImage()->GetExtent(extent);

  for (int i = 0; i < 3; i++)
  {
    if (extent[2 * i] > extent[2 * i + 1])
    {
      svtkErrorMacro("Invalid extent [" << extent[0] << ", " << extent[1] << ", " << extent[2]
                                       << ", " << extent[3] << ", " << extent[4] << ", "
                                       << extent[5] << "]."
                                       << " Perhaps the input data is empty?");
      break;
    }
  }

  const int planeOrientation = this->GetCursorAlgorithm()->GetReslicePlaneNormal();
  svtkPlane* plane = this->GetResliceCursor()->GetPlane(planeOrientation);
  double planeNormal[3];
  plane->GetNormal(planeNormal);

  // Compute the origin of the reslice plane prior to transformations.

  this->ComputeReslicePlaneOrigin();

  this->PlaneSource->SetNormal(planeNormal);
  this->PlaneSource->SetCenter(plane->GetOrigin());

  double planeAxis1[3];
  double planeAxis2[3];

  double* p1 = this->PlaneSource->GetPoint1();
  double* o = this->PlaneSource->GetOrigin();
  svtkMath::Subtract(p1, o, planeAxis1);
  double* p2 = this->PlaneSource->GetPoint2();
  svtkMath::Subtract(p2, o, planeAxis2);

  // The x,y dimensions of the plane
  //
  const double planeSizeX = svtkMath::Normalize(planeAxis1);
  const double planeSizeY = svtkMath::Normalize(planeAxis2);

  double normal[3];
  this->PlaneSource->GetNormal(normal);

  this->NewResliceAxes->Identity();
  for (int i = 0; i < 3; i++)
  {
    this->NewResliceAxes->SetElement(0, i, planeAxis1[i]);
    this->NewResliceAxes->SetElement(1, i, planeAxis2[i]);
    this->NewResliceAxes->SetElement(2, i, normal[i]);
  }

  const double spacingX = fabs(planeAxis1[0] * spacing[0]) + fabs(planeAxis1[1] * spacing[1]) +
    fabs(planeAxis1[2] * spacing[2]);

  const double spacingY = fabs(planeAxis2[0] * spacing[0]) + fabs(planeAxis2[1] * spacing[1]) +
    fabs(planeAxis2[2] * spacing[2]);

  double planeOrigin[4];
  this->PlaneSource->GetOrigin(planeOrigin);

  planeOrigin[3] = 1.0;
  double originXYZW[4];
  double neworiginXYZW[4];

  this->NewResliceAxes->MultiplyPoint(planeOrigin, originXYZW);
  this->NewResliceAxes->Transpose();
  this->NewResliceAxes->MultiplyPoint(originXYZW, neworiginXYZW);

  this->NewResliceAxes->SetElement(0, 3, neworiginXYZW[0]);
  this->NewResliceAxes->SetElement(1, 3, neworiginXYZW[1]);
  this->NewResliceAxes->SetElement(2, 3, neworiginXYZW[2]);

  // Compute a new set of resliced extents
  int extentX, extentY;

  // Pad extent up to a power of two for efficient texture mapping

  // make sure we're working with valid values
  double realExtentX = (spacingX == 0) ? SVTK_INT_MAX : planeSizeX / spacingX;

  // Sanity check the input data:
  // * if realExtentX is too large, extentX will wrap
  // * if spacingX is 0, things will blow up.
  if (realExtentX > (SVTK_INT_MAX >> 1))
  {
    svtkErrorMacro(<< "Invalid X extent: " << realExtentX);
    extentX = 0;
  }
  else
  {
    extentX = 1;
    while (extentX < realExtentX)
    {
      extentX = extentX << 1;
    }
  }

  // make sure extentY doesn't wrap during padding
  double realExtentY = (spacingY == 0) ? SVTK_INT_MAX : planeSizeY / spacingY;

  if (realExtentY > (SVTK_INT_MAX >> 1))
  {
    svtkErrorMacro(<< "Invalid Y extent: " << realExtentY);
    extentY = 0;
  }
  else
  {
    extentY = 1;
    while (extentY < realExtentY)
    {
      extentY = extentY << 1;
    }
  }

  double outputSpacingX = (extentX == 0) ? 1.0 : planeSizeX / extentX;
  double outputSpacingY = (extentY == 0) ? 1.0 : planeSizeY / extentY;

  bool modify = false;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      double d = this->NewResliceAxes->GetElement(i, j);
      if (d != this->ResliceAxes->GetElement(i, j))
      {
        this->ResliceAxes->SetElement(i, j, d);
        modify = true;
      }
    }
  }

  if (modify)
  {
    this->ResliceAxes->Modified();
  }

  this->SetResliceParameters(outputSpacingX, outputSpacingY, extentX, extentY);
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::ComputeOrigin(svtkMatrix4x4* m)
{
  double center[4] = { 0, 0, 0, 1 };
  double centerTransformed[4];

  this->GetResliceCursor()->GetCenter(center);
  m->MultiplyPoint(center, centerTransformed);

  for (int i = 0; i < 3; i++)
  {
    m->SetElement(i, 3, m->GetElement(i, 3) + center[i] - centerTransformed[i]);
  }
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation ::SetResliceParameters(
  double outputSpacingX, double outputSpacingY, int extentX, int extentY)
{
  svtkImageReslice* reslice = svtkImageReslice::SafeDownCast(this->Reslice);

  if (reslice)
  {
    // Set the default color the minimum scalar value
    double range[2];
    svtkImageData::SafeDownCast(reslice->GetInput())->GetScalarRange(range);
    reslice->SetBackgroundLevel(range[0]);

    this->ColorMap->SetInputConnection(reslice->GetOutputPort());
    reslice->TransformInputSamplingOff();
    reslice->AutoCropOutputOn();
    reslice->SetResliceAxes(this->ResliceAxes);
    reslice->SetOutputSpacing(outputSpacingX, outputSpacingY, 1);
    reslice->SetOutputOrigin(0.5 * outputSpacingX, 0.5 * outputSpacingY, 0);
    reslice->SetOutputExtent(0, extentX - 1, 0, extentY - 1, 0, 0);
  }
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation ::SetWindowLevel(double window, double level, int copy)
{
  if (copy)
  {
    this->CurrentWindow = window;
    this->CurrentLevel = level;
    return;
  }

  if (this->CurrentWindow == window && this->CurrentLevel == level)
  {
    return;
  }

  // if the new window is negative and the old window was positive invert table
  if (((window < 0 && this->CurrentWindow > 0) || (window > 0 && this->CurrentWindow < 0)))
  {
    this->InvertTable();
  }

  this->CurrentWindow = window;
  this->CurrentLevel = level;

  double rmin = this->CurrentLevel - 0.5 * fabs(this->CurrentWindow);
  double rmax = rmin + fabs(this->CurrentWindow);
  this->LookupTable->SetRange(rmin, rmax);

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::GetWindowLevel(double wl[2])
{
  wl[0] = this->CurrentWindow;
  wl[1] = this->CurrentLevel;
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::WindowLevel(double X, double Y)
{
  if (!this->Renderer)
  {
    return;
  }

  const int* size = this->Renderer->GetSize();
  double window = this->InitialWindow;
  double level = this->InitialLevel;

  // Compute normalized delta

  double dx = 2.0 * (X - this->StartEventPosition[0]) / size[0];
  double dy = 2.0 * (this->StartEventPosition[1] - Y) / size[1];

  // Scale by current values

  if (fabs(window) > 0.01)
  {
    dx = dx * window;
  }
  else
  {
    dx = dx * (window < 0 ? -0.01 : 0.01);
  }
  if (fabs(level) > 0.01)
  {
    dy = dy * level;
  }
  else
  {
    dy = dy * (level < 0 ? -0.01 : 0.01);
  }

  // Abs so that direction does not flip

  if (window < 0.0)
  {
    dx = -1 * dx;
  }
  if (level < 0.0)
  {
    dy = -1 * dy;
  }

  // Compute new window level

  double newWindow = dx + window;
  double newLevel = level - dy;

  if (fabs(newWindow) < 0.01)
  {
    newWindow = 0.01 * (newWindow < 0 ? -1 : 1);
  }
  if (fabs(newLevel) < 0.01)
  {
    newLevel = 0.01 * (newLevel < 0 ? -1 : 1);
  }

  if ((newWindow < 0 && this->CurrentWindow > 0) || (newWindow > 0 && this->CurrentWindow < 0))
  {
    this->InvertTable();
  }

  double rmin = newLevel - 0.5 * fabs(newWindow);
  double rmax = rmin + fabs(newWindow);
  this->LookupTable->SetRange(rmin, rmax);

  if (this->DisplayText && (this->CurrentWindow != newWindow || this->CurrentLevel != newLevel))
  {
    this->CurrentWindow = newWindow;
    this->CurrentLevel = newLevel;
  }
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::InvertTable()
{
  svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->LookupTable);

  if (lut)
  {
    int index = lut->GetNumberOfTableValues();
    unsigned char swap[4];
    size_t num = 4 * sizeof(unsigned char);
    svtkUnsignedCharArray* table = lut->GetTable();
    for (int count = 0; count < --index; count++)
    {
      unsigned char* rgba1 = table->GetPointer(4 * count);
      unsigned char* rgba2 = table->GetPointer(4 * index);
      memcpy(swap, rgba1, num);
      memcpy(rgba1, rgba2, num);
      memcpy(rgba2, swap, num);
    }

    // force the lookuptable to update its InsertTime to avoid
    // rebuilding the array
    double temp[4];
    lut->GetTableValue(0, temp);
    lut->SetTableValue(0, temp);
  }
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::CreateDefaultResliceAlgorithm()
{
  // Allows users to optionally use their own reslice filters or other
  // algorithms here.
  if (!this->Reslice)
  {
    this->Reslice = svtkImageReslice::New();
  }
}

//----------------------------------------------------------------------------
svtkScalarsToColors* svtkResliceCursorRepresentation::CreateDefaultLookupTable()
{
  svtkLookupTable* lut = svtkLookupTable::New();
  lut->Register(this);
  lut->Delete();
  lut->SetNumberOfColors(256);
  lut->SetHueRange(0, 0);
  lut->SetSaturationRange(0, 0);
  lut->SetValueRange(0, 1);
  lut->SetAlphaRange(1, 1);
  lut->Build();
  return lut;
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::ActivateText(int i)
{
  this->TextActor->SetVisibility(this->Renderer && this->GetVisibility() && i && this->DisplayText);
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::ManageTextDisplay()
{
  if (!this->DisplayText)
  {
    return;
  }

  if (this->ManipulationMode == svtkResliceCursorRepresentation::WindowLevelling)
  {
    snprintf(this->TextBuff, SVTK_RESLICE_CURSOR_REPRESENTATION_MAX_TEXTBUFF,
      "Window, Level: ( %g, %g )", this->CurrentWindow, this->CurrentLevel);
  }
  else if (this->ManipulationMode == svtkResliceCursorRepresentation::ResizeThickness)
  {
    // For now all the thickness' are the same anyway.
    snprintf(this->TextBuff, SVTK_RESLICE_CURSOR_REPRESENTATION_MAX_TEXTBUFF,
      "Reslice Thickness: %g mm", this->GetResliceCursor()->GetThickness()[0]);
  }

  this->TextActor->SetInput(this->TextBuff);
  this->TextActor->Modified();
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::SetTextProperty(svtkTextProperty* tprop)
{
  this->TextActor->SetTextProperty(tprop);
}

//----------------------------------------------------------------------------
svtkTextProperty* svtkResliceCursorRepresentation::GetTextProperty()
{
  return this->TextActor->GetTextProperty();
}

//----------------------------------------------------------------------------
void svtkResliceCursorRepresentation::GenerateText()
{
  snprintf(this->TextBuff, SVTK_RESLICE_CURSOR_REPRESENTATION_MAX_TEXTBUFF, "NA");
  this->TextActor->SetInput(this->TextBuff);
  this->TextActor->SetTextScaleModeToNone();

  svtkTextProperty* textprop = this->TextActor->GetTextProperty();
  textprop->SetColor(1, 1, 1);
  textprop->SetFontFamilyToArial();
  textprop->SetFontSize(18);
  textprop->BoldOff();
  textprop->ItalicOff();
  textprop->ShadowOff();
  textprop->SetJustificationToLeft();
  textprop->SetVerticalJustificationToBottom();

  svtkCoordinate* coord = this->TextActor->GetPositionCoordinate();
  coord->SetCoordinateSystemToNormalizedViewport();
  coord->SetValue(.01, .01);

  this->TextActor->VisibilityOff();
}

//----------------------------------------------------------------------
// Prints an object if it exists.
#define svtkPrintMemberObjectMacro(obj, os, indent)                                                 \
  os << indent << #obj << ": ";                                                                    \
  if (this->obj)                                                                                   \
  {                                                                                                \
    os << this->obj << "\n";                                                                       \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    os << "(null)\n";                                                                              \
  }

//----------------------------------------------------------------------
void svtkResliceCursorRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Thickness Label Text: " << this->GetThicknessLabelText() << "\n";
  os << indent << "PlaneSource: " << this->PlaneSource << "\n";
  if (this->PlaneSource)
  {
    this->PlaneSource->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "PlaneSource: " << this->PlaneSource << "\n";
  svtkPrintMemberObjectMacro(ThicknessLabelFormat, os, indent);
  svtkPrintMemberObjectMacro(Reslice, os, indent);
  svtkPrintMemberObjectMacro(ThicknessTextProperty, os, indent);
  svtkPrintMemberObjectMacro(ThicknessTextMapper, os, indent);
  svtkPrintMemberObjectMacro(ThicknessTextActor, os, indent);
  svtkPrintMemberObjectMacro(ResliceAxes, os, indent);
  svtkPrintMemberObjectMacro(NewResliceAxes, os, indent);
  svtkPrintMemberObjectMacro(ColorMap, os, indent);
  svtkPrintMemberObjectMacro(TexturePlaneActor, os, indent);
  svtkPrintMemberObjectMacro(Texture, os, indent);
  svtkPrintMemberObjectMacro(LookupTable, os, indent);
  svtkPrintMemberObjectMacro(ImageActor, os, indent);
  svtkPrintMemberObjectMacro(TextActor, os, indent);
  os << indent << "RestrictPlaneToVolume: " << this->RestrictPlaneToVolume << "\n";
  os << indent << "ShowReslicedImage: " << this->ShowReslicedImage << "\n";
  os << indent << "OriginalWindow: " << this->OriginalWindow << "\n";
  os << indent << "OriginalLevel: " << this->OriginalLevel << "\n";
  os << indent << "CurrentWindow: " << this->CurrentWindow << "\n";
  os << indent << "CurrentLevel: " << this->CurrentLevel << "\n";
  os << indent << "InitialWindow: " << this->InitialWindow << "\n";
  os << indent << "InitialLevel: " << this->InitialLevel << "\n";
  os << indent << "UseImageActor: " << this->UseImageActor << "\n";
  os << indent << "DisplayText: " << this->DisplayText << "\n";
  // this->ManipulationMode
  // this->LastEventPosition[2]
  // this->TextBuff[128];
  // this->ResliceAxes;
  // this->LookupTable;
  // this->Reslice;
  // this->ThicknessLabelFormat;
  // this->ColorMap;
  // this->ImageActor;
  // this->ThicknessTextProperty;
  // this->ThicknessTextMapper;
  // this->ThicknessTextActor;
  // this->NewResliceAxes;
  // this->TexturePlaneActor;
  // this->Texture;
  // this->TextActor;
}
