/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVRMLExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVRMLExporter.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataSet.h"
#include "svtkGeometryFilter.h"
#include "svtkImageData.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRendererCollection.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"
#include "svtkTransform.h"
#include "svtkUnsignedCharArray.h"
#include <svtksys/SystemTools.hxx>

#include <limits>

namespace
{
// For C format strings
constexpr int max_double_digits = std::numeric_limits<double>::max_digits10;
}

svtkStandardNewMacro(svtkVRMLExporter);

svtkVRMLExporter::svtkVRMLExporter()
{
  this->Speed = 4.0;
  this->FileName = nullptr;
  this->FilePointer = nullptr;
}

svtkVRMLExporter::~svtkVRMLExporter()
{
  delete[] this->FileName;
}

void svtkVRMLExporter::SetFilePointer(FILE* fp)
{
  if (fp != this->FilePointer)
  {
    this->Modified();
    this->FilePointer = fp;
  }
}

void svtkVRMLExporter::WriteData()
{
  svtkActorCollection* ac;
  svtkActor *anActor, *aPart;
  svtkLightCollection* lc;
  svtkLight* aLight;
  svtkCamera* cam;
  double* tempd;
  FILE* fp;

  // make sure the user specified a FileName or FilePointer
  if (!this->FilePointer && (this->FileName == nullptr))
  {
    svtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  // get the renderer
  svtkRenderer* ren = this->ActiveRenderer;
  if (!ren)
  {
    ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    svtkErrorMacro(<< "no actors found for writing VRML file.");
    return;
  }

  // try opening the files
  if (!this->FilePointer)
  {
    fp = svtksys::SystemTools::Fopen(this->FileName, "w");
    if (!fp)
    {
      svtkErrorMacro(<< "unable to open VRML file " << this->FileName);
      return;
    }
  }
  else
  {
    fp = this->FilePointer;
  }

  //
  //  Write header
  //
  svtkDebugMacro("Writing VRML file");
  fprintf(fp, "#VRML V2.0 utf8\n");
  fprintf(fp, "# VRML file written by the visualization toolkit\n\n");

  // Start write the Background
  double background[3];
  ren->GetBackground(background);
  fprintf(fp, "    Background {\n ");
  fprintf(fp, "   skyColor [%f %f %f, ]\n", background[0], background[1], background[2]);
  fprintf(fp, "    }\n ");
  // End of Background

  // do the camera
  cam = ren->GetActiveCamera();
  fprintf(fp, "    Viewpoint\n      {\n      fieldOfView %f\n",
    cam->GetViewAngle() * svtkMath::Pi() / 180.0);
  fprintf(fp, "      position %f %f %f\n", cam->GetPosition()[0], cam->GetPosition()[1],
    cam->GetPosition()[2]);
  fprintf(fp, "      description \"Default View\"\n");
  tempd = cam->GetOrientationWXYZ();
  fprintf(fp, "      orientation %.*g %.*g %.*g %.*g\n      }\n", max_double_digits, tempd[1],
    max_double_digits, tempd[2], max_double_digits, tempd[3], max_double_digits,
    tempd[0] * svtkMath::Pi() / 180.0);

  // do the lights first the ambient then the others
  fprintf(
    fp, "    NavigationInfo {\n      type [\"EXAMINE\",\"FLY\"]\n      speed %f\n", this->Speed);
  if (ren->GetLights()->GetNumberOfItems() == 0)
  {
    fprintf(fp, "      headlight TRUE}\n\n");
  }
  else
  {
    fprintf(fp, "      headlight FALSE}\n\n");
  }
  fprintf(fp, "    DirectionalLight { ambientIntensity 1 intensity 0 # ambient light\n");
  fprintf(fp, "      color %f %f %f }\n\n", ren->GetAmbient()[0], ren->GetAmbient()[1],
    ren->GetAmbient()[2]);

  // make sure we have a default light
  // if we don't then use a headlight
  lc = ren->GetLights();
  svtkCollectionSimpleIterator lsit;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit));)
  {
    this->WriteALight(aLight, fp);
  }

  // do the actors now
  ac = ren->GetActors();
  svtkAssemblyPath* apath;
  svtkCollectionSimpleIterator ait;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
  {
    for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
    {
      aPart = static_cast<svtkActor*>(apath->GetLastNode()->GetViewProp());
      this->WriteAnActor(aPart, fp);
    }
  }
  if (!this->FilePointer)
  {
    fclose(fp);
  }
}

void svtkVRMLExporter::WriteALight(svtkLight* aLight, FILE* fp)
{
  double *pos, *focus, *color;
  double dir[3];

  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  color = aLight->GetDiffuseColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  svtkMath::Normalize(dir);

  if (aLight->GetPositional())
  {
    double* attn;

    if (aLight->GetConeAngle() >= 90.0)
    {
      fprintf(fp, "    PointLight {\n");
    }
    else
    {
      fprintf(fp, "    SpotLight {\n");
      fprintf(fp, "      direction %f %f %f\n", dir[0], dir[1], dir[2]);
      fprintf(fp, "      cutOffAngle %f\n", aLight->GetConeAngle());
    }
    fprintf(fp, "      location %f %f %f\n", pos[0], pos[1], pos[2]);
    attn = aLight->GetAttenuationValues();
    fprintf(fp, "      attenuation %f %f %f\n", attn[0], attn[1], attn[2]);
  }
  else
  {
    fprintf(fp, "    DirectionalLight {\n");
    fprintf(fp, "      direction %f %f %f\n", dir[0], dir[1], dir[2]);
  }

  fprintf(fp, "      color %f %f %f\n", color[0], color[1], color[2]);
  fprintf(fp, "      intensity %f\n", aLight->GetIntensity());
  if (aLight->GetSwitch())
  {
    fprintf(fp, "      on TRUE\n      }\n");
  }
  else
  {
    fprintf(fp, "      on FALSE\n      }\n");
  }
}

void svtkVRMLExporter::WriteAnActor(svtkActor* anActor, FILE* fp)
{
  svtkSmartPointer<svtkPolyData> pd;
  svtkPointData* pntData;
  svtkPoints* points;
  svtkDataArray* normals = nullptr;
  svtkDataArray* tcoords = nullptr;
  int i, i1, i2;
  double* tempd;
  svtkCellArray* cells;
  svtkIdType npts = 0;
  const svtkIdType* indx = nullptr;
  int pointDataWritten = 0;
  svtkPolyDataMapper* pm;
  svtkUnsignedCharArray* colors;
  double* p;
  unsigned char* c;
  svtkTransform* trans;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == nullptr)
  {
    return;
  }
  if (anActor->GetVisibility() == 0)
  {
    return;
  }

  // Before putting out anything in the file, ensure that we have an exportable
  // dataset being rendered by the actor.
  svtkDataObject* inputDO = anActor->GetMapper()->GetInputDataObject(0, 0);
  if (inputDO == nullptr)
  {
    return;
  }

  // we really want polydata, so apply geometry filter, if needed.
  if (inputDO->IsA("svtkCompositeDataSet"))
  {
    svtkCompositeDataGeometryFilter* gf = svtkCompositeDataGeometryFilter::New();
    gf->SetInputConnection(anActor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    pd = gf->GetOutput();
    gf->Delete();
  }
  else if (inputDO->GetDataObjectType() != SVTK_POLY_DATA)
  {
    svtkGeometryFilter* gf = svtkGeometryFilter::New();
    gf->SetInputConnection(anActor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    pd = gf->GetOutput();
    gf->Delete();
  }
  else
  {
    anActor->GetMapper()->Update();
    pd = static_cast<svtkPolyData*>(inputDO);
  }

  if (pd == nullptr || pd->GetNumberOfPoints() == 0)
  {
    return;
  }

  // first stuff out the transform
  trans = svtkTransform::New();
  trans->SetMatrix(anActor->svtkProp3D::GetMatrix());

  fprintf(fp, "    Transform {\n");
  tempd = trans->GetPosition();
  fprintf(fp, "      translation %.*g %.*g %.*g\n", max_double_digits, tempd[0], max_double_digits,
    tempd[1], max_double_digits, tempd[2]);
  tempd = trans->GetOrientationWXYZ();
  fprintf(fp, "      rotation %.*g %.*g %.*g %.*g\n", max_double_digits, tempd[1],
    max_double_digits, tempd[2], max_double_digits, tempd[3], max_double_digits,
    tempd[0] * svtkMath::Pi() / 180.0);
  tempd = trans->GetScale();
  fprintf(fp, "      scale %.*g %.*g %.*g\n", max_double_digits, tempd[0], max_double_digits,
    tempd[1], max_double_digits, tempd[2]);
  fprintf(fp, "      children [\n");
  trans->Delete();

  pm = svtkPolyDataMapper::New();
  pm->SetInputData(pd);
  pm->SetScalarRange(anActor->GetMapper()->GetScalarRange());
  pm->SetScalarVisibility(anActor->GetMapper()->GetScalarVisibility());
  pm->SetLookupTable(anActor->GetMapper()->GetLookupTable());
  pm->SetScalarMode(anActor->GetMapper()->GetScalarMode());

  if (pm->GetScalarMode() == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
    pm->GetScalarMode() == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (anActor->GetMapper()->GetArrayAccessMode() == SVTK_GET_ARRAY_BY_ID)
    {
      pm->ColorByArrayComponent(
        anActor->GetMapper()->GetArrayId(), anActor->GetMapper()->GetArrayComponent());
    }
    else
    {
      pm->ColorByArrayComponent(
        anActor->GetMapper()->GetArrayName(), anActor->GetMapper()->GetArrayComponent());
    }
  }

  points = pd->GetPoints();
  pntData = pd->GetPointData();
  normals = pntData->GetNormals();
  tcoords = pntData->GetTCoords();
  colors = pm->MapScalars(1.0);

  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
  {
    WriteShapeBegin(anActor, fp, pd, pntData, colors);
    fprintf(fp, "          geometry IndexedFaceSet {\n");
    // two sided lighting ? for now assume it is on
    fprintf(fp, "            solid FALSE\n");
    if (!pointDataWritten)
    {
      this->WritePointData(points, normals, tcoords, colors, fp);
      pointDataWritten = 1;
    }
    else
    {
      fprintf(fp, "            coord  USE SVTKcoordinates\n");
      if (normals)
      {
        fprintf(fp, "            normal  USE SVTKnormals\n");
      }
      if (tcoords)
      {
        fprintf(fp, "            texCoord  USE SVTKtcoords\n");
      }
      if (colors)
      {
        fprintf(fp, "            color  USE SVTKcolors\n");
      }
    }

    fprintf(fp, "            coordIndex  [\n");

    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fprintf(fp, "              ");
      for (i = 0; i < npts; i++)
      {
        // treating svtkIdType as int
        fprintf(fp, "%i, ", static_cast<int>(indx[i]));
      }
      fprintf(fp, "-1,\n");
    }
    fprintf(fp, "            ]\n");
    fprintf(fp, "          }\n");
    WriteShapeEnd(fp);
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
  {
    WriteShapeBegin(anActor, fp, pd, pntData, colors);
    fprintf(fp, "          geometry IndexedFaceSet {\n");
    if (!pointDataWritten)
    {
      this->WritePointData(points, normals, tcoords, colors, fp);
      pointDataWritten = 1;
    }
    else
    {
      fprintf(fp, "            coord  USE SVTKcoordinates\n");
      if (normals)
      {
        fprintf(fp, "            normal  USE SVTKnormals\n");
      }
      if (tcoords)
      {
        fprintf(fp, "            texCoord  USE SVTKtcoords\n");
      }
      if (colors)
      {
        fprintf(fp, "            color  USE SVTKcolors\n");
      }
    }
    fprintf(fp, "            coordIndex  [\n");
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      for (i = 2; i < npts; i++)
      {
        if (i % 2)
        {
          i1 = i - 1;
          i2 = i - 2;
        }
        else
        {
          i1 = i - 2;
          i2 = i - 1;
        }
        // treating svtkIdType as int
        fprintf(fp, "              %i, %i, %i, -1,\n", static_cast<int>(indx[i1]),
          static_cast<int>(indx[i2]), static_cast<int>(indx[i]));
      }
    }
    fprintf(fp, "            ]\n");
    fprintf(fp, "          }\n");
    WriteShapeEnd(fp);
  }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
  {
    WriteShapeBegin(anActor, fp, pd, pntData, colors);
    fprintf(fp, "          geometry IndexedLineSet {\n");
    if (!pointDataWritten)
    {
      this->WritePointData(points, nullptr, nullptr, colors, fp);
      pointDataWritten = 1;
    }
    else
    {
      fprintf(fp, "            coord  USE SVTKcoordinates\n");
      if (colors)
      {
        fprintf(fp, "            color  USE SVTKcolors\n");
      }
    }

    fprintf(fp, "            coordIndex  [\n");

    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fprintf(fp, "              ");
      for (i = 0; i < npts; i++)
      {
        // treating svtkIdType as int
        fprintf(fp, "%i, ", static_cast<int>(indx[i]));
      }
      fprintf(fp, "-1,\n");
    }
    fprintf(fp, "            ]\n");
    fprintf(fp, "          }\n");
    WriteShapeEnd(fp);
  }

  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
  {
    WriteShapeBegin(anActor, fp, pd, pntData, colors);
    fprintf(fp, "          geometry PointSet {\n");
    cells = pd->GetVerts();
    fprintf(fp, "            coord Coordinate {");
    fprintf(fp, "              point [");
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fprintf(fp, "              ");
      for (i = 0; i < npts; i++)
      {
        p = points->GetPoint(indx[i]);
        fprintf(fp, "              %.*g %.*g %.*g,\n", max_double_digits, p[0], max_double_digits,
          p[1], max_double_digits, p[2]);
      }
    }
    fprintf(fp, "              ]\n");
    fprintf(fp, "            }\n");
    if (colors)
    {
      fprintf(fp, "            color Color {");
      fprintf(fp, "              color [");
      for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
      {
        fprintf(fp, "              ");
        for (i = 0; i < npts; i++)
        {
          c = colors->GetPointer(4 * indx[i]);
          fprintf(fp, "           %.*g %.*g %.*g,\n", max_double_digits, c[0] / 255.0,
            max_double_digits, c[1] / 255.0, max_double_digits, c[2] / 255.0);
        }
      }
      fprintf(fp, "              ]\n");
      fprintf(fp, "            }\n");
    }
    fprintf(fp, "          }\n");
    WriteShapeEnd(fp);
  }

  fprintf(fp, "      ]\n"); // close the original transforms children
  fprintf(fp, "    }\n");   // close the original transform

  pm->Delete();
}

void svtkVRMLExporter::WriteShapeBegin(svtkActor* actor, FILE* fileP, svtkPolyData* polyData,
  svtkPointData* pntData, svtkUnsignedCharArray* color)
{
  double* tempd;
  double tempf2;

  fprintf(fileP, "        Shape {\n");
  svtkProperty* props = actor->GetProperty();
  // write out the material properties to the mat file
  fprintf(fileP, "          appearance Appearance {\n");
  fprintf(fileP, "            material Material {\n");
  fprintf(fileP, "              ambientIntensity %.*g\n", max_double_digits, props->GetAmbient());
  // if we don't have colors and we have only lines & points
  // use emissive to color them
  if (!(pntData->GetNormals() || color || polyData->GetNumberOfPolys() ||
        polyData->GetNumberOfStrips()))
  {
    tempf2 = props->GetAmbient();
    tempd = props->GetAmbientColor();
    fprintf(fileP, "              emissiveColor %.*g %.*g %.*g\n", max_double_digits,
      tempd[0] * tempf2, max_double_digits, tempd[1] * tempf2, max_double_digits,
      tempd[2] * tempf2);
  }
  tempf2 = props->GetDiffuse();
  tempd = props->GetDiffuseColor();
  fprintf(fileP, "              diffuseColor %.*g %.*g %.*g\n", max_double_digits,
    tempd[0] * tempf2, max_double_digits, tempd[1] * tempf2, max_double_digits, tempd[2] * tempf2);
  tempf2 = props->GetSpecular();
  tempd = props->GetSpecularColor();
  fprintf(fileP, "              specularColor %.*g %.*g %.*g\n", max_double_digits,
    tempd[0] * tempf2, max_double_digits, tempd[1] * tempf2, max_double_digits, tempd[2] * tempf2);
  fprintf(
    fileP, "              shininess %.*g\n", max_double_digits, props->GetSpecularPower() / 128.0);
  fprintf(fileP, "              transparency %.*g\n", max_double_digits, 1.0 - props->GetOpacity());
  fprintf(fileP, "              }\n"); // close matrial

  // is there a texture map
  if (actor->GetTexture())
  {
    svtkTexture* aTexture = actor->GetTexture();
    int *size, xsize, ysize, bpp;
    svtkDataArray* scalars;
    svtkDataArray* mappedScalars;
    unsigned char* txtrData;

    // make sure it is updated and then get some info
    if (aTexture->GetInput() == nullptr)
    {
      svtkErrorMacro(<< "texture has no input!\n");
      return;
    }
    aTexture->GetInputAlgorithm()->Update();
    size = aTexture->GetInput()->GetDimensions();
    scalars = aTexture->GetInput()->GetPointData()->GetScalars();

    // make sure scalars are non null
    if (!scalars)
    {
      svtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
    }

    // make sure using unsigned char data of color scalars type
    if (aTexture->GetColorMode() == SVTK_COLOR_MODE_MAP_SCALARS ||
      (scalars->GetDataType() != SVTK_UNSIGNED_CHAR))
    {
      mappedScalars = aTexture->GetMappedScalars();
    }
    else
    {
      mappedScalars = scalars;
    }

    // we only support 2d texture maps right now
    // so one of the three sizes must be 1, but it
    // could be any of them, so lets find it
    if (size[0] == 1)
    {
      xsize = size[1];
      ysize = size[2];
    }
    else
    {
      xsize = size[0];
      if (size[1] == 1)
      {
        ysize = size[2];
      }
      else
      {
        ysize = size[1];
        if (size[2] != 1)
        {
          svtkErrorMacro(<< "3D texture maps currently are not supported!\n");
          return;
        }
      }
    }

    fprintf(fileP, "            texture PixelTexture {\n");
    bpp = mappedScalars->GetNumberOfComponents();
    fprintf(fileP, "              image %i %i %i\n", xsize, ysize, bpp);
    txtrData = static_cast<svtkUnsignedCharArray*>(mappedScalars)->GetPointer(0);
    int totalValues = xsize * ysize;
    for (int i = 0; i < totalValues; i++)
    {
      fprintf(fileP, "0x%.2x", *txtrData);
      txtrData++;
      if (bpp > 1)
      {
        fprintf(fileP, "%.2x", *txtrData);
        txtrData++;
      }
      if (bpp > 2)
      {
        fprintf(fileP, "%.2x", *txtrData);
        txtrData++;
      }
      if (bpp > 3)
      {
        fprintf(fileP, "%.2x", *txtrData);
        txtrData++;
      }
      if (i % 8 == 0)
      {
        fprintf(fileP, "\n");
      }
      else
      {
        fprintf(fileP, " ");
      }
    }
    if (!(aTexture->GetRepeat()))
    {
      fprintf(fileP, "              repeatS FALSE\n");
      fprintf(fileP, "              repeatT FALSE\n");
    }
    fprintf(fileP, "              }\n"); // close texture
  }
  fprintf(fileP, "            }\n"); // close appearance
}

void svtkVRMLExporter::WriteShapeEnd(FILE* fileP)
{
  fprintf(fileP, "        }\n"); // close the  Shape
}

void svtkVRMLExporter::WritePointData(svtkPoints* points, svtkDataArray* normals,
  svtkDataArray* tcoords, svtkUnsignedCharArray* colors, FILE* fp)
{
  double* p;
  int i;
  unsigned char* c;

  // write out the points
  fprintf(fp, "            coord DEF SVTKcoordinates Coordinate {\n");
  fprintf(fp, "              point [\n");
  for (i = 0; i < points->GetNumberOfPoints(); i++)
  {
    p = points->GetPoint(i);
    fprintf(fp, "              %.*g %.*g %.*g,\n", max_double_digits, p[0], max_double_digits, p[1],
      max_double_digits, p[2]);
  }
  fprintf(fp, "              ]\n");
  fprintf(fp, "            }\n");

  // write out the point data
  if (normals)
  {
    fprintf(fp, "            normal DEF SVTKnormals Normal {\n");
    fprintf(fp, "              vector [\n");
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
    {
      p = normals->GetTuple(i);
      fprintf(fp, "           %.*g %.*g %.*g,\n", max_double_digits, p[0], max_double_digits, p[1],
        max_double_digits, p[2]);
    }
    fprintf(fp, "            ]\n");
    fprintf(fp, "          }\n");
  }

  // write out the point data
  if (tcoords)
  {
    fprintf(fp, "            texCoord DEF SVTKtcoords TextureCoordinate {\n");
    fprintf(fp, "              point [\n");
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
    {
      p = tcoords->GetTuple(i);
      fprintf(fp, "           %.*g %.*g,\n", max_double_digits, p[0], max_double_digits, p[1]);
    }
    fprintf(fp, "            ]\n");
    fprintf(fp, "          }\n");
  }

  // write out the point data
  if (colors)
  {
    fprintf(fp, "            color DEF SVTKcolors Color {\n");
    fprintf(fp, "              color [\n");
    for (i = 0; i < colors->GetNumberOfTuples(); i++)
    {
      c = colors->GetPointer(4 * i);
      fprintf(fp, "           %.*g %.*g %.*g,\n", max_double_digits, c[0] / 255.0,
        max_double_digits, c[1] / 255.0, max_double_digits, c[2] / 255.0);
    }
    fprintf(fp, "            ]\n");
    fprintf(fp, "          }\n");
  }
}

void svtkVRMLExporter::PrintSelf(ostream& os, svtkIndent indent)
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

  os << indent << "Speed: " << this->Speed << "\n";
}
