/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLogoRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLogoRepresentation.h"
#include "svtkCallbackCommand.h"
#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkPropCollection.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"
#include "svtkTexturedActor2D.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkLogoRepresentation);

svtkCxxSetObjectMacro(svtkLogoRepresentation, Image, svtkImageData);
svtkCxxSetObjectMacro(svtkLogoRepresentation, ImageProperty, svtkProperty2D);

//-------------------------------------------------------------------------
svtkLogoRepresentation::svtkLogoRepresentation()
{
  // Initialize the data members
  this->Image = nullptr;
  this->ImageProperty = svtkProperty2D::New();

  // Setup the pipeline
  this->Texture = svtkTexture::New();
  this->TexturePolyData = svtkPolyData::New();
  this->TexturePoints = svtkPoints::New();
  this->TexturePoints->SetNumberOfPoints(4);
  this->TexturePolyData->SetPoints(this->TexturePoints);
  svtkCellArray* polys = svtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->TexturePolyData->SetPolys(polys);
  polys->Delete();
  svtkFloatArray* tc = svtkFloatArray::New();
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->InsertComponent(0, 0, 0.0);
  tc->InsertComponent(0, 1, 0.0);
  tc->InsertComponent(1, 0, 1.0);
  tc->InsertComponent(1, 1, 0.0);
  tc->InsertComponent(2, 0, 1.0);
  tc->InsertComponent(2, 1, 1.0);
  tc->InsertComponent(3, 0, 0.0);
  tc->InsertComponent(3, 1, 1.0);
  this->TexturePolyData->GetPointData()->SetTCoords(tc);
  tc->Delete();
  this->TextureMapper = svtkPolyDataMapper2D::New();
  this->TextureMapper->SetInputData(this->TexturePolyData);
  this->TextureActor = svtkTexturedActor2D::New();
  this->TextureActor->SetMapper(this->TextureMapper);
  this->TextureActor->SetTexture(this->Texture);
  this->ImageProperty->SetOpacity(0.25);
  this->TextureActor->SetProperty(this->ImageProperty);

  // Set up parameters from the superclass
  double size[2];
  this->GetSize(size);
  this->Position2Coordinate->SetValue(0.04 * size[0], 0.04 * size[1]);
  this->ProportionalResize = 1;
  this->Moving = 1;
  this->SetShowBorder(svtkBorderRepresentation::BORDER_ACTIVE);
  this->PositionCoordinate->SetValue(0.9, 0.025);
  this->Position2Coordinate->SetValue(0.075, 0.075);
}

//-------------------------------------------------------------------------
svtkLogoRepresentation::~svtkLogoRepresentation()
{
  if (this->Image)
  {
    this->Image->Delete();
  }
  this->ImageProperty->Delete();
  this->Texture->Delete();
  this->TexturePoints->Delete();
  this->TexturePolyData->Delete();
  this->TextureMapper->Delete();
  this->TextureActor->Delete();
}

//----------------------------------------------------------------------
void svtkLogoRepresentation::AdjustImageSize(double o[2], double borderSize[2], double imageSize[2])
{
  // Scale the image to fit with in the border.
  // Also update the origin so the image is centered.
  double r0 = borderSize[0] / imageSize[0];
  double r1 = borderSize[1] / imageSize[1];
  if (r0 > r1)
  {
    imageSize[0] *= r1;
    imageSize[1] *= r1;
  }
  else
  {
    imageSize[0] *= r0;
    imageSize[1] *= r0;
  }

  if (imageSize[0] < borderSize[0])
  {
    o[0] += (borderSize[0] - imageSize[0]) / 2.0;
  }
  if (imageSize[1] < borderSize[1])
  {
    o[1] += (borderSize[1] - imageSize[1]) / 2.0;
  }
}

//-------------------------------------------------------------------------
void svtkLogoRepresentation::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {

    // Determine and adjust the size of the image
    if (this->Image)
    {
      double imageSize[2], borderSize[2], o[2];
      imageSize[0] = 0.0;
      imageSize[1] = 0.0;
      // this->Image->Update();
      if (this->Image->GetDataDimension() == 2)
      {
        int dims[3];
        this->Image->GetDimensions(dims);
        imageSize[0] = static_cast<double>(dims[0]);
        imageSize[1] = static_cast<double>(dims[1]);
      }
      int* p1 = this->PositionCoordinate->GetComputedDisplayValue(this->Renderer);
      int* p2 = this->Position2Coordinate->GetComputedDisplayValue(this->Renderer);
      borderSize[0] = p2[0] - p1[0];
      borderSize[1] = p2[1] - p1[1];
      o[0] = static_cast<double>(p1[0]);
      o[1] = static_cast<double>(p1[1]);

      // this preserves the image aspect ratio. The image is
      // centered around the center of the bordered ragion.
      this->AdjustImageSize(o, borderSize, imageSize);

      // Update the points
      this->Texture->SetInputData(this->Image);
      this->Texture->InterpolateOn();
      this->TexturePoints->SetPoint(0, o[0], o[1], 0.0);
      this->TexturePoints->SetPoint(1, o[0] + imageSize[0], o[1], 0.0);
      this->TexturePoints->SetPoint(2, o[0] + imageSize[0], o[1] + imageSize[1], 0.0);
      this->TexturePoints->SetPoint(3, o[0], o[1] + imageSize[1], 0.0);
      // For GL backend 2 it is important to modify the point array
      this->TexturePoints->Modified();
    }
  }

  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//-------------------------------------------------------------------------
void svtkLogoRepresentation::GetActors2D(svtkPropCollection* pc)
{
  pc->AddItem(this->TextureActor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void svtkLogoRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->TextureActor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int svtkLogoRepresentation::RenderOverlay(svtkViewport* v)
{
  int count = 0;
  if (this->TextureActor->GetVisibility())
  {
    svtkRenderer* ren = svtkRenderer::SafeDownCast(v);
    if (ren)
    {
      count += this->TextureActor->RenderOverlay(v);
    }
    // Display border on top of logo
    count += this->Superclass::RenderOverlay(v);
  }
  return count;
}

//-------------------------------------------------------------------------
void svtkLogoRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Image)
  {
    os << indent << "Image:\n";
    this->Image->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Image: (none)\n";
  }

  if (this->ImageProperty)
  {
    os << indent << "Image Property:\n";
    this->ImageProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Image Property: (none)\n";
  }
}
