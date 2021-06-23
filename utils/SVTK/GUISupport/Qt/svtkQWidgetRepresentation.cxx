/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQWidgetRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkQWidgetRepresentation.h"

#include "svtkActor.h"
#include "svtkCellPicker.h"
#include "svtkEventData.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLTexture.h"
#include "svtkPickingManager.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkQWidgetTexture.h"
#include "svtkRenderer.h"
#include "svtkVectorOperators.h"
#include <QtWidgets/QWidget>

#include "svtk_glew.h"

svtkStandardNewMacro(svtkQWidgetRepresentation);

//----------------------------------------------------------------------------
svtkQWidgetRepresentation::svtkQWidgetRepresentation()
{
  this->PlaneSource = svtkPlaneSource::New();
  this->PlaneSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  this->PlaneMapper = svtkPolyDataMapper::New();
  this->PlaneMapper->SetInputConnection(this->PlaneSource->GetOutputPort());

  this->QWidgetTexture = svtkQWidgetTexture::New();
  this->PlaneTexture = svtkOpenGLTexture::New();
  this->PlaneTexture->SetTextureObject(this->QWidgetTexture);

  this->PlaneActor = svtkActor::New();
  this->PlaneActor->SetMapper(this->PlaneMapper);
  this->PlaneActor->SetTexture(this->PlaneTexture);
  this->PlaneActor->GetProperty()->SetAmbient(1.0);
  this->PlaneActor->GetProperty()->SetDiffuse(0.0);

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  this->Picker = svtkCellPicker::New();
  this->Picker->SetTolerance(0.005);
  this->Picker->AddPickList(this->PlaneActor);
  this->Picker->PickFromListOn();
}

//----------------------------------------------------------------------------
svtkQWidgetRepresentation::~svtkQWidgetRepresentation()
{
  this->PlaneSource->Delete();
  this->PlaneMapper->Delete();
  this->PlaneActor->Delete();
  this->PlaneTexture->Delete();
  this->QWidgetTexture->Delete();

  this->Picker->Delete();
}

void svtkQWidgetRepresentation::SetWidget(QWidget* w)
{
  // just pass down to the QWidgetTexture
  this->QWidgetTexture->SetWidget(w);
  this->Modified();
}

// see if the event hits the widget rep, if so set the WidgetCoordinates
// and move to Inside state
int svtkQWidgetRepresentation::ComputeComplexInteractionState(
  svtkRenderWindowInteractor*, svtkAbstractWidget*, unsigned long, void* calldata, int)
{
  svtkEventData* edata = static_cast<svtkEventData*>(calldata);
  svtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    // compute intersection point using math, faster better
    svtkVector3d origin;
    this->PlaneSource->GetOrigin(origin.GetData());
    svtkVector3d axis0;
    this->PlaneSource->GetPoint1(axis0.GetData());
    svtkVector3d axis1;
    this->PlaneSource->GetPoint2(axis1.GetData());

    axis0 = axis0 - origin;
    axis1 = axis1 - origin;

    svtkVector3d rpos;
    edd->GetWorldPosition(rpos.GetData());
    rpos = rpos - origin;

    svtkVector3d rdir;
    edd->GetWorldDirection(rdir.GetData());

    double lengtha0 = svtkMath::Normalize(axis0.GetData());
    double lengtha1 = svtkMath::Normalize(axis1.GetData());

    svtkVector3d pnorm;
    pnorm = axis0.Cross(axis1);
    pnorm.Normalize();
    double dist = rpos.Dot(pnorm) / rdir.Dot(pnorm);
    rpos = rpos - rdir * dist;
    double wCoords[2] = { 0.0, 0.0 };
    wCoords[0] = rpos.Dot(axis0) / lengtha0;
    wCoords[1] = rpos.Dot(axis1) / lengtha1;

    if (wCoords[0] < 0.0 || wCoords[0] > 1.0 || wCoords[1] < 0.0 || wCoords[1] > 1.0)
    {
      this->InteractionState = svtkQWidgetRepresentation::Outside;
      return this->InteractionState;
    }

    // the ray hit the widget
    this->ValidPick = 1;
    this->InteractionState = svtkQWidgetRepresentation::Inside;

    QWidget* widget = this->QWidgetTexture->GetWidget();
    this->WidgetCoordinates[0] = wCoords[0] * widget->width();
    this->WidgetCoordinates[1] = wCoords[1] * widget->height();
    this->WidgetCoordinates[1] = widget->height() - this->WidgetCoordinates[1];
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
double* svtkQWidgetRepresentation::GetBounds()
{
  this->BuildRepresentation();
  return this->PlaneActor->GetBounds();
}

//----------------------------------------------------------------------------
void svtkQWidgetRepresentation::GetActors(svtkPropCollection* pc)
{
  this->PlaneActor->GetActors(pc);
}

//----------------------------------------------------------------------------
void svtkQWidgetRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->PlaneActor->ReleaseGraphicsResources(w);
  this->PlaneMapper->ReleaseGraphicsResources(w);
  this->PlaneTexture->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int svtkQWidgetRepresentation::RenderOpaqueGeometry(svtkViewport* v)
{
  svtkInformation* info = this->GetPropertyKeys();
  this->PlaneActor->SetPropertyKeys(info);

  svtkOpenGLRenderWindow* renWin =
    static_cast<svtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  svtkOpenGLState* ostate = renWin->GetState();

  // always draw over the rest
  ostate->svtkglDepthFunc(GL_ALWAYS);
  int result = this->PlaneActor->RenderOpaqueGeometry(v);
  ostate->svtkglDepthFunc(GL_LEQUAL);

  return result;
}

//-----------------------------------------------------------------------------
int svtkQWidgetRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport*)
{
  return 0;
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkQWidgetRepresentation::HasTranslucentPolygonalGeometry()
{
  return false;
}

//----------------------------------------------------------------------------
void svtkQWidgetRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}

//----------------------------------------------------------------------------
void svtkQWidgetRepresentation::PlaceWidget(double bds[6])
{
  this->PlaneSource->SetOrigin(bds[0], bds[2], bds[4]);
  this->PlaneSource->SetPoint1(bds[1], bds[2], bds[4]);
  this->PlaneSource->SetPoint2(bds[0], bds[2], bds[5]);

  this->ValidPick = 1; // since we have positioned the widget successfully
}

//----------------------------------------------------------------------------
svtkPolyDataAlgorithm* svtkQWidgetRepresentation::GetPolyDataAlgorithm()
{
  return this->PlaneSource;
}

//----------------------------------------------------------------------------
void svtkQWidgetRepresentation::UpdatePlacement() {}

//----------------------------------------------------------------------------
void svtkQWidgetRepresentation::BuildRepresentation()
{
  // rep is always built via plane source and doesn't change
}

//----------------------------------------------------------------------
void svtkQWidgetRepresentation::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}
