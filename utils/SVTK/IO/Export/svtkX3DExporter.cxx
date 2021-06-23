/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkX3DExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Kristian Sons
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkX3DExporter.h"

#include "svtkActor2D.h"
#include "svtkActor2DCollection.h"
#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkGeometryFilter.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRendererCollection.h"
#include "svtkSmartPointer.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkTexture.h"
#include "svtkTransform.h"
#include "svtkUnsignedCharArray.h"
#include "svtkX3D.h"
#include "svtkX3DExporterFIWriter.h"
#include "svtkX3DExporterXMLWriter.h"

#include <cassert>
#include <sstream>

using namespace svtkX3D;

// forward declarations
static bool svtkX3DExporterWriterUsingCellColors(svtkMapper* anActor);
static bool svtkX3DExporterWriterRenderFaceSet(int cellType, int representation, svtkPoints* points,
  svtkIdType cellOffset, svtkCellArray* cells, svtkUnsignedCharArray* colors, bool cell_colors,
  svtkDataArray* normals, bool cell_normals, svtkDataArray* tcoords, bool common_data_written,
  int index, svtkX3DExporterWriter* writer);
static void svtkX3DExporterWriteData(svtkPoints* points, svtkDataArray* normals, svtkDataArray* tcoords,
  svtkUnsignedCharArray* colors, int index, svtkX3DExporterWriter* writer);
static void svtkX3DExporterUseData(
  bool normals, bool tcoords, bool colors, int index, svtkX3DExporterWriter* writer);
static bool svtkX3DExporterWriterRenderVerts(svtkPoints* points, svtkCellArray* cells,
  svtkUnsignedCharArray* colors, bool cell_colors, svtkX3DExporterWriter* writer);
static bool svtkX3DExporterWriterRenderPoints(
  svtkPolyData* pd, svtkUnsignedCharArray* colors, bool cell_colors, svtkX3DExporterWriter* writer);

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkX3DExporter);

//----------------------------------------------------------------------------
svtkX3DExporter::svtkX3DExporter()
{
  this->Speed = 4.0;
  this->FileName = nullptr;
  this->Binary = 0;
  this->Fastest = 0;
  this->WriteToOutputString = 0;
  this->OutputString = nullptr;
  this->OutputStringLength = 0;
}

//----------------------------------------------------------------------------
svtkX3DExporter::~svtkX3DExporter()
{
  this->SetFileName(nullptr);
  delete[] this->OutputString;
}

//----------------------------------------------------------------------------
void svtkX3DExporter::WriteData()
{
  svtkSmartPointer<svtkX3DExporterWriter> writer;
  svtkActorCollection* ac;
  svtkActor2DCollection* a2Dc;
  svtkActor *anActor, *aPart;
  svtkActor2D *anTextActor2D, *aPart2D;
  svtkLightCollection* lc;
  svtkLight* aLight;
  svtkCamera* cam;

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr && (!this->WriteToOutputString))
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
    svtkErrorMacro(<< "no actors found for writing X3D file.");
    return;
  }

  // try opening the files
  if (this->Binary)
  {
    svtkX3DExporterFIWriter* temp = svtkX3DExporterFIWriter::New();
    temp->SetFastest(this->GetFastest());
    writer.TakeReference(temp);
  }
  else
  {
    writer = svtkSmartPointer<svtkX3DExporterXMLWriter>::New();
  }

  if (this->WriteToOutputString)
  {
    if (!writer->OpenStream())
    {
      svtkErrorMacro(<< "unable to open X3D stream");
      return;
    }
  }
  else
  {
    if (!writer->OpenFile(this->FileName))
    {
      svtkErrorMacro(<< "unable to open X3D file " << this->FileName);
      return;
    }
  }

  //
  //  Write header
  //
  svtkDebugMacro("Writing X3D file");

  writer->StartDocument();

  writer->StartNode(X3D);
  writer->SetField(profile, "Immersive");
  writer->SetField(svtkX3D::version, "3.0");

  writer->StartNode(head);

  writer->StartNode(meta);
  writer->SetField(name, "filename");
  writer->SetField(content, this->FileName ? this->FileName : "Stream");
  writer->EndNode();

  writer->StartNode(meta);
  writer->SetField(name, "generator");
  writer->SetField(content, "Visualization ToolKit X3D exporter v0.9.1");
  writer->EndNode();

  writer->StartNode(meta);
  writer->SetField(name, "numberofelements");
  std::ostringstream ss;
  ss << ren->GetActors()->GetNumberOfItems();
  writer->SetField(content, ss.str().c_str());
  writer->EndNode();

  writer->EndNode(); // head

  writer->StartNode(Scene);

  // Start write the Background
  writer->StartNode(Background);
  writer->SetField(skyColor, SFVEC3F, ren->GetBackground());
  writer->EndNode();
  // End of Background

  // Start write the Camera
  cam = ren->GetActiveCamera();
  writer->StartNode(Viewpoint);
  writer->SetField(
    fieldOfView, static_cast<float>(svtkMath::RadiansFromDegrees(cam->GetViewAngle())));
  writer->SetField(position, SFVEC3F, cam->GetPosition());
  writer->SetField(description, "Default View");
  writer->SetField(orientation, SFROTATION, cam->GetOrientationWXYZ());
  writer->SetField(centerOfRotation, SFVEC3F, cam->GetFocalPoint());
  writer->EndNode();
  // End of Camera

  // do the lights first the ambient then the others
  writer->StartNode(NavigationInfo);
  writer->SetField(type, "\"EXAMINE\" \"FLY\" \"ANY\"", true);
  writer->SetField(speed, static_cast<float>(this->Speed));
  writer->SetField(headlight, this->HasHeadLight(ren) ? true : false);
  writer->EndNode();

  writer->StartNode(DirectionalLight);
  writer->SetField(ambientIntensity, 1.0f);
  writer->SetField(intensity, 0.0f);
  writer->SetField(color, SFCOLOR, ren->GetAmbient());
  writer->EndNode();

  // label ROOT
  static double n[] = { 0.0, 0.0, 0.0 };
  writer->StartNode(Transform);
  writer->SetField(DEF, "ROOT");
  writer->SetField(translation, SFVEC3F, n);

  // make sure we have a default light
  // if we don't then use a headlight
  lc = ren->GetLights();
  svtkCollectionSimpleIterator lsit;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit));)
  {
    if (!aLight->LightTypeIsHeadlight())
    {
      this->WriteALight(aLight, writer);
    }
  }

  // do the actors now
  ac = ren->GetActors();
  svtkAssemblyPath* apath;
  svtkCollectionSimpleIterator ait;
  int index = 0;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
  {
    for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
    {
      if (anActor->GetVisibility() != 0)
      {
        aPart = static_cast<svtkActor*>(apath->GetLastNode()->GetViewProp());
        this->WriteAnActor(aPart, writer, index);
        index++;
      }
    }
  }
  writer->EndNode(); // ROOT Transform

  //////////////////////////////////////////////
  // do the 2D actors now
  a2Dc = ren->GetActors2D();

  if (a2Dc->GetNumberOfItems() != 0)
  {
    static double s[] = { 1000000.0, 1000000.0, 1000000.0 };
    writer->StartNode(ProximitySensor);
    writer->SetField(DEF, "PROX_LABEL");
    writer->SetField(size, SFVEC3F, s);
    writer->EndNode();

    // disable collision for the text annotations
    writer->StartNode(Collision);
    writer->SetField(enabled, false);

    // add a Label TRANS_LABEL for the text annotations and the sensor
    writer->StartNode(Transform);
    writer->SetField(DEF, "TRANS_LABEL");

    svtkAssemblyPath* apath2D;
    svtkCollectionSimpleIterator ait2D;
    for (a2Dc->InitTraversal(ait2D); (anTextActor2D = a2Dc->GetNextActor2D(ait2D));)
    {

      for (anTextActor2D->InitPathTraversal(); (apath2D = anTextActor2D->GetNextPath());)
      {
        aPart2D = static_cast<svtkActor2D*>(apath2D->GetLastNode()->GetViewProp());
        this->WriteATextActor2D(aPart2D, writer);
      }
    }
    writer->EndNode(); // Transform
    writer->EndNode(); // Collision

    writer->StartNode(ROUTE);
    writer->SetField(fromNode, "PROX_LABEL");
    writer->SetField(fromField, "position_changed");
    writer->SetField(toNode, "TRANS_LABEL");
    writer->SetField(toField, "set_translation");
    writer->EndNode(); // Route

    writer->StartNode(ROUTE);
    writer->SetField(fromNode, "PROX_LABEL");
    writer->SetField(fromField, "orientation_changed");
    writer->SetField(toNode, "TRANS_LABEL");
    writer->SetField(toField, "set_rotation");
    writer->EndNode(); // Route
  }
  /////////////////////////////////////////////////

  this->WriteAdditionalNodes(writer);

  writer->EndNode(); // Scene
  writer->EndNode(); // X3D
  writer->Flush();
  writer->EndDocument();
  writer->CloseFile();

  if (this->WriteToOutputString)
  {
    this->OutputStringLength = writer->GetOutputStringLength();
    this->OutputString = writer->RegisterAndGetOutputString();
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporter::WriteALight(svtkLight* aLight, svtkX3DExporterWriter* writer)
{
  double *pos, *focus, *colord;
  double dir[3];

  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  colord = aLight->GetDiffuseColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  svtkMath::Normalize(dir);

  if (aLight->GetPositional())
  {
    if (aLight->GetConeAngle() >= 90.0)
    {
      writer->StartNode(PointLight);
    }
    else
    {
      writer->StartNode(SpotLight);
      writer->SetField(direction, SFVEC3F, dir);
      writer->SetField(cutOffAngle, static_cast<float>(aLight->GetConeAngle()));
    }
    writer->SetField(location, SFVEC3F, pos);
    writer->SetField(attenuation, SFVEC3F, aLight->GetAttenuationValues());
  }
  else
  {
    writer->StartNode(DirectionalLight);
    writer->SetField(direction, SFVEC3F, dir);
  }

  // TODO: Check correct color
  writer->SetField(color, SFCOLOR, colord);
  writer->SetField(intensity, static_cast<float>(aLight->GetIntensity()));
  writer->SetField(on, aLight->GetSwitch() ? true : false);
  writer->EndNode();
  writer->Flush();
}

//----------------------------------------------------------------------------
void svtkX3DExporter::WriteAnActor(svtkActor* anActor, svtkX3DExporterWriter* writer, int index)
{
  // see if the actor has a mapper. it could be an assembly
  svtkMapper* mapper = anActor->GetMapper();
  if (mapper == nullptr)
  {
    return;
  }
  mapper->Update();

  // validate mapper input dataset.
  svtkDataObject* dObj = mapper->GetInputDataObject(0, 0);
  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(dObj);
  svtkPolyData* pd = svtkPolyData::SafeDownCast(dObj);
  if (cd == nullptr && pd == nullptr)
  {
    // we don't support the input dataset or is empty.
    return;
  }

  // first stuff out the transform
  svtkSmartPointer<svtkTransform> trans = svtkSmartPointer<svtkTransform>::New();
  trans->SetMatrix(anActor->svtkProp3D::GetMatrix());

  writer->StartNode(Transform);
  writer->SetField(translation, SFVEC3F, trans->GetPosition());
  writer->SetField(rotation, SFROTATION, trans->GetOrientationWXYZ());
  writer->SetField(scale, SFVEC3F, trans->GetScale());

  if (cd)
  {
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (svtkPolyData* currentPD = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject()))
      {
        writer->StartNode(Group);
        if (iter->HasCurrentMetaData() &&
          iter->GetCurrentMetaData()->Has(svtkCompositeDataSet::NAME()))
        {
          if (const char* aname = iter->GetCurrentMetaData()->Get(svtkCompositeDataSet::NAME()))
          {
            std::string mfname = "\"" + std::string(aname) + "\"";
            writer->StartNode(MetadataString);
            writer->SetField(name, "name", false);
            writer->SetField(value, mfname.c_str(), true);
            writer->EndNode();
          }
        }
        this->WriteAPiece(currentPD, anActor, writer, index);
        writer->EndNode(); // close the Group.
      }
    }
  }
  else
  {
    assert(pd != nullptr);
    this->WriteAPiece(pd, anActor, writer, index);
  }
  writer->EndNode();
}

//----------------------------------------------------------------------------
void svtkX3DExporter::WriteAPiece(
  svtkPolyData* pd, svtkActor* anActor, svtkX3DExporterWriter* writer, int index)
{
  svtkPointData* pntData;
  svtkCellData* cellData;
  svtkPoints* points;
  svtkDataArray* normals = nullptr;
  svtkDataArray* tcoords = nullptr;
  svtkProperty* prop;
  svtkUnsignedCharArray* colors;
  svtkSmartPointer<svtkTransform> trans;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == nullptr)
  {
    return;
  }

  if (pd == nullptr)
  {
    return;
  }

  // Create a temporary poly-data mapper that we use.
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();

  mapper->SetInputData(pd);
  mapper->SetScalarRange(anActor->GetMapper()->GetScalarRange());
  mapper->SetScalarVisibility(anActor->GetMapper()->GetScalarVisibility());
  mapper->SetLookupTable(anActor->GetMapper()->GetLookupTable());
  mapper->SetScalarMode(anActor->GetMapper()->GetScalarMode());

  // Essential to turn of interpolate scalars otherwise GetScalars() may return
  // nullptr. We restore value before returning.
  mapper->SetInterpolateScalarsBeforeMapping(0);
  if (mapper->GetScalarMode() == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
    mapper->GetScalarMode() == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (anActor->GetMapper()->GetArrayAccessMode() == SVTK_GET_ARRAY_BY_ID)
    {
      mapper->ColorByArrayComponent(
        anActor->GetMapper()->GetArrayId(), anActor->GetMapper()->GetArrayComponent());
    }
    else
    {
      mapper->ColorByArrayComponent(
        anActor->GetMapper()->GetArrayName(), anActor->GetMapper()->GetArrayComponent());
    }
  }

  prop = anActor->GetProperty();
  points = pd->GetPoints();
  pntData = pd->GetPointData();
  tcoords = pntData->GetTCoords();
  cellData = pd->GetCellData();

  colors = mapper->MapScalars(255.0);

  // Are we using cell colors? Pass the temporary mapper we created here since
  // we're assured that that mapper only has svtkPolyData as input and hence
  // don't run into issue when dealing with composite datasets.
  bool cell_colors = svtkX3DExporterWriterUsingCellColors(mapper);

  normals = pntData->GetNormals();

  // Are we using cell normals.
  bool cell_normals = false;
  if (prop->GetInterpolation() == SVTK_FLAT || !normals)
  {
    // use cell normals, if any.
    normals = cellData->GetNormals();
    cell_normals = true;
  }

  // if we don't have colors and we have only lines & points
  // use emissive to color them
  bool writeEmissiveColor =
    !(normals || colors || pd->GetNumberOfPolys() || pd->GetNumberOfStrips());

  // write out the material properties to the mat file
  int representation = prop->GetRepresentation();

  if (representation == SVTK_POINTS)
  {
    // If representation is points, then we don't have to render different cell
    // types in separate shapes, since the cells type no longer matter.
    if (true)
    {
      writer->StartNode(Shape);
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      svtkX3DExporterWriterRenderPoints(pd, colors, cell_colors, writer);
      writer->EndNode();
    }
  }
  else
  {
    // When rendering as lines or surface, we need to respect the cell
    // structure. This requires rendering polys, tstrips, lines, verts in
    // separate shapes.
    svtkCellArray* verts = pd->GetVerts();
    svtkCellArray* lines = pd->GetLines();
    svtkCellArray* polys = pd->GetPolys();
    svtkCellArray* tstrips = pd->GetStrips();

    svtkIdType numVerts = verts->GetNumberOfCells();
    svtkIdType numLines = lines->GetNumberOfCells();
    svtkIdType numPolys = polys->GetNumberOfCells();
    svtkIdType numStrips = tstrips->GetNumberOfCells();

    bool common_data_written = false;
    if (numPolys > 0)
    {
      writer->StartNode(Shape);
      // Write Appearance
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      // Write Geometry
      svtkX3DExporterWriterRenderFaceSet(SVTK_POLYGON, representation, points, (numVerts + numLines),
        polys, colors, cell_colors, normals, cell_normals, tcoords, common_data_written, index,
        writer);
      writer->EndNode(); // close the  Shape
      common_data_written = true;
    }

    if (numStrips > 0)
    {
      writer->StartNode(Shape);
      // Write Appearance
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      // Write Geometry
      svtkX3DExporterWriterRenderFaceSet(SVTK_TRIANGLE_STRIP, representation, points,
        (numVerts + numLines + numPolys), tstrips, colors, cell_colors, normals, cell_normals,
        tcoords, common_data_written, index, writer);
      writer->EndNode(); // close the  Shape
      common_data_written = true;
    }

    if (numLines > 0)
    {
      writer->StartNode(Shape);
      // Write Appearance
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      // Write Geometry
      svtkX3DExporterWriterRenderFaceSet(SVTK_POLY_LINE,
        (representation == SVTK_SURFACE ? SVTK_WIREFRAME : representation), points, (numVerts), lines,
        colors, cell_colors, normals, cell_normals, tcoords, common_data_written, index, writer);
      writer->EndNode(); // close the  Shape
      common_data_written = true;
    }

    if (numVerts > 0)
    {
      writer->StartNode(Shape);
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      svtkX3DExporterWriterRenderVerts(points, verts, colors, cell_normals, writer);
      writer->EndNode(); // close the  Shape
    }
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporter::PrintSelf(ostream& os, svtkIndent indent)
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
  os << indent << "Binary: " << this->Binary << "\n";
  os << indent << "Fastest: " << this->Fastest << endl;

  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "On" : "Off")
     << std::endl;
  os << indent << "OutputStringLength: " << this->OutputStringLength << std::endl;
  if (this->OutputString)
  {
    os << indent << "OutputString: " << this->OutputString << std::endl;
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporter::WriteATextActor2D(svtkActor2D* anTextActor2D, svtkX3DExporterWriter* writer)
{
  char* ds;
  svtkTextActor* ta;
  svtkTextProperty* tp;

  if (!anTextActor2D->IsA("svtkTextActor"))
  {
    return;
  }

  ta = static_cast<svtkTextActor*>(anTextActor2D);
  tp = ta->GetTextProperty();
  ds = ta->GetInput();

  if (ds == nullptr)
  {
    return;
  }

  double temp[3];

  writer->StartNode(Transform);
  temp[0] = ((ta->GetPosition()[0]) / (this->RenderWindow->GetSize()[0])) - 0.5;
  temp[1] = ((ta->GetPosition()[1]) / (this->RenderWindow->GetSize()[1])) - 0.5;
  temp[2] = -2.0;
  writer->SetField(translation, SFVEC3F, temp);
  temp[0] = temp[1] = temp[2] = 0.002;
  writer->SetField(scale, SFVEC3F, temp);

  writer->StartNode(Shape);

  writer->StartNode(Appearance);

  writer->StartNode(Material);
  temp[0] = 0.0;
  temp[1] = 0.0;
  temp[2] = 1.0;
  writer->SetField(diffuseColor, SFCOLOR, temp);
  tp->GetColor(temp);
  writer->SetField(emissiveColor, SFCOLOR, temp);
  writer->EndNode(); // Material

  writer->EndNode(); // Appearance

  writer->StartNode(Text);
  writer->SetField(svtkX3D::string, ds);

  std::string familyStr;
  switch (tp->GetFontFamily())
  {
    case 0:
    default:
      familyStr = "\"SANS\"";
      break;
    case 1:
      familyStr = "\"TYPEWRITER\"";
      break;
    case 2:
      familyStr = "\"SERIF\"";
      break;
  }

  std::string justifyStr;
  switch (tp->GetJustification())
  {
    case 0:
    default:
      justifyStr += "\"BEGIN\"";
      break;
    case 2:
      justifyStr += "\"END\"";
      break;
  }

  justifyStr += " \"BEGIN\"";

  writer->StartNode(FontStyle);
  writer->SetField(family, familyStr.c_str(), true);
  writer->SetField(topToBottom, tp->GetVerticalJustification() == 2);
  writer->SetField(justify, justifyStr.c_str(), true);
  writer->SetField(size, tp->GetFontSize());
  writer->EndNode(); // FontStyle
  writer->EndNode(); // Text
  writer->EndNode(); // Shape
  writer->EndNode(); // Transform
}

void svtkX3DExporter::WriteAnAppearance(
  svtkActor* anActor, bool emissive, svtkX3DExporterWriter* writer)
{
  double tempd[3];
  double tempf2;

  svtkProperty* prop = anActor->GetProperty();

  writer->StartNode(Appearance);
  writer->StartNode(Material);
  writer->SetField(ambientIntensity, static_cast<float>(prop->GetAmbient()));

  if (emissive)
  {
    tempf2 = prop->GetAmbient();
    prop->GetAmbientColor(tempd);
    tempd[0] *= tempf2;
    tempd[1] *= tempf2;
    tempd[2] *= tempf2;
  }
  else
  {
    tempd[0] = tempd[1] = tempd[2] = 0.0f;
  }
  writer->SetField(emissiveColor, SFCOLOR, tempd);

  // Set diffuse color
  tempf2 = prop->GetDiffuse();
  prop->GetDiffuseColor(tempd);
  tempd[0] *= tempf2;
  tempd[1] *= tempf2;
  tempd[2] *= tempf2;
  writer->SetField(diffuseColor, SFCOLOR, tempd);

  // Set specular color
  tempf2 = prop->GetSpecular();
  prop->GetSpecularColor(tempd);
  tempd[0] *= tempf2;
  tempd[1] *= tempf2;
  tempd[2] *= tempf2;
  writer->SetField(specularColor, SFCOLOR, tempd);

  // Material shininess
  writer->SetField(shininess, static_cast<float>(prop->GetSpecularPower() / 128.0));
  // Material transparency
  writer->SetField(transparency, static_cast<float>(1.0 - prop->GetOpacity()));
  writer->EndNode(); // close material

  // is there a texture map
  if (anActor->GetTexture())
  {
    this->WriteATexture(anActor, writer);
  }
  writer->EndNode(); // close appearance
}

void svtkX3DExporter::WriteATexture(svtkActor* anActor, svtkX3DExporterWriter* writer)
{
  svtkTexture* aTexture = anActor->GetTexture();
  int *size, xsize, ysize;
  svtkDataArray* scalars;
  svtkDataArray* mappedScalars;
  unsigned char* txtrData;
  int totalValues;

  // make sure it is updated and then get some info
  if (aTexture->GetInput() == nullptr)
  {
    svtkErrorMacro(<< "texture has no input!\n");
    return;
  }
  aTexture->Update();
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

  std::vector<int> imageDataVec;
  imageDataVec.push_back(xsize);
  imageDataVec.push_back(ysize);
  imageDataVec.push_back(mappedScalars->GetNumberOfComponents());

  totalValues = xsize * ysize;
  txtrData = static_cast<svtkUnsignedCharArray*>(mappedScalars)->GetPointer(0);
  for (int i = 0; i < totalValues; i++)
  {
    int result = 0;
    for (int j = 0; j < imageDataVec[2]; j++)
    {
      result = result << 8;
      result += *txtrData;
      txtrData++;
    }
    imageDataVec.push_back(result);
  }

  writer->StartNode(PixelTexture);
  writer->SetField(image, &(imageDataVec.front()), imageDataVec.size(), true);
  if (!(aTexture->GetRepeat()))
  {
    writer->SetField(repeatS, false);
    writer->SetField(repeatT, false);
  }
  writer->EndNode();
}

//----------------------------------------------------------------------------
int svtkX3DExporter::HasHeadLight(svtkRenderer* ren)
{
  // make sure we have a default light
  // if we don't then use a headlight
  svtkLightCollection* lc = ren->GetLights();
  svtkCollectionSimpleIterator lsit;
  svtkLight* aLight = nullptr;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit));)
  {
    if (aLight->LightTypeIsHeadlight())
    {
      return 1;
    }
  }
  return 0;
}

// Determine if we're using cell data for scalar coloring. Returns true if
// that's the case.
static bool svtkX3DExporterWriterUsingCellColors(svtkMapper* mapper)
{
  int cellFlag = 0;
  svtkAbstractMapper::GetScalars(mapper->GetInput(), mapper->GetScalarMode(),
    mapper->GetArrayAccessMode(), mapper->GetArrayId(), mapper->GetArrayName(), cellFlag);
  return (cellFlag == 1);
}

//----------------------------------------------------------------------------
static bool svtkX3DExporterWriterRenderFaceSet(int cellType, int representation, svtkPoints* points,
  svtkIdType cellOffset, svtkCellArray* cells, svtkUnsignedCharArray* colors, bool cell_colors,
  svtkDataArray* normals, bool cell_normals, svtkDataArray* tcoords, bool common_data_written,
  int index, svtkX3DExporterWriter* writer)
{
  std::vector<int> coordIndexVector;
  std::vector<int> cellIndexVector;

  svtkIdType npts = 0;
  const svtkIdType* indx = nullptr;

  if (cellType == SVTK_POLYGON || cellType == SVTK_POLY_LINE)
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx); cellOffset++)
    {
      for (svtkIdType cc = 0; cc < npts; cc++)
      {
        coordIndexVector.push_back(static_cast<int>(indx[cc]));
      }

      if (representation == SVTK_WIREFRAME && npts > 2 && cellType == SVTK_POLYGON)
      {
        // close the polygon.
        coordIndexVector.push_back(static_cast<int>(indx[0]));
      }
      coordIndexVector.push_back(-1);

      svtkIdType cellid = cellOffset;
      cellIndexVector.push_back(cellid);
    }
  }
  else // cellType == SVTK_TRIANGLE_STRIP
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx); cellOffset++)
    {
      for (svtkIdType cc = 2; cc < npts; cc++)
      {
        svtkIdType i1;
        svtkIdType i2;
        if (cc % 2)
        {
          i1 = cc - 1;
          i2 = cc - 2;
        }
        else
        {
          i1 = cc - 2;
          i2 = cc - 1;
        }
        coordIndexVector.push_back(static_cast<int>(indx[i1]));
        coordIndexVector.push_back(static_cast<int>(indx[i2]));
        coordIndexVector.push_back(static_cast<int>(indx[cc]));

        if (representation == SVTK_WIREFRAME)
        {
          // close the polygon when drawing lines
          coordIndexVector.push_back(static_cast<int>(indx[i1]));
        }
        coordIndexVector.push_back(-1);

        svtkIdType cellid = cellOffset;
        cellIndexVector.push_back(static_cast<int>(cellid));
      }
    }
  }

  if (representation == SVTK_SURFACE)
  {
    writer->StartNode(IndexedFaceSet);
    writer->SetField(solid, false);
    writer->SetField(colorPerVertex, !cell_colors);
    writer->SetField(normalPerVertex, !cell_normals);
    writer->SetField(coordIndex, &(coordIndexVector.front()), coordIndexVector.size());
  }
  else
  {
    // don't save normals/tcoords when saving wireframes.
    normals = nullptr;
    tcoords = nullptr;

    writer->StartNode(IndexedLineSet);
    writer->SetField(colorPerVertex, !cell_colors);
    writer->SetField(coordIndex, &(coordIndexVector.front()), coordIndexVector.size());
  }

  if (normals && cell_normals && representation == SVTK_SURFACE)
  {
    writer->SetField(normalIndex, &(cellIndexVector.front()), cellIndexVector.size());
  }

  if (colors && cell_colors)
  {
    writer->SetField(colorIndex, &(cellIndexVector.front()), cellIndexVector.size());
  }

  // Now save Coordinate, Color, Normal TextureCoordinate nodes.
  // Use DEF/USE to avoid duplicates.
  if (!common_data_written)
  {
    svtkX3DExporterWriteData(points, normals, tcoords, colors, index, writer);
  }
  else
  {
    svtkX3DExporterUseData(
      (normals != nullptr), (tcoords != nullptr), (colors != nullptr), index, writer);
  }

  writer->EndNode(); // end IndexedFaceSet or IndexedLineSet
  return true;
}

static void svtkX3DExporterWriteData(svtkPoints* points, svtkDataArray* normals, svtkDataArray* tcoords,
  svtkUnsignedCharArray* colors, int index, svtkX3DExporterWriter* writer)
{
  char indexString[100];
  snprintf(indexString, sizeof(indexString), "%04d", index);

  // write out the points
  std::string defString = "SVTKcoordinates";
  writer->StartNode(Coordinate);
  writer->SetField(DEF, defString.append(indexString).c_str());
  writer->SetField(point, MFVEC3F, points->GetData());
  writer->EndNode();

  // write out the point data
  if (normals)
  {
    defString = "SVTKnormals";
    writer->StartNode(Normal);
    writer->SetField(DEF, defString.append(indexString).c_str());
    writer->SetField(svtkX3D::vector, MFVEC3F, normals);
    writer->EndNode();
  }

  // write out the point data
  if (tcoords)
  {
    defString = "SVTKtcoords";
    writer->StartNode(TextureCoordinate);
    writer->SetField(DEF, defString.append(indexString).c_str());
    writer->SetField(point, MFVEC2F, tcoords);
    writer->EndNode();
  }

  // write out the point data
  if (colors)
  {
    defString = "SVTKcolors";
    writer->StartNode(Color);
    writer->SetField(DEF, defString.append(indexString).c_str());

    std::vector<double> colorVec;
    unsigned char c[4];
    for (int i = 0; i < colors->GetNumberOfTuples(); i++)
    {
      colors->GetTypedTuple(i, c);
      colorVec.push_back(c[0] / 255.0);
      colorVec.push_back(c[1] / 255.0);
      colorVec.push_back(c[2] / 255.0);
    }
    writer->SetField(color, &(colorVec.front()), colorVec.size());
    writer->EndNode();
  }
}

static void svtkX3DExporterUseData(
  bool normals, bool tcoords, bool colors, int index, svtkX3DExporterWriter* writer)
{
  char indexString[100];
  snprintf(indexString, sizeof(indexString), "%04d", index);
  std::string defString = "SVTKcoordinates";
  writer->StartNode(Coordinate);
  writer->SetField(USE, defString.append(indexString).c_str());
  writer->EndNode();

  // write out the point data
  if (normals)
  {
    defString = "SVTKnormals";
    writer->StartNode(Normal);
    writer->SetField(USE, defString.append(indexString).c_str());
    writer->EndNode();
  }

  // write out the point data
  if (tcoords)
  {
    defString = "SVTKtcoords";
    writer->StartNode(TextureCoordinate);
    writer->SetField(USE, defString.append(indexString).c_str());
    writer->EndNode();
  }

  // write out the point data
  if (colors)
  {
    defString = "SVTKcolors";
    writer->StartNode(Color);
    writer->SetField(USE, defString.append(indexString).c_str());
    writer->EndNode();
  }
}

static bool svtkX3DExporterWriterRenderVerts(svtkPoints* points, svtkCellArray* cells,
  svtkUnsignedCharArray* colors, bool cell_colors, svtkX3DExporterWriter* writer)
{
  std::vector<double> colorVector;

  if (colors)
  {
    svtkIdType cellId = 0;
    svtkIdType npts = 0;
    const svtkIdType* indx = nullptr;
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx); cellId++)
    {
      for (svtkIdType cc = 0; cc < npts; cc++)
      {
        unsigned char color[4];
        if (cell_colors)
        {
          colors->GetTypedTuple(cellId, color);
        }
        else
        {
          colors->GetTypedTuple(indx[cc], color);
        }

        colorVector.push_back(color[0] / 255.0);
        colorVector.push_back(color[1] / 255.0);
        colorVector.push_back(color[2] / 255.0);
      }
    }
  }

  writer->StartNode(PointSet);
  writer->StartNode(Coordinate);
  writer->SetField(point, MFVEC3F, points->GetData());
  writer->EndNode();
  if (colors)
  {
    writer->StartNode(Color);
    writer->SetField(point, &(colorVector.front()), colorVector.size());
    writer->EndNode();
  }
  return true;
}

static bool svtkX3DExporterWriterRenderPoints(
  svtkPolyData* pd, svtkUnsignedCharArray* colors, bool cell_colors, svtkX3DExporterWriter* writer)
{
  if (pd->GetNumberOfCells() == 0)
  {
    return false;
  }

  std::vector<double> colorVec;
  std::vector<double> coordinateVec;

  svtkPoints* points = pd->GetPoints();

  // We render as cells so that even when coloring with cell data, the points
  // are assigned colors correctly.

  if ((colors != nullptr) && cell_colors)
  {
    // Cell colors are used, however PointSet element can only have point
    // colors, hence we use this method. Although here we end up with duplicate
    // points, that's exactly what happens in case of OpenGL rendering, so it's
    // okay.
    svtkIdType numCells = pd->GetNumberOfCells();
    svtkSmartPointer<svtkIdList> pointIds = svtkSmartPointer<svtkIdList>::New();
    for (svtkIdType cid = 0; cid < numCells; cid++)
    {
      pointIds->Reset();
      pd->GetCellPoints(cid, pointIds);

      // Get the color for this cell.
      unsigned char color[4];
      colors->GetTypedTuple(cid, color);
      double dcolor[3];
      dcolor[0] = color[0] / 255.0;
      dcolor[1] = color[1] / 255.0;
      dcolor[2] = color[2] / 255.0;

      for (svtkIdType cc = 0; cc < pointIds->GetNumberOfIds(); cc++)
      {
        svtkIdType pid = pointIds->GetId(cc);
        double* point = points->GetPoint(pid);
        coordinateVec.push_back(point[0]);
        coordinateVec.push_back(point[1]);
        coordinateVec.push_back(point[2]);
        colorVec.push_back(dcolor[0]);
        colorVec.push_back(dcolor[1]);
        colorVec.push_back(dcolor[2]);
      }
    }
  }
  else
  {
    // Colors are point colors, simply render all the points and corresponding
    // colors.
    svtkIdType numPoints = points->GetNumberOfPoints();
    for (svtkIdType pid = 0; pid < numPoints; pid++)
    {
      double* point = points->GetPoint(pid);
      coordinateVec.push_back(point[0]);
      coordinateVec.push_back(point[1]);
      coordinateVec.push_back(point[2]);

      if (colors)
      {
        unsigned char color[4];
        colors->GetTypedTuple(pid, color);
        colorVec.push_back(color[0] / 255.0);
        colorVec.push_back(color[1] / 255.0);
        colorVec.push_back(color[2] / 255.0);
      }
    }
  }

  writer->StartNode(PointSet);
  writer->StartNode(Coordinate);
  writer->SetField(point, &(coordinateVec.front()), coordinateVec.size());
  writer->EndNode(); // Coordinate
  if (colors)
  {
    writer->StartNode(Color);
    writer->SetField(color, &(colorVec.front()), colorVec.size());
    writer->EndNode(); // Color
  }
  writer->EndNode(); // PointSet
  return true;
}
//----------------------------------------------------------------------------
char* svtkX3DExporter::RegisterAndGetOutputString()
{
  char* tmp = this->OutputString;

  this->OutputString = nullptr;
  this->OutputStringLength = 0;

  return tmp;
}
