/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgressBarRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProgressBarRepresentation
 * @brief   represent a svtkProgressBarWidget
 *
 * This class is used to represent a svtkProgressBarWidget.
 *
 * @sa
 * svtkProgressBarWidget
 */

#ifndef svtkProgressBarRepresentation_h
#define svtkProgressBarRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkActor2D;
class svtkPoints;
class svtkPolyData;
class svtkProperty2D;
class svtkUnsignedCharArray;

class SVTKINTERACTIONWIDGETS_EXPORT svtkProgressBarRepresentation : public svtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkProgressBarRepresentation* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkProgressBarRepresentation, svtkBorderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * By obtaining this property you can specify the properties of the
   * representation.
   */
  svtkGetObjectMacro(Property, svtkProperty2D);
  //@}

  //@{
  /**
   * Set/Get the progress rate of the progress bar, between 0 and 1
   * default is 0
   */
  svtkSetClampMacro(ProgressRate, double, 0, 1);
  svtkGetMacro(ProgressRate, double);
  //@}

  //@{
  /**
   * Set/Get the progress bar color
   * Default is pure green
   */
  svtkSetVector3Macro(ProgressBarColor, double);
  svtkGetVector3Macro(ProgressBarColor, double);
  //@}

  //@{
  /**
   * Set/Get the background color
   * Default is white
   */
  svtkSetVector3Macro(BackgroundColor, double);
  svtkGetVector3Macro(BackgroundColor, double);
  //@}

  //@{
  /**
   * Set/Get background visibility
   * Default is off
   */
  svtkSetMacro(DrawBackground, bool);
  svtkGetMacro(DrawBackground, bool);
  svtkBooleanMacro(DrawBackground, bool);
  //@}

  //@{
  /**
   * Satisfy the superclasses' API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override;
  //@}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkProgressBarRepresentation();
  ~svtkProgressBarRepresentation() override;

  double ProgressRate;
  double ProgressBarColor[3];
  double BackgroundColor[3];
  bool DrawBackground;

  svtkPoints* Points;
  svtkUnsignedCharArray* ProgressBarData;
  svtkProperty2D* Property;
  svtkActor2D* Actor;
  svtkActor2D* BackgroundActor;

private:
  svtkProgressBarRepresentation(const svtkProgressBarRepresentation&) = delete;
  void operator=(const svtkProgressBarRepresentation&) = delete;
};

#endif
