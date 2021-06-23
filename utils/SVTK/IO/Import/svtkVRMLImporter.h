/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVRMLImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVRMLImporter
 * @brief   imports VRML 2.0 files.
 *
 *
 * svtkVRMLImporter imports VRML 2.0 files into SVTK.
 *
 * @warning
 * These nodes are currently supported:
 *      Appearance                              IndexedFaceSet
 *      Box                                     IndexedLineSet
 *      Color                                   Material
 *      Cone                                    Shape
 *      Coordinate                              Sphere
 *      Cylinder                                Transform
 *      DirectionalLight
 *
 * @warning
 * As you can see this implementation focuses on getting the geometry
 * translated.  The routes and scripting nodes are ignored since they deal
 * with directly accessing a nodes internal structure based on the VRML
 * spec. Since this is a translation the internal data structures differ
 * greatly from the VRML spec and the External Authoring Interface (see the
 * VRML spec). The DEF/USE mechanism does allow the SVTK user to extract
 * objects from the scene and directly manipulate them using the native
 * language (Python, Java, or whatever language SVTK is wrapped
 * in). This, in a way, removes the need for the route and script mechanism
 * (not completely though).
 * Texture coordinates are attached to the mesh is available but
 * image textures are not loaded.
 * Viewpoints (camera presets) are not imported.
 *
 * @par Thanks:
 *  Thanks to Russ Coucher of Areva for numerous bug fixes and a new test.
 *
 * @sa
 * svtkImporter
 */

#ifndef svtkVRMLImporter_h
#define svtkVRMLImporter_h

#include "svtkIOImportModule.h" // For export macro
#include "svtkImporter.h"

class svtkActor;
class svtkAlgorithm;
class svtkProperty;
class svtkLight;
class svtkTransform;
class svtkLookupTable;
class svtkFloatArray;
class svtkPolyDataMapper;
class svtkPoints;
class svtkIdTypeArray;
class svtkVRMLImporterInternal;
class svtkVRMLYaccData;
class svtkCellArray;

class SVTKIOIMPORT_EXPORT svtkVRMLImporter : public svtkImporter
{
public:
  static svtkVRMLImporter* New();

  svtkTypeMacro(svtkVRMLImporter, svtkImporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the resolution for Sphere, Cone and Cylinder shape sources.
   * Default is 12.
   */
  svtkSetMacro(ShapeResolution, int);
  svtkGetMacro(ShapeResolution, int);
  //@}

  /**
   * In the VRML spec you can DEF and USE nodes (name them),
   * This routine will return the associated SVTK object which
   * was created as a result of the DEF mechanism
   * Send in the name from the VRML file, get the SVTK object.
   * You will have to check and correctly cast the object since
   * this only returns svtkObjects.
   */
  svtkObject* GetVRMLDEFObject(const char* name);

  /**
   * Get a printable string describing the outputs
   */
  std::string GetOutputsDescription() override;

protected:
  svtkVRMLImporter();
  ~svtkVRMLImporter() override;

  int OpenImportFile();
  int ImportBegin() override;
  void ImportEnd() override;
  void ImportActors(svtkRenderer*) override {}
  void ImportCameras(svtkRenderer*) override {}
  void ImportLights(svtkRenderer*) override {}
  void ImportProperties(svtkRenderer*) override {}

  //@{
  /**
   * Needed by the yacc/lex grammar used
   */
  virtual void enterNode(const char*);
  virtual void exitNode();
  virtual void enterField(const char*);
  virtual void exitField();
  virtual void useNode(const char*);
  //@}

  /**
   * Return the file pointer to the open file.
   */
  FILE* GetFileFD() { return this->FileFD; }

  char* FileName;
  FILE* FileFD;
  int ShapeResolution;

  friend class svtkVRMLYaccData;

private:
  svtkPoints* PointsNew();
  svtkFloatArray* FloatArrayNew();
  svtkIdTypeArray* IdTypeArrayNew();

  void DeleteObject(svtkObject*);

  svtkVRMLImporterInternal* Internal;
  svtkVRMLYaccData* Parser;
  svtkActor* CurrentActor;
  svtkProperty* CurrentProperty;
  svtkLight* CurrentLight;
  svtkTransform* CurrentTransform;
  svtkAlgorithm* CurrentSource;
  svtkPoints* CurrentPoints;
  svtkFloatArray* CurrentNormals;
  svtkCellArray* CurrentNormalCells;
  svtkFloatArray* CurrentTCoords;
  svtkCellArray* CurrentTCoordCells;
  svtkLookupTable* CurrentLut;
  svtkFloatArray* CurrentScalars;
  svtkPolyDataMapper* CurrentMapper;

private:
  svtkVRMLImporter(const svtkVRMLImporter&) = delete;
  void operator=(const svtkVRMLImporter&) = delete;
};

#endif
