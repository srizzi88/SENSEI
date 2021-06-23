/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxesActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAxesActor
 * @brief   a 3D axes representation
 *
 * svtkAxesActor is a hybrid 2D/3D actor used to represent 3D axes in a scene.
 * The user can define the geometry to use for the shaft or the tip, and the
 * user can set the text for the three axes. The text will appear to follow the
 * camera since it is implemented by means of svtkCaptionActor2D.  All of the
 * functionality of the underlying svtkCaptionActor2D objects are accessible so
 * that, for instance, the font attributes of the axes text can be manipulated
 * through svtkTextProperty. Since this class inherits from svtkProp3D, one can
 * apply a user transform to the underlying geometry and the positioning of the
 * labels. For example, a rotation transform could be used to generate a
 * left-handed axes representation.
 *
 * @par Thanks:
 * Thanks to Goodwin Lawlor for posting a tcl script which featured the
 * use of svtkCaptionActor2D to implement the text labels.  This class is
 * based on Paraview's svtkPVAxesActor.
 *
 * @warning
 * svtkAxesActor is primarily intended for use with svtkOrientationMarkerWidget.
 * The bounds of this actor are calculated as though the geometry of the axes
 * were symmetric: that is, although only positive axes are visible, bounds
 * are calculated as though negative axes are present too.  This is done
 * intentionally to implement functionality of the camera update mechanism
 * in svtkOrientationMarkerWidget.
 *
 * @sa
 * svtkAnnotatedCubeActor svtkOrientationMarkerWidget svtkCaptionActor2D
 * svtkTextProperty
 */

#ifndef svtkAxesActor_h
#define svtkAxesActor_h

#include "svtkProp3D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkActor;
class svtkCaptionActor2D;
class svtkConeSource;
class svtkCylinderSource;
class svtkLineSource;
class svtkPolyData;
class svtkPropCollection;
class svtkProperty;
class svtkRenderer;
class svtkSphereSource;

class SVTKRENDERINGANNOTATION_EXPORT svtkAxesActor : public svtkProp3D
{
public:
  static svtkAxesActor* New();
  svtkTypeMacro(svtkAxesActor, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(svtkPropCollection*) override;

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Shallow copy of an axes actor. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  void GetBounds(double bounds[6]);
  double* GetBounds() SVTK_SIZEHINT(6) override;
  //@}

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, textures
   * etc.
   */
  svtkMTimeType GetRedrawMTime() override;

  //@{
  /**
   * Set the total length of the axes in 3 dimensions.
   */
  void SetTotalLength(double v[3]) { this->SetTotalLength(v[0], v[1], v[2]); }
  void SetTotalLength(double x, double y, double z);
  svtkGetVectorMacro(TotalLength, double, 3);
  //@}

  //@{
  /**
   * Set the normalized (0-1) length of the shaft.
   */
  void SetNormalizedShaftLength(double v[3]) { this->SetNormalizedShaftLength(v[0], v[1], v[2]); }
  void SetNormalizedShaftLength(double x, double y, double z);
  svtkGetVectorMacro(NormalizedShaftLength, double, 3);
  //@}

  //@{
  /**
   * Set the normalized (0-1) length of the tip.  Normally, this would be
   * 1 - the normalized length of the shaft.
   */
  void SetNormalizedTipLength(double v[3]) { this->SetNormalizedTipLength(v[0], v[1], v[2]); }
  void SetNormalizedTipLength(double x, double y, double z);
  svtkGetVectorMacro(NormalizedTipLength, double, 3);
  //@}

  //@{
  /**
   * Set the normalized (0-1) position of the label along the length of
   * the shaft.  A value > 1 is permissible.
   */
  void SetNormalizedLabelPosition(double v[3])
  {
    this->SetNormalizedLabelPosition(v[0], v[1], v[2]);
  }
  void SetNormalizedLabelPosition(double x, double y, double z);
  svtkGetVectorMacro(NormalizedLabelPosition, double, 3);
  //@}

  //@{
  /**
   * Set/get the resolution of the pieces of the axes actor.
   */
  svtkSetClampMacro(ConeResolution, int, 3, 128);
  svtkGetMacro(ConeResolution, int);
  svtkSetClampMacro(SphereResolution, int, 3, 128);
  svtkGetMacro(SphereResolution, int);
  svtkSetClampMacro(CylinderResolution, int, 3, 128);
  svtkGetMacro(CylinderResolution, int);
  //@}

  //@{
  /**
   * Set/get the radius of the pieces of the axes actor.
   */
  svtkSetClampMacro(ConeRadius, double, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(ConeRadius, double);
  svtkSetClampMacro(SphereRadius, double, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(SphereRadius, double);
  svtkSetClampMacro(CylinderRadius, double, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(CylinderRadius, double);
  //@}

  //@{
  /**
   * Set the type of the shaft to a cylinder, line, or user defined geometry.
   */
  void SetShaftType(int type);
  void SetShaftTypeToCylinder() { this->SetShaftType(svtkAxesActor::CYLINDER_SHAFT); }
  void SetShaftTypeToLine() { this->SetShaftType(svtkAxesActor::LINE_SHAFT); }
  void SetShaftTypeToUserDefined() { this->SetShaftType(svtkAxesActor::USER_DEFINED_SHAFT); }
  svtkGetMacro(ShaftType, int);
  //@}

  //@{
  /**
   * Set the type of the tip to a cone, sphere, or user defined geometry.
   */
  void SetTipType(int type);
  void SetTipTypeToCone() { this->SetTipType(svtkAxesActor::CONE_TIP); }
  void SetTipTypeToSphere() { this->SetTipType(svtkAxesActor::SPHERE_TIP); }
  void SetTipTypeToUserDefined() { this->SetTipType(svtkAxesActor::USER_DEFINED_TIP); }
  svtkGetMacro(TipType, int);
  //@}

  //@{
  /**
   * Set the user defined tip polydata.
   */
  void SetUserDefinedTip(svtkPolyData*);
  svtkGetObjectMacro(UserDefinedTip, svtkPolyData);
  //@}

  //@{
  /**
   * Set the user defined shaft polydata.
   */
  void SetUserDefinedShaft(svtkPolyData*);
  svtkGetObjectMacro(UserDefinedShaft, svtkPolyData);
  //@}

  //@{
  /**
   * Get the tip properties.
   */
  svtkProperty* GetXAxisTipProperty();
  svtkProperty* GetYAxisTipProperty();
  svtkProperty* GetZAxisTipProperty();
  //@}

  //@{
  /**
   * Get the shaft properties.
   */
  svtkProperty* GetXAxisShaftProperty();
  svtkProperty* GetYAxisShaftProperty();
  svtkProperty* GetZAxisShaftProperty();
  //@}

  /**
   * Retrieve handles to the X, Y and Z axis (so that you can set their text
   * properties for example)
   */
  svtkCaptionActor2D* GetXAxisCaptionActor2D() { return this->XAxisLabel; }
  svtkCaptionActor2D* GetYAxisCaptionActor2D() { return this->YAxisLabel; }
  svtkCaptionActor2D* GetZAxisCaptionActor2D() { return this->ZAxisLabel; }

  //@{
  /**
   * Set/get the label text.
   */
  svtkSetStringMacro(XAxisLabelText);
  svtkGetStringMacro(XAxisLabelText);
  svtkSetStringMacro(YAxisLabelText);
  svtkGetStringMacro(YAxisLabelText);
  svtkSetStringMacro(ZAxisLabelText);
  svtkGetStringMacro(ZAxisLabelText);
  //@}

  //@{
  /**
   * Enable/disable drawing the axis labels.
   */
  svtkSetMacro(AxisLabels, svtkTypeBool);
  svtkGetMacro(AxisLabels, svtkTypeBool);
  svtkBooleanMacro(AxisLabels, svtkTypeBool);
  //@}

  enum
  {
    CYLINDER_SHAFT,
    LINE_SHAFT,
    USER_DEFINED_SHAFT
  };

  enum
  {
    CONE_TIP,
    SPHERE_TIP,
    USER_DEFINED_TIP
  };

protected:
  svtkAxesActor();
  ~svtkAxesActor() override;

  svtkCylinderSource* CylinderSource;
  svtkLineSource* LineSource;
  svtkConeSource* ConeSource;
  svtkSphereSource* SphereSource;

  svtkActor* XAxisShaft;
  svtkActor* YAxisShaft;
  svtkActor* ZAxisShaft;

  svtkActor* XAxisTip;
  svtkActor* YAxisTip;
  svtkActor* ZAxisTip;

  void UpdateProps();

  double TotalLength[3];
  double NormalizedShaftLength[3];
  double NormalizedTipLength[3];
  double NormalizedLabelPosition[3];

  int ShaftType;
  int TipType;

  svtkPolyData* UserDefinedTip;
  svtkPolyData* UserDefinedShaft;

  char* XAxisLabelText;
  char* YAxisLabelText;
  char* ZAxisLabelText;

  svtkCaptionActor2D* XAxisLabel;
  svtkCaptionActor2D* YAxisLabel;
  svtkCaptionActor2D* ZAxisLabel;

  svtkTypeBool AxisLabels;

  int ConeResolution;
  int SphereResolution;
  int CylinderResolution;

  double ConeRadius;
  double SphereRadius;
  double CylinderRadius;

private:
  svtkAxesActor(const svtkAxesActor&) = delete;
  void operator=(const svtkAxesActor&) = delete;
};

#endif
