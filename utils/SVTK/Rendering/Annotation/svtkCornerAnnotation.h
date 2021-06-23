/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCornerAnnotation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCornerAnnotation
 * @brief   text annotation in four corners
 *
 * This is an annotation object that manages four text actors / mappers
 * to provide annotation in the four corners of a viewport
 *
 * @par Special input text::
 * - <image> : will be replaced with slice number (relative number)
 * - <slice> : will be replaced with slice number (relative number)
 * - <image_and_max> : will be replaced with slice number and slice max (relative)
 * - <slice_and_max> : will be replaced with slice number and slice max (relative)
 * - <slice_pos> : will be replaced by the position of the current slice
 * - <window> : will be replaced with window value
 * - <level> : will be replaced with level value
 * - <window_level> : will be replaced with window and level value
 *
 * @sa
 * svtkActor2D svtkTextMapper
 */

#ifndef svtkCornerAnnotation_h
#define svtkCornerAnnotation_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkTextMapper;
class svtkImageMapToWindowLevelColors;
class svtkImageActor;
class svtkTextProperty;

class SVTKRENDERINGANNOTATION_EXPORT svtkCornerAnnotation : public svtkActor2D
{
public:
  svtkTypeMacro(svtkCornerAnnotation, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with a rectangle in normaled view coordinates
   * of (0.2,0.85, 0.8, 0.95).
   */
  static svtkCornerAnnotation* New();

  //@{
  /**
   * Draw the scalar bar and annotation text to the screen.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  //@{
  /**
   * Set/Get the maximum height of a line of text as a
   * percentage of the vertical area allocated to this
   * scaled text actor. Defaults to 1.0
   */
  svtkSetMacro(MaximumLineHeight, double);
  svtkGetMacro(MaximumLineHeight, double);
  //@}

  //@{
  /**
   * Set/Get the minimum/maximum size font that will be shown.
   * If the font drops below the minimum size it will not be rendered.
   */
  svtkSetMacro(MinimumFontSize, int);
  svtkGetMacro(MinimumFontSize, int);
  svtkSetMacro(MaximumFontSize, int);
  svtkGetMacro(MaximumFontSize, int);
  //@}

  //@{
  /**
   * Set/Get font scaling factors
   * The font size, f, is calculated as the largest possible value
   * such that the annotations for the given viewport do not overlap.
   * This font size is scaled non-linearly with the viewport size,
   * to maintain an acceptable readable size at larger viewport sizes,
   * without being too big.
   * f' = linearScale * pow(f,nonlinearScale)
   */
  svtkSetMacro(LinearFontScaleFactor, double);
  svtkGetMacro(LinearFontScaleFactor, double);
  svtkSetMacro(NonlinearFontScaleFactor, double);
  svtkGetMacro(NonlinearFontScaleFactor, double);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * Position used to get or set the corner annotation text.
   * \sa GetText(), SetText()
   */
  enum TextPosition
  {
    LowerLeft = 0, ///< Uses the lower left corner.
    LowerRight,    ///< Uses the lower right corner.
    UpperLeft,     ///< Uses the upper left corner.
    UpperRight,    ///< Uses the upper right corner.
    LowerEdge,     ///< Uses the lower edge center.
    RightEdge,     ///< Uses the right edge center.
    LeftEdge,      ///< Uses the left edge center
    UpperEdge      ///< Uses the upper edge center.
  };
  static const int NumTextPositions = 8;
  //@}

  //@{
  /**
   * Set/Get the text to be displayed for each corner
   * \sa TextPosition
   */
  void SetText(int i, const char* text);
  const char* GetText(int i);
  void ClearAllTexts();
  void CopyAllTextsFrom(svtkCornerAnnotation* ca);
  //@}

  //@{
  /**
   * Set an image actor to look at for slice information
   */
  void SetImageActor(svtkImageActor*);
  svtkGetObjectMacro(ImageActor, svtkImageActor);
  //@}

  //@{
  /**
   * Set an instance of svtkImageMapToWindowLevelColors to use for
   * looking at window level changes
   */
  void SetWindowLevel(svtkImageMapToWindowLevelColors*);
  svtkGetObjectMacro(WindowLevel, svtkImageMapToWindowLevelColors);
  //@}

  //@{
  /**
   * Set the value to shift the level by.
   */
  svtkSetMacro(LevelShift, double);
  svtkGetMacro(LevelShift, double);
  //@}

  //@{
  /**
   * Set the value to scale the level by.
   */
  svtkSetMacro(LevelScale, double);
  svtkGetMacro(LevelScale, double);
  //@}

  //@{
  /**
   * Set/Get the text property of all corners.
   */
  virtual void SetTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Even if there is an image actor, should `slice' and `image' be displayed?
   */
  svtkBooleanMacro(ShowSliceAndImage, svtkTypeBool);
  svtkSetMacro(ShowSliceAndImage, svtkTypeBool);
  svtkGetMacro(ShowSliceAndImage, svtkTypeBool);
  //@}

protected:
  svtkCornerAnnotation();
  ~svtkCornerAnnotation() override;

  double MaximumLineHeight;

  svtkTextProperty* TextProperty;

  svtkImageMapToWindowLevelColors* WindowLevel;
  double LevelShift;
  double LevelScale;
  svtkImageActor* ImageActor;
  svtkImageActor* LastImageActor;

  char* CornerText[NumTextPositions];

  int FontSize;
  svtkActor2D* TextActor[NumTextPositions];
  svtkTimeStamp BuildTime;
  int LastSize[2];
  svtkTextMapper* TextMapper[NumTextPositions];

  int MinimumFontSize;
  int MaximumFontSize;

  double LinearFontScaleFactor;
  double NonlinearFontScaleFactor;

  svtkTypeBool ShowSliceAndImage;

  /**
   * Search for replaceable tokens and replace
   */
  virtual void TextReplace(svtkImageActor* ia, svtkImageMapToWindowLevelColors* wl);

  //@{
  /**
   * Set text actor positions given a viewport size and justification
   */
  virtual void SetTextActorsPosition(const int vsize[2]);
  virtual void SetTextActorsJustification();
  //@}

private:
  svtkCornerAnnotation(const svtkCornerAnnotation&) = delete;
  void operator=(const svtkCornerAnnotation&) = delete;
};

#endif
