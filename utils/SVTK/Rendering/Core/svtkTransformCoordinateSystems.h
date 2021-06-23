/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformCoordinateSystems.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTransformCoordinateSystems
 * @brief   transform points into different coordinate systems
 *
 * This filter transforms points from one coordinate system to another. The user
 * must specify the coordinate systems in which the input and output are
 * specified. The user must also specify the SVTK viewport (i.e., renderer) in
 * which the transformation occurs.
 *
 * @sa
 * svtkCoordinate svtkTransformFilter svtkTransformPolyData svtkPolyDataMapper2D
 */

#ifndef svtkTransformCoordinateSystems_h
#define svtkTransformCoordinateSystems_h

#include "svtkCoordinate.h" //to get the defines in svtkCoordinate
#include "svtkPointSetAlgorithm.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkTransformCoordinateSystems : public svtkPointSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkTransformCoordinateSystems, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Instantiate this class. By default no transformation is specified and
   * the input and output is identical.
   */
  static svtkTransformCoordinateSystems* New();

  //@{
  /**
   * Set/get the coordinate system in which the input is specified.
   * The current options are World, Viewport, and Display. By default the
   * input coordinate system is World.
   */
  svtkSetMacro(InputCoordinateSystem, int);
  svtkGetMacro(InputCoordinateSystem, int);
  void SetInputCoordinateSystemToDisplay() { this->SetInputCoordinateSystem(SVTK_DISPLAY); }
  void SetInputCoordinateSystemToViewport() { this->SetInputCoordinateSystem(SVTK_VIEWPORT); }
  void SetInputCoordinateSystemToWorld() { this->SetInputCoordinateSystem(SVTK_WORLD); }
  //@}

  //@{
  /**
   * Set/get the coordinate system to which to transform the output.
   * The current options are World, Viewport, and Display. By default the
   * output coordinate system is Display.
   */
  svtkSetMacro(OutputCoordinateSystem, int);
  svtkGetMacro(OutputCoordinateSystem, int);
  void SetOutputCoordinateSystemToDisplay() { this->SetOutputCoordinateSystem(SVTK_DISPLAY); }
  void SetOutputCoordinateSystemToViewport() { this->SetOutputCoordinateSystem(SVTK_VIEWPORT); }
  void SetOutputCoordinateSystemToWorld() { this->SetOutputCoordinateSystem(SVTK_WORLD); }
  //@}

  /**
   * Return the MTime also considering the instance of svtkCoordinate.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * In order for a successful coordinate transformation to occur, an
   * instance of svtkViewport (e.g., a SVTK renderer) must be specified.
   * NOTE: this is a raw pointer, not a weak pointer nor a reference counted
   * object, to avoid reference cycle loop between rendering classes and filter
   * classes.
   */
  void SetViewport(svtkViewport* viewport);
  svtkGetObjectMacro(Viewport, svtkViewport);
  //@}

protected:
  svtkTransformCoordinateSystems();
  ~svtkTransformCoordinateSystems() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int InputCoordinateSystem;
  int OutputCoordinateSystem;
  svtkViewport* Viewport;

  svtkCoordinate* TransformCoordinate;

private:
  svtkTransformCoordinateSystems(const svtkTransformCoordinateSystems&) = delete;
  void operator=(const svtkTransformCoordinateSystems&) = delete;
};

#endif
