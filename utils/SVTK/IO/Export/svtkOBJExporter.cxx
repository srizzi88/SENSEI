/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBJExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOBJExporter.h"

#include "svtkActorCollection.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCellArray.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkGeometryFilter.h"
#include "svtkImageData.h"
#include "svtkImageFlip.h"
#include "svtkMapper.h"
#include "svtkNew.h"
#include "svtkNumberToString.h"
#include "svtkObjectFactory.h"
#include "svtkPNGWriter.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRendererCollection.h"
#include "svtkTexture.h"
#include "svtkTransform.h"

#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

#include <sstream>

svtkStandardNewMacro(svtkOBJExporter);

svtkOBJExporter::svtkOBJExporter()
{
  this->FilePrefix = nullptr;
  this->OBJFileComment = nullptr;
  this->MTLFileComment = nullptr;
  this->FlipTexture = false;
  this->SetOBJFileComment("wavefront obj file written by the visualization toolkit");
  this->SetMTLFileComment("wavefront mtl file written by the visualization toolkit");
}

svtkOBJExporter::~svtkOBJExporter()
{
  delete[] this->OBJFileComment;
  delete[] this->MTLFileComment;
  delete[] this->FilePrefix;
}

void svtkOBJExporter::WriteData()
{
  // make sure the user specified a filename
  if (this->FilePrefix == nullptr)
  {
    svtkErrorMacro(<< "Please specify file prefix to use");
    return;
  }

  svtkRenderer* ren = this->ActiveRenderer;
  if (!ren)
  {
    ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    svtkErrorMacro(<< "no actors found for writing .obj file.");
    return;
  }

  // try opening the files
  std::string objFilePath = std::string(this->FilePrefix) + ".obj";
  std::string filePrefix(this->FilePrefix);
  // get the model name which isthe last component of the FilePrefix
  std::string modelName;
  if (filePrefix.find_last_of("/") != std::string::npos)
  {
    modelName = filePrefix.substr(filePrefix.find_last_of("/") + 1);
  }
  else
  {
    modelName = filePrefix;
  }

  svtksys::ofstream fpObj(objFilePath.c_str(), ios::out);
  if (!fpObj)
  {
    svtkErrorMacro(<< "unable to open " << objFilePath);
    return;
  }
  std::string mtlFilePath = std::string(this->FilePrefix) + ".mtl";
  svtksys::ofstream fpMtl(mtlFilePath.c_str(), ios::out);
  if (!fpMtl)
  {
    fpMtl.close();
    svtkErrorMacro(<< "unable to open .mtl files ");
    return;
  }

  //
  //  Write header
  //
  svtkDebugMacro("Writing wavefront files");
  if (this->GetOBJFileComment())
  {
    fpObj << "#  " << this->GetOBJFileComment() << "\n\n";
  }

  std::string mtlFileName = svtksys::SystemTools::GetFilenameName(mtlFilePath);
  fpObj << "mtllib " << mtlFileName << "\n\n";
  if (this->GetMTLFileComment())
  {
    fpMtl << "# " << this->GetMTLFileComment() << "\n\n";
  }

  svtkActorCollection* allActors = ren->GetActors();
  svtkCollectionSimpleIterator actorsIt;
  svtkActor* anActor;
  int idStart = 1;
  for (allActors->InitTraversal(actorsIt); (anActor = allActors->GetNextActor(actorsIt));)
  {
    svtkAssemblyPath* aPath;
    for (anActor->InitPathTraversal(); (aPath = anActor->GetNextPath());)
    {
      svtkActor* aPart = svtkActor::SafeDownCast(aPath->GetLastNode()->GetViewProp());
      this->WriteAnActor(aPart, fpObj, fpMtl, modelName, idStart);
    }
  }
  // Write texture files
  for (auto t : this->TextureFileMap)
  {
    std::stringstream fullFileName;
    fullFileName << this->FilePrefix << t.first;
    auto writeTexture = svtkSmartPointer<svtkPNGWriter>::New();
    if (this->FlipTexture)
    {
      auto flip = svtkSmartPointer<svtkImageFlip>::New();
      flip->SetInputData(t.second->GetInput());
      flip->SetFilteredAxis(1);
      flip->Update();
      writeTexture->SetInputData(flip->GetOutput());
    }
    else
    {
      writeTexture->SetInputData(t.second->GetInput());
    }
    writeTexture->SetFileName(fullFileName.str().c_str());
    writeTexture->Write();
  }
  fpObj.close();
  fpMtl.close();
}

void svtkOBJExporter::WriteAnActor(
  svtkActor* anActor, std::ostream& fpObj, std::ostream& fpMtl, std::string& modelName, int& idStart)
{
  svtkDataSet* ds;
  svtkNew<svtkPolyData> pd;
  svtkPointData* pntData;
  svtkPoints* points;
  svtkDataArray* tcoords;
  int i, i1, i2, idNext;
  svtkProperty* prop;
  double* tempd;
  double* p;
  svtkCellArray* cells;
  svtkNew<svtkTransform> trans;
  svtkIdType npts = 0;
  const svtkIdType* indx = nullptr;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == nullptr)
  {
    return;
  }
  // write out the material properties to the mat file
  prop = anActor->GetProperty();
  if (anActor->GetVisibility() == 0)
  {
    return;
  }
  svtkNumberToString convert;
  double temp;
  fpMtl << "newmtl mtl" << idStart << "\n";
  tempd = prop->GetAmbientColor();
  temp = prop->GetAmbient();
  fpMtl << "Ka " << convert(temp * tempd[0]) << " " << convert(temp * tempd[1]) << " "
        << convert(temp * tempd[2]) << "\n";
  tempd = prop->GetDiffuseColor();
  temp = prop->GetDiffuse();
  fpMtl << "Kd " << convert(temp * tempd[0]) << " " << convert(temp * tempd[1]) << " "
        << convert(temp * tempd[2]) << "\n";
  tempd = prop->GetSpecularColor();
  temp = prop->GetSpecular();
  fpMtl << "Ks " << convert(temp * tempd[0]) << " " << convert(temp * tempd[1]) << " "
        << convert(temp * tempd[2]) << "\n";
  fpMtl << "Ns " << convert(prop->GetSpecularPower()) << "\n";
  fpMtl << "Tr " << convert(prop->GetOpacity()) << "\n";
  fpMtl << "illum 3\n";

  // Actor has the texture
  bool hasTexture = anActor->GetTexture() != nullptr;
  ;

  // Actor's property has the texture. We choose the albedo texture
  // since it seems to be similar to the texture we expect
  auto allTextures = prop->GetAllTextures();
  bool hasTextureProp = allTextures.find("albedoTex") != allTextures.end();
  if (hasTexture)
  {
    std::stringstream textureFileName;
    textureFileName << "texture" << idStart << ".png";
    fpMtl << "map_Kd " << modelName << textureFileName.str() << "\n\n";
    this->TextureFileMap[textureFileName.str()] = anActor->GetTexture();
  }
  else if (hasTextureProp)
  {
    std::stringstream textureFileName;
    textureFileName << "albedoTex"
                    << "_" << idStart << ".png";
    fpMtl << "map_Kd " << modelName << textureFileName.str() << "\n\n";
    auto albedoTexture = allTextures.find("albedoTex");
    this->TextureFileMap[textureFileName.str()] = albedoTexture->second;
    this->FlipTexture = true;
  }

  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();
  // see if the mapper has an input.
  if (ds == nullptr)
  {
    return;
  }
  anActor->GetMapper()->GetInputAlgorithm()->Update();
  trans->SetMatrix(anActor->svtkProp3D::GetMatrix());

  // we really want polydata
  if (ds->GetDataObjectType() != SVTK_POLY_DATA)
  {
    svtkNew<svtkGeometryFilter> gf;
    gf->SetInputConnection(anActor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    pd->DeepCopy(gf->GetOutput());
  }
  else
  {
    pd->DeepCopy(ds);
  }

  // write out the points
  points = svtkPoints::New();
  trans->TransformPoints(pd->GetPoints(), points);
  for (i = 0; i < points->GetNumberOfPoints(); i++)
  {
    p = points->GetPoint(i);
    fpObj << "v " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
  }
  idNext = idStart + static_cast<int>(points->GetNumberOfPoints());
  points->Delete();

  // write out the point data
  pntData = pd->GetPointData();
  if (pntData->GetNormals())
  {
    svtkNew<svtkFloatArray> normals;
    normals->SetNumberOfComponents(3);
    trans->TransformNormals(pntData->GetNormals(), normals);
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
    {
      p = normals->GetTuple(i);
      fpObj << "vn " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
    }
  }

  tcoords = pntData->GetTCoords();
  if (tcoords)
  {
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
    {
      p = tcoords->GetTuple(i);
      fpObj << "vt " << convert(p[0]) << " " << convert(p[1]) << " " << 0.0 << "\n";
    }
  }

  // write out a group name and material
  fpObj << "\ng grp" << idStart << "\n";
  fpObj << "usemtl mtl" << idStart << "\n";
  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
  {
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fpObj << "p ";
      for (i = 0; i < npts; i++)
      {
        // treating svtkIdType as int
        fpObj << static_cast<int>(indx[i]) + idStart << " ";
      }
      fpObj << "\n";
    }
  }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
  {
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fpObj << "l ";
      if (tcoords)
      {
        for (i = 0; i < npts; i++)
        {
          // treating svtkIdType as int
          fpObj << static_cast<int>(indx[i]) + idStart << "/"
                << static_cast<int>(indx[i]) + idStart;
        }
      }
      else
      {
        for (i = 0; i < npts; i++)
        {
          // treating svtkIdType as int
          fpObj << static_cast<int>(indx[i]) + idStart << " ";
        }
      }
      fpObj << "\n";
    }
  }
  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
  {
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fpObj << "f ";
      for (i = 0; i < npts; i++)
      {
        if (pntData->GetNormals())
        {
          if (tcoords)
          {
            // treating svtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << " ";
          }
          else
          {
            // treating svtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << "//"
                  << static_cast<int>(indx[i]) + idStart << " ";
          }
        }
        else
        {
          if (tcoords)
          {
            // treating svtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << " ";
          }
          else
          {
            // treating svtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << " ";
          }
        }
      }
      fpObj << "\n";
    }
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
  {
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      for (i = 2; i < npts; i++)
      {
        if (i % 2 == 0)
        {
          i1 = i - 2;
          i2 = i - 1;
        }
        else
        {
          i1 = i - 1;
          i2 = i - 2;
        }
        if (pntData->GetNormals())
        {
          if (tcoords)
          {
            // treating svtkIdType as int
            fpObj << "f " << static_cast<int>(indx[i1]) + idStart << "/"
                  << static_cast<int>(indx[i1]) + idStart << "/"
                  << static_cast<int>(indx[i1]) + idStart << " ";
            fpObj << static_cast<int>(indx[i2]) + idStart << "/"
                  << static_cast<int>(indx[i2]) + idStart << "/"
                  << static_cast<int>(indx[i2]) + idStart << " ";
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
          else
          {
            // treating svtkIdType as int
            fpObj << static_cast<int>(indx[i2]) + idStart << "//"
                  << static_cast<int>(indx[i2]) + idStart << " ";
            fpObj << static_cast<int>(indx[i]) + idStart << "//"
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
        }
        else
        {
          if (tcoords)
          {
            // treating svtkIdType as int
            fpObj << "f " << static_cast<int>(indx[i1]) + idStart << "/"
                  << static_cast<int>(indx[i1]) + idStart << " ";
            fpObj << static_cast<int>(indx[i2]) + idStart << "/"
                  << static_cast<int>(indx[i2]) + idStart << " ";
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
          else
          {
            // treating svtkIdType as int
            fpObj << "f " << static_cast<int>(indx[i1]) + idStart << " "
                  << static_cast<int>(indx[i2]) + idStart << " "
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
        }
      }
    }
  }

  idStart = idNext;
}

void svtkOBJExporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FilePrefix: " << (this->FilePrefix ? this->FilePrefix : "(null)") << "\n";
  os << indent << "OBJFileComment: " << (this->OBJFileComment ? this->OBJFileComment : "(null)")
     << "\n";
  os << indent << "MTLFileComment: " << (this->MTLFileComment ? this->MTLFileComment : "(null)")
     << "\n";
}
