/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayRenderMesh.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can do simple mesh rendering with ospray
// and that SVTK's many standard rendering modes (points, lines, surface, with
// a variety of color controls (actor, point, cell, texture) etc work as
// they should.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit.
//              In interactive mode it responds to the keys listed
//              svtkOSPRayTestInteractor.h
// -GL       => users OpenGL instead of OSPRay to render
// -type N   => where N is one of 0,1,2, or 3 makes meshes consisting of
//              points, wireframes, triangles (=the default) or triangle strips
// -rep N    => where N is one of 0,1 or 2 draws the meshes as points, lines
//              or surfaces

#include "svtkActor.h"
#include "svtkActorCollection.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkExtractEdges.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkOSPRayActorNode.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkStripper.h"
#include "svtkTexture.h"
#include "svtkTextureMapToSphere.h"
#include "svtkTransformTextureCoords.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVertexGlyphFilter.h"

#include <string>
#include <vector>

#include "svtkOSPRayTestInteractor.h"

class renderable
{
public:
  svtkSphereSource* s;
  svtkPolyDataMapper* m;
  svtkActor* a;
  ~renderable()
  {
    s->Delete();
    m->Delete();
    a->Delete();
  }
};

renderable* MakeSphereAt(double x, double y, double z, int res, int type, int rep, const char* name)
{
  svtkOSPRayTestInteractor::AddName(name);
  renderable* ret = new renderable;
  ret->s = svtkSphereSource::New();
  ret->s->SetEndTheta(180); // half spheres better show variation and f and back
  ret->s->SetStartPhi(30);
  ret->s->SetEndPhi(150);
  ret->s->SetPhiResolution(res);
  ret->s->SetThetaResolution(res);
  ret->s->SetCenter(x, y, z);
  // make texture coordinate
  svtkSmartPointer<svtkTextureMapToSphere> tc = svtkSmartPointer<svtkTextureMapToSphere>::New();
  tc->SetCenter(x, y, z);
  tc->PreventSeamOn();
  tc->AutomaticSphereGenerationOff();
  tc->SetInputConnection(ret->s->GetOutputPort());
  svtkSmartPointer<svtkTransformTextureCoords> tt = svtkSmartPointer<svtkTransformTextureCoords>::New();
  tt->SetInputConnection(tc->GetOutputPort());
  // tt->SetScale(1,0.5,1);
  // make normals
  svtkSmartPointer<svtkPolyDataNormals> nl = svtkSmartPointer<svtkPolyDataNormals>::New();
  nl->SetInputConnection(tt->GetOutputPort());
  nl->Update();
  // make more attribute arrays
  svtkPolyData* pd = nl->GetOutput();
  // point aligned
  svtkSmartPointer<svtkDoubleArray> da = svtkSmartPointer<svtkDoubleArray>::New();
  da->SetName("testarray1");
  da->SetNumberOfComponents(1);
  pd->GetPointData()->AddArray(da);
  int np = pd->GetNumberOfPoints();
  int nc = pd->GetNumberOfCells();
  for (int i = 0; i < np; i++)
  {
    da->InsertNextValue((double)i / (double)np);
  }
  da = svtkSmartPointer<svtkDoubleArray>::New();
  da->SetName("testarray2");
  da->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(da);
  for (int i = 0; i < np; i++)
  {
    double vals[3] = { (double)i / (double)np, (double)(i * 4) / (double)np - 2.0, 42.0 };
    da->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }

  svtkSmartPointer<svtkUnsignedCharArray> pac = svtkSmartPointer<svtkUnsignedCharArray>::New();
  pac->SetName("testarrayc1");
  pac->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(pac);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>(255 * ((double)i / (double)np)),
      static_cast<unsigned char>(255 * ((double)(i * 4) / (double)np - 2.0)), 42 };
    pac->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }

  svtkSmartPointer<svtkUnsignedCharArray> ca = svtkSmartPointer<svtkUnsignedCharArray>::New();
  ca->SetName("testarray3");
  ca->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(ca);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>((double)i / (double)np * 255),
      static_cast<unsigned char>((double)(1 - i) / (double)np), 42 };
    ca->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  // cell aligned
  da = svtkSmartPointer<svtkDoubleArray>::New();
  da->SetName("testarray4");
  da->SetNumberOfComponents(1);
  pd->GetCellData()->AddArray(da);
  for (int i = 0; i < pd->GetNumberOfCells(); i++)
  {
    da->InsertNextValue((double)i / (double)pd->GetNumberOfCells());
  }
  da = svtkSmartPointer<svtkDoubleArray>::New();
  da->SetName("testarray5");
  da->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(da);
  for (int i = 0; i < nc; i++)
  {
    double vals[3] = { (double)i / (double)nc, (double)(i * 2) / (double)nc, 42.0 };
    da->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  ca = svtkSmartPointer<svtkUnsignedCharArray>::New();
  ca->SetName("testarray6");
  ca->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(ca);
  for (int i = 0; i < nc; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>((double)i / (double)np * 255),
      static_cast<unsigned char>((double)(1 - i) / (double)np), 42 };
    ca->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  ret->m = svtkPolyDataMapper::New();
  ret->m->SetInputData(pd);

  switch (type)
  {
    case 0: // points
    {
      svtkSmartPointer<svtkVertexGlyphFilter> filter = svtkSmartPointer<svtkVertexGlyphFilter>::New();
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
    case 1: // lines
    {
      svtkSmartPointer<svtkExtractEdges> filter = svtkSmartPointer<svtkExtractEdges>::New();
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
    case 2: // polys
      break;
    case 3: // strips
    {
      svtkSmartPointer<svtkStripper> filter = svtkSmartPointer<svtkStripper>::New();
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
  }
  ret->a = svtkActor::New();
  ret->a->SetMapper(ret->m);
  ret->a->GetProperty()->SetPointSize(20);
  ret->a->GetProperty()->SetLineWidth(10);
  if (rep != -1)
  {
    ret->a->GetProperty()->SetRepresentation(rep);
  }
  return ret;
}

int TestOSPRayRenderMesh(int argc, char* argv[])
{
  bool useGL = false;
  int type = 2;
  int rep = -1;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      useGL = true;
    }
    if (!strcmp(argv[i], "-type"))
    {
      type = atoi(argv[i + 1]);
    }
    if (!strcmp(argv[i], "-rep"))
    {
      rep = atoi(argv[i + 1]);
    }
  }

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->AutomaticLightCreationOn();
  renderer->SetBackground(0.75, 0.75, 0.75);
  renderer->SetEnvironmentalBG(0.75, 0.75, 0.75);
  renWin->SetSize(600, 550);
  svtkSmartPointer<svtkCamera> camera = svtkSmartPointer<svtkCamera>::New();
  camera->SetPosition(2.5, 11, -3);
  camera->SetFocalPoint(2.5, 0, -3);
  camera->SetViewUp(0, 0, 1);
  renderer->SetActiveCamera(camera);
  renWin->Render();

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  if (!useGL)
  {
    renderer->SetPass(ospray);

    for (int i = 0; i < argc; ++i)
    {
      if (!strcmp(argv[i], "--OptiX"))
      {
        svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
        break;
      }
    }
  }
  // Now, vary most of the many parameters that rendering can vary by.

  // representations points, wireframe, surface
  renderable* ren = MakeSphereAt(5, 0, -5, 10, type, rep, "points");
  ren->a->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(ren->a);
  delete (ren);

  ren = MakeSphereAt(5, 0, -4, 10, type, rep, "wireframe");
  ren->a->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(ren->a);
  delete (ren);

  ren = MakeSphereAt(5, 0, -3, 10, type, rep, "surface");
  ren->a->GetProperty()->SetRepresentationToSurface();
  renderer->AddActor(ren->a);
  delete (ren);

  // actor color
  ren = MakeSphereAt(4, 0, -5, 10, type, rep, "actor_color");
  ren->a->GetProperty()->SetColor(0, 1, 0);
  renderer->AddActor(ren->a);
  delete (ren);

  // ambient, diffuse, and specular components
  ren = MakeSphereAt(4, 0, -4, 7, type, rep, "amb/diff/spec");
  ren->a->GetProperty()->SetAmbient(0.5);
  ren->a->GetProperty()->SetAmbientColor(0.1, 0.1, 0.3);
  ren->a->GetProperty()->SetDiffuse(0.4);
  ren->a->GetProperty()->SetDiffuseColor(0.5, 0.1, 0.1);
  ren->a->GetProperty()->SetSpecular(0.2);
  ren->a->GetProperty()->SetSpecularColor(1, 1, 1);
  ren->a->GetProperty()->SetSpecularPower(100);
  ren->a->GetProperty()->SetInterpolationToPhong();
  renderer->AddActor(ren->a);
  delete (ren);

  // opacity
  ren = MakeSphereAt(4, 0, -3, 10, type, rep, "opacity");
  ren->a->GetProperty()->SetOpacity(0.2);
  renderer->AddActor(ren->a);
  delete (ren);

  // color map cell values
  ren = MakeSphereAt(3, 0, -5, 10, type, rep, "cell_value");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(0);
  renderer->AddActor(ren->a);
  delete (ren);

  // default color component
  ren = MakeSphereAt(3, 0, -4, 10, type, rep, "cell_default_comp");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(1);
  renderer->AddActor(ren->a);
  delete (ren);

  // choose color component
  ren = MakeSphereAt(3, 0, -3, 10, type, rep, "cell_comp_1");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(1);
  ren->m->ColorByArrayComponent(1, 1); // todo, use lut since this is deprecated
  renderer->AddActor(ren->a);
  delete (ren);

  // RGB direct
  ren = MakeSphereAt(3, 0, -2, 10, type, rep, "cell_rgb");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(2);
  renderer->AddActor(ren->a);
  delete (ren);

  // RGB through LUT
  ren = MakeSphereAt(3, 0, -1, 10, type, rep, "cell_rgb_through_LUT");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(2);
  ren->m->SetColorModeToMapScalars();
  renderer->AddActor(ren->a);
  delete (ren);

  // color map point values
  ren = MakeSphereAt(2, 0, -5, 6, type, rep, "point_value");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SelectColorArray("testarray1");
  renderer->AddActor(ren->a);
  delete (ren);

  // interpolate scalars before mapping
  ren = MakeSphereAt(2, 0, -4, 6, type, rep, "point_interp");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SelectColorArray("testarray1");
  ren->m->InterpolateScalarsBeforeMappingOn();
  renderer->AddActor(ren->a);
  delete (ren);

  // RGB direct
  ren = MakeSphereAt(2, 0, -3, 10, type, rep, "point_rgb");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SetColorModeToDefault();
  ren->m->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->a);
  delete (ren);

  // RGB mapped
  ren = MakeSphereAt(2, 0, -2, 10, type, rep, "point_rgb_through_LUT");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SetColorModeToMapScalars();
  ren->m->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->a);
  delete (ren);

  // unlit, flat, and gouraud lighting
  ren = MakeSphereAt(1, 0, -5, 7, type, rep, "not_lit");
  ren->a->GetProperty()->LightingOff();
  renderer->AddActor(ren->a);
  delete (ren);

  ren = MakeSphereAt(1, 0, -4, 7, type, rep, "flat");
  ren->a->GetProperty()->SetInterpolationToFlat();
  renderer->AddActor(ren->a);
  delete (ren);

  ren = MakeSphereAt(1, 0, -3, 7, type, rep, "gouraud");
  ren->a->GetProperty()->SetInterpolationToGouraud();
  renderer->AddActor(ren->a);
  delete (ren);

  // texture
  int maxi = 100;
  int maxj = 100;
  svtkSmartPointer<svtkImageData> texin = svtkSmartPointer<svtkImageData>::New();
  texin->SetExtent(0, maxi, 0, maxj, 0, 0);
  texin->AllocateScalars(SVTK_UNSIGNED_CHAR, 3);
  svtkUnsignedCharArray* aa =
    svtkArrayDownCast<svtkUnsignedCharArray>(texin->GetPointData()->GetScalars());
  int idx = 0;
  for (int i = 0; i <= maxi; i++)
  {
    for (int j = 0; j <= maxj; j++)
    {
      bool ival = (i / 10) % 2 == 1;
      bool jval = (j / 10) % 2 == 1;
      unsigned char val = (ival ^ jval) ? 255 : 0;
      aa->SetTuple3(idx, val, val, val);
      if (j <= 3 || j >= maxj - 3)
      {
        aa->SetTuple3(idx, 255, 255, 0);
      }
      if (i <= 20 || i >= maxi - 20)
      {
        aa->SetTuple3(idx, 255, 0, 0);
      }
      idx = idx + 1;
    }
  }
  ren = MakeSphereAt(0, 0, -5, 20, type, rep, "texture");
  renderer->AddActor(ren->a);
  svtkSmartPointer<svtkTexture> texture = svtkSmartPointer<svtkTexture>::New();
  texture->SetInputData(texin);
  ren->a->SetTexture(texture);
  delete (ren);

  // imagespace positional transformations
  ren = MakeSphereAt(0, 0, -4, 10, type, rep, "transform");
  ren->a->SetScale(1.2, 1.0, 0.87);
  renderer->AddActor(ren->a);
  delete (ren);

  // TODO: lut manipulation and range effects
  // TODO: NaN colors
  // TODO: mapper clipping planes
  // TODO: hierarchical actors

  renWin->Render();

  svtkLight* light = svtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  double lColor[3];
  light->GetDiffuseColor(lColor);
  light->SetPosition(2, 15, -2);
  light->SetFocalPoint(2, 0, -2);
  light->PositionalOff();

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
