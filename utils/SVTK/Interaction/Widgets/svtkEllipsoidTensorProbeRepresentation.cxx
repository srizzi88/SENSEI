/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEllipsoidTensorProbeRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEllipsoidTensorProbeRepresentation.h"
#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkCellArray.h"
#include "svtkCellPicker.h"
#include "svtkCommand.h"
#include "svtkDoubleArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTensorGlyph.h"

svtkStandardNewMacro(svtkEllipsoidTensorProbeRepresentation);

//----------------------------------------------------------------------
svtkEllipsoidTensorProbeRepresentation ::svtkEllipsoidTensorProbeRepresentation()
{
  svtkSphereSource* sphere = svtkSphereSource::New();
  sphere->SetThetaResolution(24);
  sphere->SetPhiResolution(24);

  svtkPoints* points = svtkPoints::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(1);
  points->SetPoint(0, 0.0, 0.0, 0.0);

  this->TensorSource = svtkPolyData::New();
  this->TensorSource->SetPoints(points);
  svtkDoubleArray* tensor = svtkDoubleArray::New();
  tensor->SetNumberOfComponents(9);
  tensor->Allocate(9);
  double t[9];
  t[0] = .01; // Component(0,0)
  t[4] = .01; // Component(1,1);
  t[8] = .01; // Component(2,2);
  t[3] = 0.0; // Component(0,1);
  t[6] = 0.0; // Component(0,2);
  t[1] = 0.0; // Component(1,0);
  t[7] = 0.0; // Component(1,2);
  t[2] = 0.0; // Component(2,0);
  t[5] = 0.0; // Component(2,1);
  tensor->InsertNextTuple(t);
  this->TensorSource->GetPointData()->SetTensors(tensor);
  tensor->Delete();
  points->Delete();

  this->TensorGlypher = svtkTensorGlyph::New();
  this->TensorGlypher->SetInputData(this->TensorSource);
  this->TensorGlypher->SetSourceConnection(sphere->GetOutputPort());
  this->TensorGlypher->SetScaleFactor(10);
  this->TensorGlypher->ClampScalingOn();
  sphere->Delete();

  this->PolyDataNormals = svtkPolyDataNormals::New();
  this->PolyDataNormals->AutoOrientNormalsOn();
  this->PolyDataNormals->SetInputConnection(this->TensorGlypher->GetOutputPort());

  this->EllipsoidMapper = svtkPolyDataMapper::New();
  this->EllipsoidMapper->SetInputConnection(this->PolyDataNormals->GetOutputPort());
  this->EllipsoidActor = svtkActor::New();
  this->EllipsoidActor->SetMapper(this->EllipsoidMapper);

  this->CellPicker = svtkCellPicker::New();
  this->CellPicker->PickFromListOn();
  this->CellPicker->AddPickList(this->EllipsoidActor);
  this->CellPicker->SetTolerance(0.01); // need some fluff
}

//----------------------------------------------------------------------
svtkEllipsoidTensorProbeRepresentation ::~svtkEllipsoidTensorProbeRepresentation()
{
  this->TensorSource->Delete();
  this->EllipsoidMapper->Delete();
  this->EllipsoidActor->Delete();
  this->TensorGlypher->Delete();
  this->CellPicker->Delete();
  this->PolyDataNormals->Delete();
}

//----------------------------------------------------------------------
void svtkEllipsoidTensorProbeRepresentation ::EvaluateTensor(double t[9])
{
  double p1[3], p2[3];
  this->Trajectory->GetPoints()->GetPoint(this->ProbeCellId, p1);
  this->Trajectory->GetPoints()->GetPoint(this->ProbeCellId + 1, p2);
  double d = svtkMath::Distance2BetweenPoints(p1, p2);
  double r = 1.0, t1[9], t2[9];
  if (d > 1e-12)
  {
    r = sqrt(svtkMath::Distance2BetweenPoints(this->ProbePosition, p2) / d);
  }

  if (svtkDataArray* tensors = this->Trajectory->GetPointData()->GetTensors())
  {
    tensors->GetTuple(this->ProbeCellId, t1);
    tensors->GetTuple(this->ProbeCellId + 1, t2);
    if (tensors->GetNumberOfComponents() == 6)
    {
      svtkMath::TensorFromSymmetricTensor(t1);
      svtkMath::TensorFromSymmetricTensor(t2);
    }
  }

  // NN interpolation ?
  // if ( r < 0.5 )
  //  {
  //  for (int i =0; i < 9; i++)
  //    {
  //    t[i] = t2[i];
  //    }
  //  }
  // else
  //  {
  //  for (int i =0; i < 9; i++)
  //    {
  //    t[i] = t1[i];
  //    }
  //  }

  // Linear interp
  for (int i = 0; i < 9; i++)
  {
    t[i] = r * t1[i] + (1 - r) * t2[i];
  }
}

//----------------------------------------------------------------------
void svtkEllipsoidTensorProbeRepresentation::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->CellPicker, this);
}

//----------------------------------------------------------------------
int svtkEllipsoidTensorProbeRepresentation ::RenderOpaqueGeometry(svtkViewport* viewport)
{
  int count = this->Superclass::RenderOpaqueGeometry(viewport);
  count += this->EllipsoidActor->RenderOpaqueGeometry(viewport);
  return count;
}

//----------------------------------------------------------------------
int svtkEllipsoidTensorProbeRepresentation::SelectProbe(int pos[2])
{
  this->VisibilityOn(); // actor must be on to be picked

  svtkAssemblyPath* path = this->GetAssemblyPath(pos[0], pos[1], 0., this->CellPicker);

  return path ? 1 : 0;
}

//----------------------------------------------------------------------
void svtkEllipsoidTensorProbeRepresentation::BuildRepresentation()
{
  this->Superclass::BuildRepresentation();

  svtkPoints* pts = this->TensorSource->GetPoints();
  pts->SetPoint(0, this->ProbePosition);

  double t[9];
  this->EvaluateTensor(t);
  this->TensorSource->GetPointData()->GetTensors()->SetTuple(0, t);
  this->TensorSource->Modified();
}

//----------------------------------------------------------------------
void svtkEllipsoidTensorProbeRepresentation::GetActors(svtkPropCollection* pc)
{
  this->EllipsoidActor->GetActors(pc);
}

//----------------------------------------------------------------------
void svtkEllipsoidTensorProbeRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->EllipsoidActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
void svtkEllipsoidTensorProbeRepresentation ::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EllipsoidActor: " << this->EllipsoidActor << endl;
  os << indent << "EllipsoidMapper: " << this->EllipsoidMapper << endl;
  os << indent << "TensorSource: " << this->TensorSource << endl;
  os << indent << "TensorGlypher: " << this->TensorGlypher << endl;
  os << indent << "CellPicker: " << this->CellPicker << endl;
  os << indent << "PolyDataNormals: " << this->PolyDataNormals << endl;
}
