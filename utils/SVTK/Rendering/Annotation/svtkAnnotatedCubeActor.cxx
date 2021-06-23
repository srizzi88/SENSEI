/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAnnotatedCubeActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAnnotatedCubeActor.h"

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkAssembly.h"
#include "svtkCubeSource.h"
#include "svtkFeatureEdges.h"
#include "svtkObject.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPropCollection.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"
#include "svtkVectorText.h"

svtkStandardNewMacro(svtkAnnotatedCubeActor);

//-------------------------------------------------------------------------
svtkAnnotatedCubeActor::svtkAnnotatedCubeActor()
{
  this->FaceTextScale = 0.5;
  this->XPlusFaceText = nullptr;
  this->XMinusFaceText = nullptr;
  this->YPlusFaceText = nullptr;
  this->YMinusFaceText = nullptr;
  this->ZPlusFaceText = nullptr;
  this->ZMinusFaceText = nullptr;

  this->Assembly = svtkAssembly::New();

  this->CubeSource = svtkCubeSource::New();
  this->CubeSource->SetBounds(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5);
  this->CubeSource->SetCenter(0, 0, 0);

  svtkPolyDataMapper* cubeMapper = svtkPolyDataMapper::New();
  this->CubeActor = svtkActor::New();
  cubeMapper->SetInputConnection(this->CubeSource->GetOutputPort());
  this->CubeActor->SetMapper(cubeMapper);
  cubeMapper->Delete();

  this->Assembly->AddPart(this->CubeActor);

  svtkProperty* prop = this->CubeActor->GetProperty();
  prop->SetRepresentationToSurface();
  prop->SetColor(1, 1, 1);
  prop->SetLineWidth(1);

  this->SetXPlusFaceText("X+");
  this->SetXMinusFaceText("X-");
  this->SetYPlusFaceText("Y+");
  this->SetYMinusFaceText("Y-");
  this->SetZPlusFaceText("Z+");
  this->SetZMinusFaceText("Z-");

  this->XPlusFaceVectorText = svtkVectorText::New();
  this->XMinusFaceVectorText = svtkVectorText::New();
  this->YPlusFaceVectorText = svtkVectorText::New();
  this->YMinusFaceVectorText = svtkVectorText::New();
  this->ZPlusFaceVectorText = svtkVectorText::New();
  this->ZMinusFaceVectorText = svtkVectorText::New();

  svtkPolyDataMapper* xplusMapper = svtkPolyDataMapper::New();
  svtkPolyDataMapper* xminusMapper = svtkPolyDataMapper::New();
  svtkPolyDataMapper* yplusMapper = svtkPolyDataMapper::New();
  svtkPolyDataMapper* yminusMapper = svtkPolyDataMapper::New();
  svtkPolyDataMapper* zplusMapper = svtkPolyDataMapper::New();
  svtkPolyDataMapper* zminusMapper = svtkPolyDataMapper::New();

  xplusMapper->SetInputConnection(this->XPlusFaceVectorText->GetOutputPort());
  xminusMapper->SetInputConnection(this->XMinusFaceVectorText->GetOutputPort());
  yplusMapper->SetInputConnection(this->YPlusFaceVectorText->GetOutputPort());
  yminusMapper->SetInputConnection(this->YMinusFaceVectorText->GetOutputPort());
  zplusMapper->SetInputConnection(this->ZPlusFaceVectorText->GetOutputPort());
  zminusMapper->SetInputConnection(this->ZMinusFaceVectorText->GetOutputPort());

  this->XPlusFaceActor = svtkActor::New();
  this->XMinusFaceActor = svtkActor::New();
  this->YPlusFaceActor = svtkActor::New();
  this->YMinusFaceActor = svtkActor::New();
  this->ZPlusFaceActor = svtkActor::New();
  this->ZMinusFaceActor = svtkActor::New();

  this->XPlusFaceActor->SetMapper(xplusMapper);
  this->XMinusFaceActor->SetMapper(xminusMapper);
  this->YPlusFaceActor->SetMapper(yplusMapper);
  this->YMinusFaceActor->SetMapper(yminusMapper);
  this->ZPlusFaceActor->SetMapper(zplusMapper);
  this->ZMinusFaceActor->SetMapper(zminusMapper);

  xplusMapper->Delete();
  xminusMapper->Delete();
  yplusMapper->Delete();
  yminusMapper->Delete();
  zplusMapper->Delete();
  zminusMapper->Delete();

  this->Assembly->AddPart(this->XPlusFaceActor);
  this->Assembly->AddPart(this->XMinusFaceActor);
  this->Assembly->AddPart(this->YPlusFaceActor);
  this->Assembly->AddPart(this->YMinusFaceActor);
  this->Assembly->AddPart(this->ZPlusFaceActor);
  this->Assembly->AddPart(this->ZMinusFaceActor);

  prop = this->XPlusFaceActor->GetProperty();
  prop->SetColor(1, 1, 1);
  prop->SetDiffuse(0);
  prop->SetAmbient(1);
  prop->BackfaceCullingOn();
  this->XMinusFaceActor->GetProperty()->DeepCopy(prop);
  this->YPlusFaceActor->GetProperty()->DeepCopy(prop);
  this->YMinusFaceActor->GetProperty()->DeepCopy(prop);
  this->ZPlusFaceActor->GetProperty()->DeepCopy(prop);
  this->ZMinusFaceActor->GetProperty()->DeepCopy(prop);

  this->AppendTextEdges = svtkAppendPolyData::New();
  this->AppendTextEdges->UserManagedInputsOn();
  this->AppendTextEdges->SetNumberOfInputs(6);

  for (int i = 0; i < 6; i++)
  {
    svtkPolyData* edges = svtkPolyData::New();
    this->AppendTextEdges->SetInputDataByNumber(i, edges);
    edges->Delete();
  }

  this->ExtractTextEdges = svtkFeatureEdges::New();
  this->ExtractTextEdges->BoundaryEdgesOn();
  this->ExtractTextEdges->ColoringOff();
  this->ExtractTextEdges->SetInputConnection(this->AppendTextEdges->GetOutputPort());

  svtkPolyDataMapper* edgesMapper = svtkPolyDataMapper::New();
  edgesMapper->SetInputConnection(this->ExtractTextEdges->GetOutputPort());

  this->TextEdgesActor = svtkActor::New();
  this->TextEdgesActor->SetMapper(edgesMapper);
  edgesMapper->Delete();

  this->Assembly->AddPart(this->TextEdgesActor);

  prop = this->TextEdgesActor->GetProperty();
  prop->SetRepresentationToWireframe();
  prop->SetColor(1, 0.5, 0);
  prop->SetDiffuse(0);
  prop->SetAmbient(1);
  prop->SetLineWidth(1);

  this->InternalTransformFilter = svtkTransformFilter::New();
  this->InternalTransform = svtkTransform::New();
  this->InternalTransformFilter->SetTransform(this->InternalTransform);

  this->XFaceTextRotation = 0.0;
  this->YFaceTextRotation = 0.0;
  this->ZFaceTextRotation = 0.0;

  this->UpdateProps();
}

//-------------------------------------------------------------------------
svtkAnnotatedCubeActor::~svtkAnnotatedCubeActor()
{
  this->CubeSource->Delete();
  this->CubeActor->Delete();

  this->SetXPlusFaceText(nullptr);
  this->SetXMinusFaceText(nullptr);
  this->SetYPlusFaceText(nullptr);
  this->SetYMinusFaceText(nullptr);
  this->SetZPlusFaceText(nullptr);
  this->SetZMinusFaceText(nullptr);

  this->XPlusFaceVectorText->Delete();
  this->XMinusFaceVectorText->Delete();
  this->YPlusFaceVectorText->Delete();
  this->YMinusFaceVectorText->Delete();
  this->ZPlusFaceVectorText->Delete();
  this->ZMinusFaceVectorText->Delete();

  this->XPlusFaceActor->Delete();
  this->XMinusFaceActor->Delete();
  this->YPlusFaceActor->Delete();
  this->YMinusFaceActor->Delete();
  this->ZPlusFaceActor->Delete();
  this->ZMinusFaceActor->Delete();

  this->AppendTextEdges->Delete();
  this->ExtractTextEdges->Delete();
  this->TextEdgesActor->Delete();

  this->InternalTransformFilter->Delete();
  this->InternalTransform->Delete();

  this->Assembly->Delete();
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::SetTextEdgesVisibility(int vis)
{
  this->TextEdgesActor->SetVisibility(vis);
  this->Assembly->Modified();
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::SetCubeVisibility(int vis)
{
  this->CubeActor->SetVisibility(vis);
  this->Assembly->Modified();
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::SetFaceTextVisibility(int vis)
{
  this->XPlusFaceActor->SetVisibility(vis);
  this->XMinusFaceActor->SetVisibility(vis);
  this->YPlusFaceActor->SetVisibility(vis);
  this->YMinusFaceActor->SetVisibility(vis);
  this->ZPlusFaceActor->SetVisibility(vis);
  this->ZMinusFaceActor->SetVisibility(vis);
  this->Assembly->Modified();
}

//-------------------------------------------------------------------------
int svtkAnnotatedCubeActor::GetTextEdgesVisibility()
{
  return this->TextEdgesActor->GetVisibility();
}

//-------------------------------------------------------------------------
int svtkAnnotatedCubeActor::GetCubeVisibility()
{
  return this->CubeActor->GetVisibility();
}

//-------------------------------------------------------------------------
int svtkAnnotatedCubeActor::GetFaceTextVisibility()
{
  // either they are all visible or not, so one response will do
  return this->XPlusFaceActor->GetVisibility();
}

//-------------------------------------------------------------------------
// Shallow copy of a svtkAnnotatedCubeActor.
void svtkAnnotatedCubeActor::ShallowCopy(svtkProp* prop)
{
  svtkAnnotatedCubeActor* a = svtkAnnotatedCubeActor::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetXPlusFaceText(a->GetXPlusFaceText());
    this->SetXMinusFaceText(a->GetXMinusFaceText());
    this->SetYPlusFaceText(a->GetYPlusFaceText());
    this->SetYMinusFaceText(a->GetYMinusFaceText());
    this->SetZPlusFaceText(a->GetZPlusFaceText());
    this->SetZMinusFaceText(a->GetZMinusFaceText());
    this->SetFaceTextScale(a->GetFaceTextScale());
  }

  // Now do superclass
  this->svtkProp3D::ShallowCopy(prop);
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::GetActors(svtkPropCollection* ac)
{
  this->Assembly->GetActors(ac);
}

//-------------------------------------------------------------------------
int svtkAnnotatedCubeActor::RenderOpaqueGeometry(svtkViewport* vp)
{
  this->UpdateProps();

  return this->Assembly->RenderOpaqueGeometry(vp);
}

//-----------------------------------------------------------------------------
int svtkAnnotatedCubeActor::RenderTranslucentPolygonalGeometry(svtkViewport* vp)
{
  this->UpdateProps();

  return this->Assembly->RenderTranslucentPolygonalGeometry(vp);
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkAnnotatedCubeActor::HasTranslucentPolygonalGeometry()
{
  this->UpdateProps();

  return this->Assembly->HasTranslucentPolygonalGeometry();
}

//-----------------------------------------------------------------------------
void svtkAnnotatedCubeActor::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Assembly->ReleaseGraphicsResources(win);
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::GetBounds(double bounds[6])
{
  this->Assembly->GetBounds(bounds);
}

//-------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkAnnotatedCubeActor::GetBounds()
{
  return this->Assembly->GetBounds();
}

//-------------------------------------------------------------------------
svtkMTimeType svtkAnnotatedCubeActor::GetMTime()
{
  return this->Assembly->GetMTime();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetXPlusFaceProperty()
{
  return this->XPlusFaceActor->GetProperty();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetXMinusFaceProperty()
{
  return this->XMinusFaceActor->GetProperty();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetYPlusFaceProperty()
{
  return this->YPlusFaceActor->GetProperty();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetYMinusFaceProperty()
{
  return this->YMinusFaceActor->GetProperty();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetZPlusFaceProperty()
{
  return this->ZPlusFaceActor->GetProperty();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetZMinusFaceProperty()
{
  return this->ZMinusFaceActor->GetProperty();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetCubeProperty()
{
  return this->CubeActor->GetProperty();
}

//-------------------------------------------------------------------------
svtkProperty* svtkAnnotatedCubeActor::GetTextEdgesProperty()
{
  return this->TextEdgesActor->GetProperty();
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::SetFaceTextScale(double scale)
{
  if (this->FaceTextScale == scale)
  {
    return;
  }
  this->FaceTextScale = scale;
  this->UpdateProps();
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::UpdateProps()
{
  this->XPlusFaceVectorText->SetText(this->XPlusFaceText);
  this->XMinusFaceVectorText->SetText(this->XMinusFaceText);
  this->YPlusFaceVectorText->SetText(this->YPlusFaceText);
  this->YMinusFaceVectorText->SetText(this->YMinusFaceText);
  this->ZPlusFaceVectorText->SetText(this->ZPlusFaceText);
  this->ZMinusFaceVectorText->SetText(this->ZMinusFaceText);

  svtkProperty* prop = this->CubeActor->GetProperty();

  // Place the text slightly offset from the cube face to prevent
  // rendering problems when the cube is in surface render mode.
  double offset = (prop->GetRepresentation() == SVTK_SURFACE) ? (0.501) : (0.5);

  this->XPlusFaceVectorText->Update();
  const double* bounds = this->XPlusFaceVectorText->GetOutput()->GetBounds();
  double cu = -this->FaceTextScale * fabs(0.5 * (bounds[0] + bounds[1]));
  double cv = -this->FaceTextScale * fabs(0.5 * (bounds[2] + bounds[3]));

  this->XPlusFaceActor->SetScale(this->FaceTextScale);
  this->XPlusFaceActor->SetPosition(offset, cu, cv);
  this->XPlusFaceActor->SetOrientation(90, 0, 90);

  this->XMinusFaceVectorText->Update();
  bounds = this->XMinusFaceVectorText->GetOutput()->GetBounds();
  cu = this->FaceTextScale * fabs(0.5 * (bounds[0] + bounds[1]));
  cv = -this->FaceTextScale * fabs(0.5 * (bounds[2] + bounds[3]));

  this->XMinusFaceActor->SetScale(this->FaceTextScale);
  this->XMinusFaceActor->SetPosition(-offset, cu, cv);
  this->XMinusFaceActor->SetOrientation(90, 0, -90);

  if (this->XFaceTextRotation != 0.0)
  {
    svtkTransform* transform = svtkTransform::New();
    transform->Identity();
    transform->RotateX(this->XFaceTextRotation);
    this->XPlusFaceActor->SetUserTransform(transform);
    this->XMinusFaceActor->SetUserTransform(transform);
    transform->Delete();
  }

  this->YPlusFaceVectorText->Update();
  bounds = this->YPlusFaceVectorText->GetOutput()->GetBounds();
  cu = this->FaceTextScale * 0.5 * (bounds[0] + bounds[1]);
  cv = -this->FaceTextScale * 0.5 * (bounds[2] + bounds[3]);

  this->YPlusFaceActor->SetScale(this->FaceTextScale);
  this->YPlusFaceActor->SetPosition(cu, offset, cv);
  this->YPlusFaceActor->SetOrientation(90, 0, 180);

  this->YMinusFaceVectorText->Update();
  bounds = this->YMinusFaceVectorText->GetOutput()->GetBounds();
  cu = -this->FaceTextScale * 0.5 * (bounds[0] + bounds[1]);
  cv = -this->FaceTextScale * 0.5 * (bounds[2] + bounds[3]);

  this->YMinusFaceActor->SetScale(this->FaceTextScale);
  this->YMinusFaceActor->SetPosition(cu, -offset, cv);
  this->YMinusFaceActor->SetOrientation(90, 0, 0);

  if (this->YFaceTextRotation != 0.0)
  {
    svtkTransform* transform = svtkTransform::New();
    transform->Identity();
    transform->RotateY(this->YFaceTextRotation);
    this->YPlusFaceActor->SetUserTransform(transform);
    this->YMinusFaceActor->SetUserTransform(transform);
    transform->Delete();
  }

  this->ZPlusFaceVectorText->Update();
  bounds = this->ZPlusFaceVectorText->GetOutput()->GetBounds();
  cu = this->FaceTextScale * 0.5 * (bounds[0] + bounds[1]);
  cv = -this->FaceTextScale * 0.5 * (bounds[2] + bounds[3]);

  this->ZPlusFaceActor->SetScale(this->FaceTextScale);
  this->ZPlusFaceActor->SetPosition(cv, cu, offset);
  this->ZPlusFaceActor->SetOrientation(0, 0, -90);

  this->ZMinusFaceVectorText->Update();
  bounds = this->ZMinusFaceVectorText->GetOutput()->GetBounds();
  cu = -this->FaceTextScale * 0.5 * (bounds[0] + bounds[1]);
  cv = -this->FaceTextScale * 0.5 * (bounds[2] + bounds[3]);

  this->ZMinusFaceActor->SetScale(this->FaceTextScale);
  this->ZMinusFaceActor->SetPosition(cv, cu, -offset);
  this->ZMinusFaceActor->SetOrientation(180, 0, 90);

  if (this->ZFaceTextRotation != 0.0)
  {
    svtkTransform* transform = svtkTransform::New();
    transform->Identity();
    transform->RotateZ(this->ZFaceTextRotation);
    this->ZPlusFaceActor->SetUserTransform(transform);
    this->ZMinusFaceActor->SetUserTransform(transform);
    transform->Delete();
  }

  this->XPlusFaceActor->ComputeMatrix();
  this->InternalTransformFilter->SetInputConnection(this->XPlusFaceVectorText->GetOutputPort());
  this->InternalTransform->SetMatrix(this->XPlusFaceActor->GetMatrix());
  this->InternalTransformFilter->Update();
  svtkPolyData* edges = this->AppendTextEdges->GetInput(0);
  edges->CopyStructure(this->InternalTransformFilter->GetOutput());

  this->XMinusFaceActor->ComputeMatrix();
  this->InternalTransformFilter->SetInputConnection(this->XMinusFaceVectorText->GetOutputPort());
  this->InternalTransform->SetMatrix(this->XMinusFaceActor->GetMatrix());
  this->InternalTransformFilter->Update();
  edges = this->AppendTextEdges->GetInput(1);
  edges->CopyStructure(this->InternalTransformFilter->GetOutput());

  this->YPlusFaceActor->ComputeMatrix();
  this->InternalTransformFilter->SetInputConnection(this->YPlusFaceVectorText->GetOutputPort());
  this->InternalTransform->SetMatrix(this->YPlusFaceActor->GetMatrix());
  this->InternalTransformFilter->Update();
  edges = this->AppendTextEdges->GetInput(2);
  edges->CopyStructure(this->InternalTransformFilter->GetOutput());

  this->YMinusFaceActor->ComputeMatrix();
  this->InternalTransformFilter->SetInputConnection(this->YMinusFaceVectorText->GetOutputPort());
  this->InternalTransform->SetMatrix(this->YMinusFaceActor->GetMatrix());
  this->InternalTransformFilter->Update();
  edges = this->AppendTextEdges->GetInput(3);
  edges->CopyStructure(this->InternalTransformFilter->GetOutput());

  this->ZPlusFaceActor->ComputeMatrix();
  this->InternalTransformFilter->SetInputConnection(this->ZPlusFaceVectorText->GetOutputPort());
  this->InternalTransform->SetMatrix(this->ZPlusFaceActor->GetMatrix());
  this->InternalTransformFilter->Update();
  edges = this->AppendTextEdges->GetInput(4);
  edges->CopyStructure(this->InternalTransformFilter->GetOutput());

  this->ZMinusFaceActor->ComputeMatrix();
  this->InternalTransformFilter->SetInputConnection(this->ZMinusFaceVectorText->GetOutputPort());
  this->InternalTransform->SetMatrix(this->ZMinusFaceActor->GetMatrix());
  this->InternalTransformFilter->Update();
  edges = this->AppendTextEdges->GetInput(5);
  edges->CopyStructure(this->InternalTransformFilter->GetOutput());
}

//-------------------------------------------------------------------------
void svtkAnnotatedCubeActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XPlusFaceText: " << (this->XPlusFaceText ? this->XPlusFaceText : "(none)")
     << endl;

  os << indent << "XMinusFaceText: " << (this->XMinusFaceText ? this->XMinusFaceText : "(none)")
     << endl;

  os << indent << "YPlusFaceText: " << (this->YPlusFaceText ? this->YPlusFaceText : "(none)")
     << endl;

  os << indent << "YMinusFaceText: " << (this->YMinusFaceText ? this->YMinusFaceText : "(none)")
     << endl;

  os << indent << "ZPlusFaceText: " << (this->ZPlusFaceText ? this->ZPlusFaceText : "(none)")
     << endl;

  os << indent << "ZMinusFaceText: " << (this->ZMinusFaceText ? this->ZMinusFaceText : "(none)")
     << endl;

  os << indent << "FaceTextScale: " << this->FaceTextScale << endl;

  os << indent << "XFaceTextRotation: " << this->XFaceTextRotation << endl;

  os << indent << "YFaceTextRotation: " << this->YFaceTextRotation << endl;

  os << indent << "ZFaceTextRotation: " << this->ZFaceTextRotation << endl;
}
