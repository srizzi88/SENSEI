/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImporter
 * @brief   importer abstract class
 *
 * svtkImporter is an abstract class that specifies the protocol for
 * importing actors, cameras, lights and properties into a
 * svtkRenderWindow. The following takes place:
 * 1) Create a RenderWindow and Renderer if none is provided.
 * 2) Call ImportBegin, if ImportBegin returns False, return
 * 3) Call ReadData, which calls:
 *  a) Import the Actors
 *  b) Import the cameras
 *  c) Import the lights
 *  d) Import the Properties
 * 7) Call ImportEnd
 *
 * Subclasses optionally implement the ImportActors, ImportCameras,
 * ImportLights and ImportProperties or ReadData methods. An ImportBegin and
 * ImportEnd can optionally be provided to perform Importer-specific
 * initialization and termination.  The Read method initiates the import
 * process. If a RenderWindow is provided, its Renderer will contained
 * the imported objects. If the RenderWindow has no Renderer, one is
 * created. If no RenderWindow is provided, both a RenderWindow and
 * Renderer will be created. Both the RenderWindow and Renderer can be
 * accessed using Get methods.
 *
 * @sa
 * svtk3DSImporter svtkExporter
 */

#ifndef svtkImporter_h
#define svtkImporter_h

#include "svtkIOImportModule.h" // For export macro
#include "svtkObject.h"

#include <string>

class svtkAbstractArray;
class svtkDataSet;
class svtkDoubleArray;
class svtkRenderWindow;
class svtkRenderer;

class SVTKIOIMPORT_EXPORT svtkImporter : public svtkObject
{
public:
  svtkTypeMacro(svtkImporter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the renderer that contains the imported actors, cameras and
   * lights.
   */
  svtkGetObjectMacro(Renderer, svtkRenderer);
  //@}

  //@{
  /**
   * Set the svtkRenderWindow to contain the imported actors, cameras and
   * lights, If no svtkRenderWindow is set, one will be created and can be
   * obtained with the GetRenderWindow method. If the svtkRenderWindow has been
   * specified, the first svtkRenderer it has will be used to import the
   * objects. If the svtkRenderWindow has no Renderer, one will be created and
   * can be accessed using GetRenderer.
   */
  virtual void SetRenderWindow(svtkRenderWindow*);
  svtkGetObjectMacro(RenderWindow, svtkRenderWindow);
  //@}

  //@{
  /**
   * Import the actors, cameras, lights and properties into a svtkRenderWindow.
   */
  void Read();
  void Update() { this->Read(); }
  //@}

  /**
   * Recover a printable string that let importer implementation
   * Describe their outputs.
   */
  virtual std::string GetOutputsDescription() { return ""; };

  /**
   * Get the number of available animations.
   * Return -1 if not provided by implementation.
   */
  virtual svtkIdType GetNumberOfAnimations();

  /**
   * Get the name of an animation.
   * Return an empty if not provided by implementation.
   */
  virtual std::string GetAnimationName(svtkIdType svtkNotUsed(animationIndex)) { return ""; };

  //@{
  /**
   * Enable/Disable/Get the status of specific animations
   */
  virtual void EnableAnimation(svtkIdType svtkNotUsed(animationIndex)){};
  virtual void DisableAnimation(svtkIdType svtkNotUsed(animationIndex)){};
  virtual bool IsAnimationEnabled(svtkIdType svtkNotUsed(animationIndex)) { return false; };
  //@}

  /**
   * Get temporal informations for the currently enabled animations.
   * the three return arguments can be defined or not.
   * Return true in case of success, false otherwise.
   */
  virtual bool GetTemporalInformation(
    svtkIdType animationIndex, int& nbTimeSteps, double timeRange[2], svtkDoubleArray* timeSteps);

  /**
   * Import the actors, camera, lights and properties at a specific timestep.
   * If not reimplemented, only call Update().
   */
  virtual void UpdateTimeStep(double timeStep);

protected:
  svtkImporter();
  ~svtkImporter() override;

  virtual int ImportBegin() { return 1; }
  virtual void ImportEnd() {}
  virtual void ImportActors(svtkRenderer*) {}
  virtual void ImportCameras(svtkRenderer*) {}
  virtual void ImportLights(svtkRenderer*) {}
  virtual void ImportProperties(svtkRenderer*) {}

  static std::string GetDataSetDescription(svtkDataSet* ds, svtkIndent indent);
  static std::string GetArrayDescription(svtkAbstractArray* array, svtkIndent indent);

  svtkRenderer* Renderer;
  svtkRenderWindow* RenderWindow;

  virtual void ReadData();

private:
  svtkImporter(const svtkImporter&) = delete;
  void operator=(const svtkImporter&) = delete;
};

#endif
