/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOVExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   SVTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    svtkPOVExporter.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "svtkPOVExporter.h"

#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataSet.h"
#include "svtkFloatArray.h"
#include "svtkGeometryFilter.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkMapper.h"
#include "svtkMatrix4x4.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"
#include "svtkTypeTraits.h"
#include "svtkUnsignedCharArray.h"
#include <svtksys/SystemTools.hxx>

#include <sstream>

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPOVExporter);

// Can't use printf("%d", a_svtkIdType) because svtkIdType is not always int.
// This internal class holds format strings svtkPOVExporter can use instead.
class svtkPOVInternals
{
public:
  svtkPOVInternals()
  {
    this->CountFormat = new char[100]; //"\t\t%d,\n"
    strcpy(this->CountFormat, "\t\t");
    strcat(this->CountFormat, svtkTypeTraits<svtkIdType>::ParseFormat());
    strcat(this->CountFormat, ",\n");

    char* triFormat = new char[100]; //"%d, %d, %d"
    strcpy(triFormat, svtkTypeTraits<svtkIdType>::ParseFormat());
    strcat(triFormat, ", ");
    strcat(triFormat, svtkTypeTraits<svtkIdType>::ParseFormat());
    strcat(triFormat, ", ");
    strcat(triFormat, svtkTypeTraits<svtkIdType>::ParseFormat());

    this->TriangleFormat1 = new char[100]; //"\t\t<%d, %d, %d>,"
    strcpy(this->TriangleFormat1, "\t\t<");
    strcat(this->TriangleFormat1, triFormat);
    strcat(this->TriangleFormat1, ">,");

    this->TriangleFormat2 = new char[100]; //" %d, %d, %d,\n"
    strcpy(this->TriangleFormat2, " ");
    strcat(this->TriangleFormat2, triFormat);
    strcat(this->TriangleFormat2, ",\n");

    delete[] triFormat;
  }

  ~svtkPOVInternals()
  {
    delete[] this->CountFormat;
    delete[] this->TriangleFormat1;
    delete[] this->TriangleFormat2;
  }

  char* CountFormat;
  char* TriangleFormat1;
  char* TriangleFormat2;
};

#define SVTKPOV_CNTFMT this->Internals->CountFormat
#define SVTKPOV_TRIFMT1 this->Internals->TriangleFormat1
#define SVTKPOV_TRIFMT2 this->Internals->TriangleFormat2

//============================================================================
svtkPOVExporter::svtkPOVExporter()
{
  this->FileName = nullptr;
  this->FilePtr = nullptr;
  this->Internals = new svtkPOVInternals;
}

svtkPOVExporter::~svtkPOVExporter()
{
  delete[] this->FileName;
  delete this->Internals;
}

void svtkPOVExporter::WriteData()
{
  // make sure user specified a filename
  if (this->FileName == nullptr)
  {
    svtkErrorMacro(<< "Please specify file name to create");
    return;
  }

  // get the renderer
  svtkRenderer* renderer = this->ActiveRenderer;
  if (!renderer)
  {
    renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (renderer->GetActors()->GetNumberOfItems() < 1)
  {
    svtkErrorMacro(<< "no actors found for writing .pov file.");
    return;
  }

  // try opening the file
  this->FilePtr = svtksys::SystemTools::Fopen(this->FileName, "w");
  if (this->FilePtr == nullptr)
  {
    svtkErrorMacro(<< "Cannot open " << this->FileName);
    return;
  }

  // write header
  this->WriteHeader(renderer);

  // write camera
  this->WriteCamera(renderer->GetActiveCamera());

  // write lights
  svtkLightCollection* lc = renderer->GetLights();
  svtkCollectionSimpleIterator sit;
  lc->InitTraversal(sit);
  if (lc->GetNextLight(sit) == nullptr)
  {
    svtkWarningMacro(<< "No light defined, creating one at camera position");
    renderer->CreateLight();
  }
  svtkLight* light;
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
  {
    if (light->GetSwitch())
    {
      this->WriteLight(light);
    }
  }

  // write actors
  svtkActorCollection* ac = renderer->GetActors();
  svtkAssemblyPath* apath;
  svtkCollectionSimpleIterator ait;
  svtkActor *anActor, *aPart;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
  {
    for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
    {
      aPart = static_cast<svtkActor*>(apath->GetLastNode()->GetViewProp());
      this->WriteActor(aPart);
    }
  }

  fclose(this->FilePtr);
}

void svtkPOVExporter::WriteHeader(svtkRenderer* renderer)
{
  fprintf(this->FilePtr, "// POVRay file exported by svtkPOVExporter\n");
  fprintf(this->FilePtr, "//\n");

  // width and height of output image,
  // and other default command line args to POVRay
  const int* size = renderer->GetSize();
  fprintf(this->FilePtr, "// +W%d +H%d\n\n", size[0], size[1]);

  // global settings
  fprintf(this->FilePtr, "global_settings {\n");
  fprintf(this->FilePtr, "\tambient_light color rgb <1.0, 1.0, 1.0>\n");
  fprintf(this->FilePtr, "\tassumed_gamma 2\n");
  fprintf(this->FilePtr, "}\n\n");

  // background
  double* color = renderer->GetBackground();
  fprintf(this->FilePtr, "background { color rgb <%f, %f, %f>}\n\n", color[0], color[1], color[2]);
}

void svtkPOVExporter::WriteCamera(svtkCamera* camera)
{
  fprintf(this->FilePtr, "camera {\n");
  if (camera->GetParallelProjection())
  {
    fprintf(this->FilePtr, "\torthographic\n");
  }
  else
  {
    fprintf(this->FilePtr, "\tperspective\n");
  }

  double* position = camera->GetPosition();
  fprintf(this->FilePtr, "\tlocation <%f, %f, %f>\n", position[0], position[1], position[2]);

  double* up = camera->GetViewUp();
  // the camera up vector is called "sky" in POVRay
  fprintf(this->FilePtr, "\tsky <%f, %f, %f>\n", up[0], up[1], up[2]);

  // make POVRay to use left handed system to right handed
  // TODO: aspect ratio
  fprintf(this->FilePtr, "\tright <-1, 0, 0>\n");
  // fprintf(this->FilePtr, "\tup <-1, 0, 0>\n");

  fprintf(this->FilePtr, "\tangle %f\n", camera->GetViewAngle());

  double* focal = camera->GetFocalPoint();
  fprintf(this->FilePtr, "\tlook_at <%f, %f, %f>\n", focal[0], focal[1], focal[2]);

  fprintf(this->FilePtr, "}\n\n");
}

void svtkPOVExporter::WriteLight(svtkLight* light)
{
  fprintf(this->FilePtr, "light_source {\n");

  double* position = light->GetPosition();
  fprintf(this->FilePtr, "\t<%f, %f, %f>\n", position[0], position[1], position[2]);

  double* color = light->GetDiffuseColor();
  fprintf(this->FilePtr, "\tcolor <%f, %f, %f>*%f\n", color[0], color[1], color[2],
    light->GetIntensity());

  if (light->GetPositional())
  {
    fprintf(this->FilePtr, "\tspotlight\n");
    fprintf(this->FilePtr, "\tradius %f\n", light->GetConeAngle());
    fprintf(this->FilePtr, "\tfalloff %f\n", light->GetExponent());
  }
  else
  {
    fprintf(this->FilePtr, "\tparallel\n");
  }
  double* focal = light->GetFocalPoint();
  fprintf(this->FilePtr, "\tpoint_at <%f, %f, %f>\n", focal[0], focal[1], focal[2]);

  fprintf(this->FilePtr, "}\n\n");
}

void svtkPOVExporter::WriteActor(svtkActor* actor)
{
  if (actor->GetMapper() == nullptr)
  {
    return;
  }
  if (actor->GetVisibility() == 0)
  {
    return;
  }

  // write geometry, first ask the pipeline to update data
  svtkDataSet* dataset = nullptr;
  svtkSmartPointer<svtkDataSet> tempDS;

  svtkDataObject* dObj = actor->GetMapper()->GetInputDataObject(0, 0);
  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
  {
    svtkCompositeDataGeometryFilter* gf = svtkCompositeDataGeometryFilter::New();
    gf->SetInputConnection(actor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    tempDS = gf->GetOutput();
    gf->Delete();

    dataset = tempDS;
  }
  else
  {
    dataset = actor->GetMapper()->GetInput();
  }

  if (dataset == nullptr)
  {
    return;
  }
  actor->GetMapper()->GetInputAlgorithm()->Update();

  // convert non polygon data to polygon data if needed
  svtkGeometryFilter* geometryFilter = nullptr;
  svtkPolyData* polys = nullptr;
  if (dataset->GetDataObjectType() != SVTK_POLY_DATA)
  {
    geometryFilter = svtkGeometryFilter::New();
    geometryFilter->SetInputConnection(actor->GetMapper()->GetInputConnection(0, 0));
    geometryFilter->Update();
    polys = geometryFilter->GetOutput();
  }
  else
  {
    polys = static_cast<svtkPolyData*>(dataset);
  }

  // we only export Polygons and Triangle Strips
  if ((polys->GetNumberOfPolys() == 0) && (polys->GetNumberOfStrips() == 0))
  {
    return;
  }

  // write point coordinates
  svtkPoints* points = polys->GetPoints();

  // we use mesh2 since it maps better to how SVTK stores
  // polygons/triangle strips
  fprintf(this->FilePtr, "mesh2 {\n");

  fprintf(this->FilePtr, "\tvertex_vectors {\n");
  fprintf(this->FilePtr, SVTKPOV_CNTFMT, points->GetNumberOfPoints());
  for (svtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double* pos = points->GetPoint(i);
    fprintf(this->FilePtr, "\t\t<%f, %f, %f>,\n", pos[0], pos[1], pos[2]);
  }
  fprintf(this->FilePtr, "\t}\n");

  // write vertex normal
  svtkPointData* pointData = polys->GetPointData();
  if (pointData->GetNormals())
  {
    svtkDataArray* normals = pointData->GetNormals();
    fprintf(this->FilePtr, "\tnormal_vectors {\n");
    fprintf(this->FilePtr, SVTKPOV_CNTFMT, normals->GetNumberOfTuples());
    for (svtkIdType i = 0; i < normals->GetNumberOfTuples(); i++)
    {
      double* normal = normals->GetTuple(i);
      fprintf(this->FilePtr, "\t\t<%f, %f, %f>,\n", normal[0], normal[1], normal[2]);
    }
    fprintf(this->FilePtr, "\t}\n");
  }

  // TODO: write texture coordinates (uv vectors)

  // write vertex texture, ask mapper to generate color for each vertex if
  // the scalar data visibility is on
  bool scalar_visible = false;
  if (actor->GetMapper()->GetScalarVisibility())
  {
    svtkUnsignedCharArray* color_array = actor->GetMapper()->MapScalars(1.0);
    if (color_array != nullptr)
    {
      scalar_visible = true;
      fprintf(this->FilePtr, "\ttexture_list {\n");
      fprintf(this->FilePtr, SVTKPOV_CNTFMT, color_array->GetNumberOfTuples());
      for (svtkIdType i = 0; i < color_array->GetNumberOfTuples(); i++)
      {
        unsigned char* color = color_array->GetPointer(4 * i);
        fprintf(this->FilePtr, "\t\ttexture { pigment {color rgbf <%f, %f, %f, %f> } },\n",
          color[0] / 255.0, color[1] / 255.0, color[2] / 255.0, 1.0 - color[3] / 255.0);
      }
      fprintf(this->FilePtr, "\t}\n");
    }
  }

  // write polygons
  if (polys->GetNumberOfPolys() > 0)
  {
    this->WritePolygons(polys, scalar_visible);
  }

  // write triangle strips
  if (polys->GetNumberOfStrips() > 0)
  {
    this->WriteTriangleStrips(polys, scalar_visible);
  }

  // write transformation for the actor, it is column major and looks like transposed
  svtkMatrix4x4* matrix = actor->GetMatrix();
  fprintf(this->FilePtr, "\tmatrix < %f, %f, %f,\n", matrix->GetElement(0, 0),
    matrix->GetElement(1, 0), matrix->GetElement(2, 0));
  fprintf(this->FilePtr, "\t\t %f, %f, %f,\n", matrix->GetElement(0, 1), matrix->GetElement(1, 1),
    matrix->GetElement(2, 1));
  fprintf(this->FilePtr, "\t\t %f, %f, %f,\n", matrix->GetElement(0, 2), matrix->GetElement(1, 2),
    matrix->GetElement(2, 2));
  fprintf(this->FilePtr, "\t\t %f, %f, %f >\n", matrix->GetElement(0, 3), matrix->GetElement(1, 3),
    matrix->GetElement(2, 3));

  // write property
  this->WriteProperty(actor->GetProperty());

  // done with this actor
  fprintf(this->FilePtr, "}\n\n");

  if (geometryFilter)
  {
    geometryFilter->Delete();
  }
}

void svtkPOVExporter::WritePolygons(svtkPolyData* polys, bool scalar_visible)
{
  // write polygons with on the fly triangulation,
  // assuming polygon are simple and can be triangulated into "fans"
  svtkIdType numtriangles = 0;
  svtkCellArray* cells = polys->GetPolys();
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;

  // first pass,
  // calculate how many triangles there will be after triangulation
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    // the number of triangles for each polygon will be # of vertex - 2
    numtriangles += (npts - 2);
  }

  // second pass, triangulate and write face indices
  fprintf(this->FilePtr, "\tface_indices {\n");
  fprintf(this->FilePtr, SVTKPOV_CNTFMT, numtriangles);
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    svtkIdType triangle[3];
    // the first triangle
    triangle[0] = pts[0];
    triangle[1] = pts[1];
    triangle[2] = pts[2];

    fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
    if (scalar_visible)
    {
      fprintf(this->FilePtr, SVTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
    }
    else
    {
      fprintf(this->FilePtr, "\n");
    }

    // the rest of triangles
    for (svtkIdType i = 3; i < npts; i++)
    {
      triangle[1] = triangle[2];
      triangle[2] = pts[i];
      fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      if (scalar_visible)
      {
        fprintf(this->FilePtr, SVTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
      }
      else
      {
        fprintf(this->FilePtr, "\n");
      }
    }
  }
  fprintf(this->FilePtr, "\t}\n");

  // third pass, the same thing as 2nd pass but for normal_indices
  if (polys->GetPointData()->GetNormals())
  {
    fprintf(this->FilePtr, "\tnormal_indices {\n");
    fprintf(this->FilePtr, SVTKPOV_CNTFMT, numtriangles);
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      svtkIdType triangle[3];
      // the first triangle
      triangle[0] = pts[0];
      triangle[1] = pts[1];
      triangle[2] = pts[2];

      fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      fprintf(this->FilePtr, "\n");

      // the rest of triangles
      for (svtkIdType i = 3; i < npts; i++)
      {
        triangle[1] = triangle[2];
        triangle[2] = pts[i];
        fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
        fprintf(this->FilePtr, "\n");
      }
    }
    fprintf(this->FilePtr, "\t}\n");
  }

  // TODO: 4th pass, texture indices
}

void svtkPOVExporter::WriteTriangleStrips(svtkPolyData* polys, bool scalar_visible)
{
  // convert triangle strips into triangles
  svtkIdType numtriangles = 0;
  svtkCellArray* cells = polys->GetStrips();
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;

  // first pass, calculate how many triangles there will be after conversion
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    // the number of triangles for each polygon will be # of vertex - 2
    numtriangles += (npts - 2);
  }

  // second pass, convert to triangles and write face indices
  fprintf(this->FilePtr, "\tface_indices {\n");
  fprintf(this->FilePtr, SVTKPOV_CNTFMT, numtriangles);
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    svtkIdType triangle[3];
    // the first triangle
    triangle[0] = pts[0];
    triangle[1] = pts[1];
    triangle[2] = pts[2];

    fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
    if (scalar_visible)
    {
      fprintf(this->FilePtr, SVTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
    }
    else
    {
      fprintf(this->FilePtr, "\n");
    }

    // the rest of triangles
    for (svtkIdType i = 3; i < npts; i++)
    {
      triangle[0] = triangle[1];
      triangle[1] = triangle[2];
      triangle[2] = pts[i];
      fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      if (scalar_visible)
      {
        fprintf(this->FilePtr, SVTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
      }
      else
      {
        fprintf(this->FilePtr, "\n");
      }
    }
  }
  fprintf(this->FilePtr, "\t}\n");

  // third pass, the same thing as 2nd pass but for normal_indices
  if (polys->GetPointData()->GetNormals())
  {
    fprintf(this->FilePtr, "\tnormal_indices {\n");
    fprintf(this->FilePtr, SVTKPOV_CNTFMT, numtriangles);
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      svtkIdType triangle[3];
      // the first triangle
      triangle[0] = pts[0];
      triangle[1] = pts[1];
      triangle[2] = pts[2];

      fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      fprintf(this->FilePtr, "\n");

      // the rest of triangles
      for (svtkIdType i = 3; i < npts; i++)
      {
        triangle[0] = triangle[1];
        triangle[1] = triangle[2];
        triangle[2] = pts[i];
        fprintf(this->FilePtr, SVTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
        fprintf(this->FilePtr, "\n");
      }
    }
    fprintf(this->FilePtr, "\t}\n");
  }

  // TODO: 4th pass, texture indices
}

void svtkPOVExporter::WriteProperty(svtkProperty* property)
{
  fprintf(this->FilePtr, "\ttexture {\n");

  /* write color */
  fprintf(this->FilePtr, "\t\tpigment {\n");
  double* color = property->GetColor();
  fprintf(this->FilePtr, "\t\t\tcolor rgbf <%f, %f, %f %f>\n", color[0], color[1], color[2],
    1.0 - property->GetOpacity());
  fprintf(this->FilePtr, "\t\t}\n");

  /* write ambient, diffuse and specular coefficients */
  fprintf(this->FilePtr, "\t\tfinish {\n\t\t\t");
  fprintf(this->FilePtr, "ambient %f  ", property->GetAmbient());
  fprintf(this->FilePtr, "diffuse %f  ", property->GetDiffuse());
  fprintf(this->FilePtr, "phong %f  ", property->GetSpecular());
  fprintf(this->FilePtr, "phong_size %f  ", property->GetSpecularPower());
  fprintf(this->FilePtr, "\n\t\t}\n");

  fprintf(this->FilePtr, "\t}\n");
}

void svtkPOVExporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}
