/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextActor
 * @brief   An actor that displays text. Scaled or unscaled
 *
 * svtkTextActor can be used to place text annotation into a window.
 * When TextScaleMode is NONE, the text is fixed font and operation is
 * the same as a svtkPolyDataMapper2D/svtkActor2D pair.
 * When TextScaleMode is VIEWPORT, the font resizes such that it maintains a
 * consistent size relative to the viewport in which it is rendered.
 * When TextScaleMode is PROP, the font resizes such that the text fits inside
 * the box defined by the position 1 & 2 coordinates. This class replaces the
 * deprecated svtkScaledTextActor and acts as a convenient wrapper for
 * a svtkTextMapper/svtkActor2D pair.
 * Set the text property/attributes through the svtkTextProperty associated to
 * this actor.
 *
 * @sa
 * svtkActor2D svtkPolyDataMapper svtkTextProperty svtkTextRenderer
 */

#ifndef svtkTextActor_h
#define svtkTextActor_h

#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkTexturedActor2D.h"

class svtkImageData;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkProperty2D;
class svtkTextProperty;
class svtkTextRenderer;
class svtkTransform;

class SVTKRENDERINGCORE_EXPORT svtkTextActor : public svtkTexturedActor2D
{
public:
  svtkTypeMacro(svtkTextActor, svtkTexturedActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with a rectangle in normaled view coordinates
   * of (0.2,0.85, 0.8, 0.95).
   */
  static svtkTextActor* New();

  /**
   * Shallow copy of this text actor. Overloads the virtual
   * svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  //@{
  /**
   * Set the text string to be displayed. "\n" is recognized
   * as a carriage return/linefeed (line separator).
   * The characters must be in the UTF-8 encoding.
   * Convenience method to the underlying mapper
   */
  void SetInput(const char* inputString);
  char* GetInput();
  //@}

  //@{
  /**
   * Set/Get the minimum size in pixels for this actor.
   * Defaults to 10,10.
   * Only valid when TextScaleMode is PROP.
   */
  svtkSetVector2Macro(MinimumSize, int);
  svtkGetVector2Macro(MinimumSize, int);
  //@}

  //@{
  /**
   * Set/Get the maximum height of a line of text as a
   * percentage of the vertical area allocated to this
   * scaled text actor. Defaults to 1.0.
   * Only valid when TextScaleMode is PROP.
   */
  svtkSetMacro(MaximumLineHeight, float);
  svtkGetMacro(MaximumLineHeight, float);
  //@}

  //@{
  /**
   * Set how text should be scaled.  If set to
   * svtkTextActor::TEXT_SCALE_MODE_NONE, the font size will be fixed by the
   * size given in TextProperty.  If set to svtkTextActor::TEXT_SCALE_MODE_PROP,
   * the text will be scaled to fit exactly in the prop as specified by the
   * position 1 & 2 coordinates.  If set to
   * svtkTextActor::TEXT_SCALE_MODE_VIEWPORT, the text will be scaled based on
   * the size of the viewport it is displayed in.
   */
  svtkSetClampMacro(TextScaleMode, int, TEXT_SCALE_MODE_NONE, TEXT_SCALE_MODE_VIEWPORT);
  svtkGetMacro(TextScaleMode, int);
  void SetTextScaleModeToNone() { this->SetTextScaleMode(TEXT_SCALE_MODE_NONE); }
  void SetTextScaleModeToProp() { this->SetTextScaleMode(TEXT_SCALE_MODE_PROP); }
  void SetTextScaleModeToViewport() { this->SetTextScaleMode(TEXT_SCALE_MODE_VIEWPORT); }
  //@}

  enum
  {
    TEXT_SCALE_MODE_NONE = 0,
    TEXT_SCALE_MODE_PROP,
    TEXT_SCALE_MODE_VIEWPORT
  };

  //@{
  /**
   * Turn on or off the UseBorderAlign option.
   * When UseBorderAlign is on, the bounding rectangle is used to align the text,
   * which is the proper behavior when using svtkTextRepresentation
   */
  svtkSetMacro(UseBorderAlign, svtkTypeBool);
  svtkGetMacro(UseBorderAlign, svtkTypeBool);
  svtkBooleanMacro(UseBorderAlign, svtkTypeBool);
  //@}

  //@{
  /**
   * This method is being deprecated.  Use SetJustification and
   * SetVerticalJustification in text property instead.
   * Set/Get the Alignment point
   * if zero (default), the text aligns itself to the bottom left corner
   * (which is defined by the PositionCoordinate)
   * otherwise the text aligns itself to corner/midpoint or centre
   * @verbatim
   * 6   7   8
   * 3   4   5
   * 0   1   2
   * @endverbatim
   * This is the same as setting the TextProperty's justification.
   * Currently TextActor is not oriented around its AlignmentPoint.
   */
  void SetAlignmentPoint(int point);
  int GetAlignmentPoint();
  //@}

  //@{
  /**
   * Counterclockwise rotation around the Alignment point.
   * Units are in degrees and defaults to 0.
   * The orientation in the text property rotates the text in the
   * texture map.  It will proba ly not give you the effect you
   * desire.
   */
  void SetOrientation(float orientation);
  svtkGetMacro(Orientation, float);
  //@}

  //@{
  /**
   * Set/Get the text property.
   */
  virtual void SetTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  //@}

  /**
   * Return the bounding box coordinates of the text in pixels.
   * The bbox array is populated with [ xmin, xmax, ymin, ymax ]
   * values in that order.
   */
  virtual void GetBoundingBox(svtkViewport* vport, double bbox[4]);

  /**
   * Syntactic sugar to get the size of text instead of the entire bounding box.
   */
  virtual void GetSize(svtkViewport* vport, double size[2]);

  //@{
  /**
   * Set and return the font size required to make this mapper fit in a given
   * target rectangle (width x height, in pixels). A static version of the
   * method is also available for convenience to other classes (e.g., widgets).
   */
  virtual int SetConstrainedFontSize(svtkViewport*, int targetWidth, int targetHeight);
  static int SetConstrainedFontSize(svtkTextActor*, svtkViewport*, int targetWidth, int targetHeight);
  //@}

  /**
   * Set and return the font size required to make each element of an array
   * of mappers fit in a given rectangle (width x height, in pixels).  This
   * font size is the smallest size that was required to fit the largest
   * mapper in this constraint.
   */
  static int SetMultipleConstrainedFontSize(svtkViewport*, int targetWidth, int targetHeight,
    svtkTextActor** actors, int nbOfActors, int* maxResultingSize);

  /**
   * Enable non-linear scaling of font sizes. This is useful in combination
   * with scaled text. With small windows you want to use the entire scaled
   * text area. With larger windows you want to reduce the font size some so
   * that the entire area is not used. These values modify the computed font
   * size as follows:
   * newFontSize = pow(FontSize,exponent)*pow(target,1.0 - exponent)
   * typically exponent should be around 0.7 and target should be around 10
   */
  virtual void SetNonLinearFontScale(double exponent, int target);

  /**
   * This is just a simple coordinate conversion method used in the render
   * process.
   */
  void SpecifiedToDisplay(double* pos, svtkViewport* vport, int specified);

  /**
   * This is just a simple coordinate conversion method used in the render
   * process.
   */
  void DisplayToSpecified(double* pos, svtkViewport* vport, int specified);

  /**
   * Compute the scale the font should be given the viewport.  The result
   * is placed in the ScaledTextProperty ivar.
   */
  virtual void ComputeScaledFont(svtkViewport* viewport);

  //@{
  /**
   * Get the scaled font.  Use ComputeScaledFont to set the scale for a given
   * viewport.
   */
  svtkGetObjectMacro(ScaledTextProperty, svtkTextProperty);
  //@}

  /**
   * Provide a font scaling based on a viewport.  This is the scaling factor
   * used when the TextScaleMode is set to VIEWPORT and has been made public for
   * other components to use.  This scaling assumes that the long dimension of
   * the viewport is meant to be 6 inches (a typical width of text in a paper)
   * and then resizes based on if that long dimension was 72 DPI.
   */
  static float GetFontScale(svtkViewport* viewport);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Draw the text actor to the screen.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

protected:
  /**
   * Render Input to Image using the supplied font property.
   */
  virtual bool RenderImage(svtkTextProperty* tprop, svtkViewport* viewport);

  /**
   * Get the bounding box for Input using the supplied font property.
   */
  virtual bool GetImageBoundingBox(svtkTextProperty* tprop, svtkViewport* viewport, int bbox[4]);

  svtkTextActor();
  ~svtkTextActor() override;

  int MinimumSize[2];
  float MaximumLineHeight;
  double FontScaleExponent;
  int TextScaleMode;
  float Orientation;
  svtkTypeBool UseBorderAlign;

  svtkTextProperty* TextProperty;
  svtkImageData* ImageData;
  svtkTextRenderer* TextRenderer;
  svtkTimeStamp BuildTime;
  svtkTransform* Transform;
  int LastSize[2];
  int LastOrigin[2];
  char* Input;
  bool InputRendered;
  double FormerOrientation;
  int RenderedDPI;

  svtkTextProperty* ScaledTextProperty;

  // Stuff needed to display the image text as a texture map.
  svtkPolyData* Rectangle;
  svtkPoints* RectanglePoints;

  virtual void ComputeRectangle(svtkViewport* viewport);

  /**
   * Ensure that \a Rectangle and \a RectanglePoints are valid and up-to-date.

   * Unlike ComputeRectangle(), this may do nothing (if the rectangle is valid),
   * or it may render the text to an image and recompute rectangle points by
   * calling ComputeRectangle.

   * Returns a non-zero value upon success or zero upon failure to
   * render the image.

   * This may be called with a NULL viewport when bounds are required before
   * a rendering has occurred.
   */
  virtual int UpdateRectangle(svtkViewport* viewport);

private:
  svtkTextActor(const svtkTextActor&) = delete;
  void operator=(const svtkTextActor&) = delete;
};

#endif
