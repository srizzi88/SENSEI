/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDistanceRepresentation3D
 * @brief   represent the svtkDistanceWidget
 *
 * The svtkDistanceRepresentation3D is a representation for the
 * svtkDistanceWidget. This representation consists of a measuring line (axis)
 * and two svtkHandleWidgets to place the end points of the line. Note that
 * this particular widget draws its representation in 3D space, so the widget
 * can be occluded.
 *
 * @sa
 * svtkDistanceWidget svtkDistanceRepresentation svtkDistanceRepresentation2D
 */

#ifndef svtkDistanceRepresentation3D_h
#define svtkDistanceRepresentation3D_h

#include "svtkDistanceRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkActor;
class svtkVectorText;
class svtkFollower;
class svtkBox;
class svtkCylinderSource;
class svtkGlyph3D;
class svtkDoubleArray;
class svtkTransformPolyDataFilter;
class svtkProperty;

class SVTKINTERACTIONWIDGETS_EXPORT svtkDistanceRepresentation3D : public svtkDistanceRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static svtkDistanceRepresentation3D* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkDistanceRepresentation3D, svtkDistanceRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Satisfy the superclasses API.
   */
  double GetDistance() override { return this->Distance; }

  //@{
  /**
   * Scale the glyphs used as tick marks. By default it is
   * 1/40th of the length.
   */
  void SetGlyphScale(double scale);
  svtkGetMacro(GlyphScale, double);
  //@}

  /**
   * Convenience method to get the line actor property.
   */
  virtual svtkProperty* GetLineProperty();

  //@{
  /**
   * Set/Get position of the label title in normalized coordinates [0,1].
   * 0 is at the start of the line whereas 1 is at the end.
   */
  void SetLabelPosition(double labelPosition);
  svtkGetMacro(LabelPosition, double);
  //@}

  //@{
  /**
   * Set/Get the maximum number of ticks in ruler mode.
   */
  svtkSetClampMacro(MaximumNumberOfRulerTicks, int, 1, SVTK_INT_MAX);
  svtkGetMacro(MaximumNumberOfRulerTicks, int);
  //@}

  //@{
  /**
   * Convenience method to get the glyph actor. Using this it is
   * possible to control the appearance of the glyphs.
   */
  svtkGetObjectMacro(GlyphActor, svtkActor);
  //@}

  //@{
  /**
   * Convenience method Get the label actor. It is possible to
   * control the appearance of the label.
   */
  svtkGetObjectMacro(LabelActor, svtkFollower);
  virtual void SetLabelActor(svtkFollower*);
  //@}

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  double* GetPoint1WorldPosition() override;
  double* GetPoint2WorldPosition() override;
  void GetPoint1WorldPosition(double pos[3]) override;
  void GetPoint2WorldPosition(double pos[3]) override;
  void SetPoint1WorldPosition(double pos[3]) override;
  void SetPoint2WorldPosition(double pos[3]) override;
  //@}

  void SetPoint1DisplayPosition(double pos[3]) override;
  void SetPoint2DisplayPosition(double pos[3]) override;
  void GetPoint1DisplayPosition(double pos[3]) override;
  void GetPoint2DisplayPosition(double pos[3]) override;

  //@{
  /**
   * Method to satisfy superclasses' API.
   */
  void BuildRepresentation() override;
  double* GetBounds() override;
  //@}

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  //@}

  //@{
  /**
   * Scale text (font size along each dimension). This helps control
   * the appearance of the 3D text.
   */
  void SetLabelScale(double x, double y, double z)
  {
    double scale[3];
    scale[0] = x;
    scale[1] = y;
    scale[2] = z;
    this->SetLabelScale(scale);
  }
  virtual void SetLabelScale(double scale[3]);
  virtual double* GetLabelScale();
  //@}

  /**
   * Get the distance annotation property
   */
  virtual svtkProperty* GetLabelProperty();

protected:
  svtkDistanceRepresentation3D();
  ~svtkDistanceRepresentation3D() override;

  // The line
  svtkPoints* LinePoints;
  svtkPolyData* LinePolyData;
  svtkPolyDataMapper* LineMapper;
  svtkActor* LineActor;

  // The distance label
  svtkVectorText* LabelText;
  svtkPolyDataMapper* LabelMapper;
  svtkFollower* LabelActor;

  // Support internal operations
  bool LabelScaleSpecified;

  // The 3D disk tick marks
  svtkPoints* GlyphPoints;
  svtkDoubleArray* GlyphVectors;
  svtkPolyData* GlyphPolyData;
  svtkCylinderSource* GlyphCylinder;
  svtkTransformPolyDataFilter* GlyphXForm;
  svtkGlyph3D* Glyph3D;
  svtkPolyDataMapper* GlyphMapper;
  svtkActor* GlyphActor;

  // Glyph3D scale
  double GlyphScale;
  bool GlyphScaleSpecified;

  // The distance between the two points
  double Distance;

  // Support GetBounds() method
  svtkBox* BoundingBox;

  // Maximum number of ticks on the 3d ruler
  int MaximumNumberOfRulerTicks;

  // Label title position
  double LabelPosition;

private:
  svtkDistanceRepresentation3D(const svtkDistanceRepresentation3D&) = delete;
  void operator=(const svtkDistanceRepresentation3D&) = delete;

  // Internal method to update the position of the label.
  void UpdateLabelPosition();
};

#endif
