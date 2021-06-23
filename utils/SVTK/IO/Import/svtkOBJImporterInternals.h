/*=========================================================================
  Program:   Visualization Toolkit
  Module:    svtkOBJImporterInternals.h
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef svtkOBJImporterInternals_h
#define svtkOBJImporterInternals_h
#ifndef __SVTK_WRAP__

#include "svtkActor.h"
#include "svtkOBJImporter.h"
#include "svtkPolyDataAlgorithm.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

struct SVTKIOIMPORT_EXPORT svtkOBJImportedMaterial
{
  std::string name;
  std::string texture_filename;
  double amb[3];
  double diff[3];
  double spec[3];
  double map_Kd_scale[3];
  double map_Kd_offset[3];
  int illum;
  double reflect;
  double refract;
  double trans;
  double specularPower;
  double glossy;
  double refract_index;
  const char* GetClassName() { return "svtkOBJImportedMaterial"; }
  svtkOBJImportedMaterial();
};

SVTKIOIMPORT_EXPORT
void obj_set_material_defaults(svtkOBJImportedMaterial* mtl);

struct svtkOBJImportedPolyDataWithMaterial;

class SVTKIOIMPORT_EXPORT svtkOBJPolyDataProcessor : public svtkPolyDataAlgorithm
{
public:
  static svtkOBJPolyDataProcessor* New();
  svtkTypeMacro(svtkOBJPolyDataProcessor, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Description:
  // Specify file name of Wavefront .obj file.
  void SetFileName(const char* arg)
  {
    if (arg == nullptr)
    {
      return;
    }
    if (!strcmp(this->FileName.c_str(), arg))
    {
      return;
    }
    FileName = std::string(arg);
  }
  void SetMTLfileName(const char* arg)
  {
    if (arg == nullptr)
    {
      return;
    }
    if (!strcmp(this->MTLFileName.c_str(), arg))
    {
      return;
    }
    MTLFileName = std::string(arg);
    this->DefaultMTLFileName = false;
  }
  void SetTexturePath(const char* arg)
  {
    TexturePath = std::string(arg);
    if (TexturePath.empty())
      return;
#if defined(_WIN32)
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    if (TexturePath.at(TexturePath.size() - 1) != sep)
      TexturePath += sep;
  }
  const std::string& GetTexturePath() const { return TexturePath; }

  const std::string& GetFileName() const { return FileName; }

  const std::string& GetMTLFileName() const { return MTLFileName; }

  svtkSetMacro(VertexScale, double);
  svtkGetMacro(VertexScale, double);
  svtkGetMacro(SuccessParsingFiles, int);

  virtual svtkPolyData* GetOutput(int idx);

  int GetNumberOfOutputs();

  svtkOBJImportedMaterial* GetMaterial(int k);

  std::string GetTextureFilename(int idx); // return string by index

  double VertexScale; // scale vertices by this during import

  std::vector<svtkOBJImportedMaterial*> parsedMTLs;
  std::map<std::string, svtkOBJImportedMaterial*> mtlName_to_mtlData;

  // our internal parsing/storage
  std::vector<svtkOBJImportedPolyDataWithMaterial*> poly_list;

  // what gets returned to client code via GetOutput()
  std::vector<svtkSmartPointer<svtkPolyData> > outVector_of_svtkPolyData;

  std::vector<svtkSmartPointer<svtkActor> > actor_list;
  /////////////////////

  std::vector<svtkOBJImportedMaterial*> ParseOBJandMTL(std::string filename, int& result_code);

  void ReadVertices(bool gotFirstUseMaterialTag, char* pLine, float xyz, int lineNr,
    const double v_scale, bool everything_ok, svtkPoints* points, const bool use_scale);

protected:
  svtkOBJPolyDataProcessor();
  ~svtkOBJPolyDataProcessor() override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override
    /*override*/;

  svtkSetMacro(SuccessParsingFiles, int);

  std::string FileName;    // filename (.obj) being read
  std::string MTLFileName; // associated .mtl to *.obj, typically it is *.obj.mtl
  bool DefaultMTLFileName; // tells whether default of *.obj.mtl to be used
  std::string TexturePath;
  int SuccessParsingFiles;

private:
  svtkOBJPolyDataProcessor(const svtkOBJPolyDataProcessor&) = delete;
  void operator=(const svtkOBJPolyDataProcessor&) = delete;
};

class svtkRenderWindow;
class svtkRenderer;
SVTKIOIMPORT_EXPORT
void bindTexturedPolydataToRenderWindow(
  svtkRenderWindow* renderWindow, svtkRenderer* renderer, svtkOBJPolyDataProcessor* reader);

#endif
#endif
// SVTK-HeaderTest-Exclude: svtkOBJImporterInternals.h
