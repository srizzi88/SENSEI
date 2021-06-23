/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarBarActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkScalarBarActor
 * @brief   Create a scalar bar with labels
 *
 * svtkScalarBarActor creates a scalar bar with tick marks. A scalar
 * bar is a legend that indicates to the viewer the correspondence between
 * color value and data value. The legend consists of a rectangular bar
 * made of rectangular pieces each colored a constant value. Since
 * svtkScalarBarActor is a subclass of svtkActor2D, it is drawn in the image
 * plane (i.e., in the renderer's viewport) on top of the 3D graphics window.
 *
 * To use svtkScalarBarActor you must associate a svtkScalarsToColors (or
 * subclass) with it. The lookup table defines the colors and the
 * range of scalar values used to map scalar data.  Typically, the
 * number of colors shown in the scalar bar is not equal to the number
 * of colors in the lookup table, in which case sampling of
 * the lookup table is performed.
 *
 * Other optional capabilities include specifying the fraction of the
 * viewport size (both x and y directions) which will control the size
 * of the scalar bar and the number of tick labels. The actual position
 * of the scalar bar on the screen is controlled by using the
 * svtkActor2D::SetPosition() method (by default the scalar bar is
 * centered in the viewport).  Other features include the ability to
 * orient the scalar bar horizontally of vertically and controlling
 * the format (printf style) with which to print the labels on the
 * scalar bar. Also, the svtkScalarBarActor's property is applied to
 * the scalar bar and annotations (including layer, and
 * compositing operator).
 *
 * Set the text property/attributes of the title and the labels through the
 * svtkTextProperty objects associated to this actor.
 *
 * @warning
 * If a svtkLogLookupTable is specified as the lookup table to use, then the
 * labels are created using a logarithmic scale.
 *
 * @sa
 * svtkActor2D svtkTextProperty svtkTextMapper svtkPolyDataMapper2D
 */

#ifndef svtkScalarBarActor_h
#define svtkScalarBarActor_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkColor3ub;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkProperty2D;
class svtkScalarsToColors;
class svtkScalarBarActorInternal;
class svtkTextActor;
class svtkTextMapper;
class svtkTextProperty;
class svtkTexture;
class svtkTexturedActor2D;

#define SVTK_ORIENT_HORIZONTAL 0
#define SVTK_ORIENT_VERTICAL 1

class SVTKRENDERINGANNOTATION_EXPORT svtkScalarBarActor : public svtkActor2D
{
public:
  svtkTypeMacro(svtkScalarBarActor, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with 64 maximum colors; 5 labels; %%-#6.3g label
   * format, no title, and vertical orientation. The initial scalar bar
   * size is (0.05 x 0.8) of the viewport size.
   */
  static svtkScalarBarActor* New();

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

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Fills rect with the dimensions of the scalar bar in viewport coordinates.
   * Only the color bar is considered -- text labels are not considered.
   * rect is {xmin, xmax, width, height}
   */
  virtual void GetScalarBarRect(int rect[4], svtkViewport* viewport);

  //@{
  /**
   * Set/Get the lookup table to use. The lookup table specifies the number
   * of colors to use in the table (if not overridden), the scalar range,
   * and any annotated values.
   * Annotated values are rendered using svtkTextActor.
   */
  virtual void SetLookupTable(svtkScalarsToColors*);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * Should be display the opacity as well. This is displayed by changing
   * the opacity of the scalar bar in accordance with the opacity of the
   * given color. For clarity, a texture grid is placed in the background
   * if Opacity is ON. You might also want to play with SetTextureGridWith
   * in that case. [Default: off]
   */
  svtkSetMacro(UseOpacity, svtkTypeBool);
  svtkGetMacro(UseOpacity, svtkTypeBool);
  svtkBooleanMacro(UseOpacity, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the maximum number of scalar bar segments to show. This may
   * differ from the number of colors in the lookup table, in which case
   * the colors are samples from the lookup table.
   */
  svtkSetClampMacro(MaximumNumberOfColors, int, 2, SVTK_INT_MAX);
  svtkGetMacro(MaximumNumberOfColors, int);
  //@}

  //@{
  /**
   * Set/Get the number of tick labels to show.
   */
  svtkSetClampMacro(NumberOfLabels, int, 0, 64);
  svtkGetMacro(NumberOfLabels, int);
  //@}

  //@{
  /**
   * Control the orientation of the scalar bar.
   */
  svtkSetClampMacro(Orientation, int, SVTK_ORIENT_HORIZONTAL, SVTK_ORIENT_VERTICAL);
  svtkGetMacro(Orientation, int);
  void SetOrientationToHorizontal() { this->SetOrientation(SVTK_ORIENT_HORIZONTAL); }
  void SetOrientationToVertical() { this->SetOrientation(SVTK_ORIENT_VERTICAL); }
  //@}

  //@{
  /**
   * Set/Get the title text property.
   */
  virtual void SetTitleTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TitleTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the labels text property.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the annotation text property.
   */
  virtual void SetAnnotationTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(AnnotationTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on the scalar
   * bar.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set/Get the title of the scalar bar actor,
   */
  svtkSetStringMacro(Title);
  svtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the title for the component that is selected,
   */
  svtkSetStringMacro(ComponentTitle);
  svtkGetStringMacro(ComponentTitle);
  //@}

  /**
   * Shallow copy of a scalar bar actor. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  //@{
  /**
   * Set the width of the texture grid. Used only if UseOpacity is ON.
   */
  svtkSetMacro(TextureGridWidth, double);
  svtkGetMacro(TextureGridWidth, double);
  //@}

  //@{
  /**
   * Get the texture actor.. you may want to change some properties on it
   */
  svtkGetObjectMacro(TextureActor, svtkTexturedActor2D);
  //@}

  enum
  {
    PrecedeScalarBar = 0,
    SucceedScalarBar
  };

  //@{
  /**
   * Should the title and tick marks precede the scalar bar or succeed it?
   * This is measured along the viewport coordinate direction perpendicular
   * to the long axis of the scalar bar, not the reading direction.
   * Thus, succeed implies the that the text is above scalar bar if
   * the orientation is horizontal or right of scalar bar if the orientation
   * is vertical. Precede is the opposite.
   */
  svtkSetClampMacro(TextPosition, int, PrecedeScalarBar, SucceedScalarBar);
  svtkGetMacro(TextPosition, int);
  virtual void SetTextPositionToPrecedeScalarBar()
  {
    this->SetTextPosition(svtkScalarBarActor::PrecedeScalarBar);
  }
  virtual void SetTextPositionToSucceedScalarBar()
  {
    this->SetTextPosition(svtkScalarBarActor::SucceedScalarBar);
  }
  //@}

  //@{
  /**
   * Set/Get the maximum width and height in pixels. Specifying the size as
   * a relative fraction of the viewport can sometimes undesirably stretch
   * the size of the actor too much. These methods allow the user to set
   * bounds on the maximum size of the scalar bar in pixels along any
   * direction. Defaults to unbounded.
   */
  svtkSetMacro(MaximumWidthInPixels, int);
  svtkGetMacro(MaximumWidthInPixels, int);
  svtkSetMacro(MaximumHeightInPixels, int);
  svtkGetMacro(MaximumHeightInPixels, int);
  //@}

  //@{
  /**
   * Set/get the padding between the scalar bar and the text annotations.
   * This space is used to draw leader lines.
   * The default is 8 pixels.
   */
  svtkSetMacro(AnnotationLeaderPadding, double);
  svtkGetMacro(AnnotationLeaderPadding, double);
  //@}

  //@{
  /**
   * Set/get whether text annotations should be rendered or not.
   * Currently, this only affects rendering when \a IndexedLookup is true.
   * The default is true.
   */
  svtkSetMacro(DrawAnnotations, svtkTypeBool);
  svtkGetMacro(DrawAnnotations, svtkTypeBool);
  svtkBooleanMacro(DrawAnnotations, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get whether the NaN annotation should be rendered or not.
   * This only affects rendering when \a DrawAnnotations is true.
   * The default is false.
   */
  svtkSetMacro(DrawNanAnnotation, svtkTypeBool);
  svtkGetMacro(DrawNanAnnotation, svtkTypeBool);
  svtkBooleanMacro(DrawNanAnnotation, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get whether the Below range swatch should be rendered or not.
   * This only affects rendering when \a DrawAnnotations is true.
   * The default is false.
   */
  svtkSetMacro(DrawBelowRangeSwatch, bool);
  svtkGetMacro(DrawBelowRangeSwatch, bool);
  svtkBooleanMacro(DrawBelowRangeSwatch, bool);
  //@}

  //@{
  /**
   * Set/get the annotation text for "Below Range" values.
   */
  svtkSetStringMacro(BelowRangeAnnotation);
  svtkGetStringMacro(BelowRangeAnnotation);
  //@}

  //@{
  /**
   * Set/get whether the Above range swatch should be rendered or not.
   * This only affects rendering when \a DrawAnnotations is true.
   * The default is false.
   */
  svtkSetMacro(DrawAboveRangeSwatch, bool);
  svtkGetMacro(DrawAboveRangeSwatch, bool);
  svtkBooleanMacro(DrawAboveRangeSwatch, bool);
  //@}

  //@{
  /**
   * Set/get the annotation text for "Above Range Swatch" values.
   */
  svtkSetStringMacro(AboveRangeAnnotation);
  svtkGetStringMacro(AboveRangeAnnotation);
  //@}
  //@{
  /**
   * Set/get how leader lines connecting annotations to values should be colored.

   * When true, leader lines are all the same color (and match the LabelTextProperty color).
   * When false, leader lines take on the color of the value they correspond to.
   * This only affects rendering when \a DrawAnnotations is true.
   * The default is false.
   */
  svtkSetMacro(FixedAnnotationLeaderLineColor, svtkTypeBool);
  svtkGetMacro(FixedAnnotationLeaderLineColor, svtkTypeBool);
  svtkBooleanMacro(FixedAnnotationLeaderLineColor, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the annotation text for "NaN" values.
   */
  svtkSetStringMacro(NanAnnotation);
  svtkGetStringMacro(NanAnnotation);
  //@}

  //@{
  /**
   * Set/get whether annotation labels should be scaled with the viewport.

   * The default value is 0 (no scaling).
   * If non-zero, the svtkTextActor instances used to render annotation
   * labels will have their TextScaleMode set to viewport-based scaling,
   * which nonlinearly scales font size with the viewport size.
   */
  svtkSetMacro(AnnotationTextScaling, svtkTypeBool);
  svtkGetMacro(AnnotationTextScaling, svtkTypeBool);
  svtkBooleanMacro(AnnotationTextScaling, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether a background should be drawn around the scalar bar.
   * Default is off.
   */
  svtkSetMacro(DrawBackground, svtkTypeBool);
  svtkGetMacro(DrawBackground, svtkTypeBool);
  svtkBooleanMacro(DrawBackground, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether a frame should be drawn around the scalar bar.
   * Default is off.
   */
  svtkSetMacro(DrawFrame, svtkTypeBool);
  svtkGetMacro(DrawFrame, svtkTypeBool);
  svtkBooleanMacro(DrawFrame, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether the color bar should be drawn. If off, only the tickmarks
   * and text will be drawn. Default is on.
   */
  svtkSetMacro(DrawColorBar, svtkTypeBool);
  svtkGetMacro(DrawColorBar, svtkTypeBool);
  svtkBooleanMacro(DrawColorBar, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether the tick labels should be drawn. Default is on.
   */
  svtkSetMacro(DrawTickLabels, svtkTypeBool);
  svtkGetMacro(DrawTickLabels, svtkTypeBool);
  svtkBooleanMacro(DrawTickLabels, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the background property.
   */
  virtual void SetBackgroundProperty(svtkProperty2D* p);
  svtkGetObjectMacro(BackgroundProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Set/Get the frame property.
   */
  virtual void SetFrameProperty(svtkProperty2D* p);
  svtkGetObjectMacro(FrameProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Set/get the amount of padding around text boxes.
   * The default is 1 pixel.
   */
  svtkGetMacro(TextPad, int);
  svtkSetMacro(TextPad, int);
  //@}

  //@{
  /**
   * Set/get the margin in pixels, between the title and the bar,
   * when the \a Orientation is vertical.
   * The default is 0 pixels.
   */
  svtkGetMacro(VerticalTitleSeparation, int);
  svtkSetMacro(VerticalTitleSeparation, int);
  //@}

  //@{
  /**
   * Set/get the thickness of the color bar relative to the widget frame.
   * The default is 0.375 and must always be in the range ]0, 1[.
   */
  svtkGetMacro(BarRatio, double);
  svtkSetClampMacro(BarRatio, double, 0., 1.);
  //@}

  //@{
  /**
   * Set/get the ratio of the title height to the tick label height
   * (used only when the \a Orientation is horizontal).
   * The default is 0.5, which attempts to make the labels and title
   * the same size. This must be a number in the range ]0, 1[.
   */
  svtkGetMacro(TitleRatio, double);
  svtkSetClampMacro(TitleRatio, double, 0., 1.);
  //@}

  //@{
  /**
   * Set/Get whether the font size of title and labels is unconstrained. Default is off.
   * When it is constrained, the size of the scalar bar will constrained the font size
   * When it is not, the size of the font will always be respected
   */
  svtkSetMacro(UnconstrainedFontSize, bool);
  svtkGetMacro(UnconstrainedFontSize, bool);
  svtkBooleanMacro(UnconstrainedFontSize, bool);
  //@}

protected:
  svtkScalarBarActor();
  ~svtkScalarBarActor() override;

  /**
   * Called from within \a RenderOpaqueGeometry when the internal state
   * members need to be updated before rendering.

   * This method invokes many virtual methods that first lay out the
   * scalar bar and then use the layout to position actors and create
   * datasets used to represent the scalar bar.
   * Specifically, it invokes: FreeLayoutStorage, ComputeFrame,
   * ComputeScalarBarThickness, LayoutNanSwatch, PrepareTitleText,
   * LayoutTitle, ComputeScalarBarLength, LayoutTicks, and LayoutAnnotations
   * to perform the layout step.
   * Then, it invokes ConfigureAnnotations, ConfigureFrame,
   * ConfigureScalarBar, ConfigureTitle, ConfigureTicks, and
   * ConfigureNanSwatch to create and position actors used to render
   * portions of the scalar bar.

   * By overriding one or more of these virtual methods, subclasses
   * may change the appearance of the scalar bar.

   * In the layout phase, text actors must have their text properties
   * and input strings updated, but the position of the actors should
   * not be set or relied upon as subsequent layout steps may alter
   * their placement.
   */
  virtual void RebuildLayout(svtkViewport* viewport);

  /**
   * Calls RebuildLayout if it is needed such as when
   * positions etc have changed. Return 1 on success
   * zero on error
   */
  virtual int RebuildLayoutIfNeeded(svtkViewport* viewport);

  /**
   * Free internal storage used by the previous layout.
   */
  virtual void FreeLayoutStorage();

  /**
   * If the scalar bar should be inset into a frame or rendered with a
   * solid background, this method will inset the outermost scalar bar
   * rectangle by a small amount to avoid having the scalar bar
   * illustration overlap any edges.

   * This method must set the frame coordinates (this->P->Frame).
   */
  virtual void ComputeFrame();

  /**
   * Determine how thick the scalar bar should be (on an axis perpendicular
   * to the direction in which scalar values vary).

   * This method must set the scalar bar thickness
   * (this->P->ScalarBarBox.Size[0]).
   * It may depend on layout performed by ComputeFrame
   * (i.e., the frame coordinates in this->P->Frame).
   */
  virtual void ComputeScalarBarThickness();

  /**
   * Compute a correct SwatchPad
   */
  virtual void ComputeSwatchPad();

  // This method must set this->P->NanSwatchSize and this->P->NanBox.
  // It may depend on layout performed by ComputeScalarBarThickness.
  virtual void LayoutNanSwatch();

  /**
   * Determine the size of the Below Range if it is to be rendered.

   * This method must set this->P->BelowSwatchSize and this->P->BelowBox.
   * It may depend on layout performed by ComputeScalarBarThickness.
   */
  virtual void LayoutBelowRangeSwatch();

  /**
   * Determine the size of the Above Range if it is to be rendered.

   * This method must set this->P->AboveBox.
   * It may depend on layout performed by ComputeScalarBarThickness.
   */
  virtual void LayoutAboveRangeSwatch();

  /**
   * Determine the position of the Above Range if it is to be rendered.

   * This method must set this->P->AboveRangeSize.
   * It may depend on layout performed by ComputeScalarBarLength.
   */
  virtual void LayoutAboveRangeSwatchPosn();

  /**
   * Set the title actor's input to the latest title (and subtitle) text.
   */
  virtual void PrepareTitleText();

  /**
   * Determine the position and size of the scalar bar title box.

   * This method must set this->P->TitleBox
   * It may depend on layout performed by LayoutNanSwatch.
   * If useTickBox is true, it should increase the target area
   * for the label to touch the tick box. It is called in this
   * way when the tick labels are small due to constraints other
   * than the title box.
   */
  virtual void LayoutTitle();

  /**
   * This method sets the title and tick-box size and position
   * for the UnconstrainedFontSize mode.
   */
  virtual void LayoutForUnconstrainedFont();

  /**
   * Determine how long the scalar bar should be (on an axis parallel
   * to the direction in which scalar values vary).

   * This method must set this->P->ScalarBarBox.Size[1] and should
   * estimate this->P->ScalarBarBox.Posn.
   * It may depend on layout performed by LayoutTitle.
   */
  virtual void ComputeScalarBarLength();

  /**
   * Determine the size and placement of any tick marks to be rendered.

   * This method must set this->P->TickBox.
   * It may depend on layout performed by ComputeScalarBarLength.

   * The default implementation creates exactly this->NumberOfLabels
   * tick marks, uniformly spaced on a linear or logarithmic scale.
   */
  virtual void LayoutTicks();

  /**
   * This method must lay out annotation text and leader lines so
   * they do not overlap.

   * This method must set this->P->AnnotationAnchors.
   * It may depend on layout performed by LayoutTicks.
   */
  virtual void LayoutAnnotations();

  /**
   * Generate/configure the annotation labels using the laid-out geometry.
   */
  virtual void ConfigureAnnotations();

  /**
   * Generate/configure the representation of the frame from laid-out geometry.
   */
  virtual void ConfigureFrame();

  /**
   * For debugging, add placement boxes to the frame polydata.
   */
  virtual void DrawBoxes();

  /**
   * Generate/configure the scalar bar representation from laid-out geometry.
   */
  virtual void ConfigureScalarBar();

  /**
   * Generate/configure the title actor using the laid-out geometry.
   */
  virtual void ConfigureTitle();

  /**
   * Generate/configure the tick-mark actors using the laid-out geometry.
   */
  virtual void ConfigureTicks();

  /**
   * Generate/configure the NaN swatch using the laid-out geometry.

   * Currently the NaN swatch is rendered by the same actor as the scalar bar.
   * This may change in the future.
   */
  virtual void ConfigureNanSwatch();

  /**
   * Generate/configure the above/below range swatch using the laid-out
   * geometry.
   */
  virtual void ConfigureAboveBelowRangeSwatch(bool above);

  /**
   * Subclasses may override this method to alter this->P->Labels, allowing
   * the addition and removal of annotations. The member maps viewport coordinates
   * along the long axis of the scalar bar to text (which may include MathText;
   * see svtkTextActor). It is a single-valued map, so you must perturb
   * the coordinate if you wish multiple labels to annotate the same position.
   * Each entry in this->P->Labels must have a matching entry in this->P->LabelColors.
   */
  virtual void EditAnnotations() {}

  /**
   * Compute the best size for the legend title.

   * This guarantees that the title will fit within the frame defined by Position and Position2.
   */
  virtual void SizeTitle(double* titleSize, int* size, svtkViewport* viewport);

  /**
   * Allocate actors for lookup table annotations and position them properly.
   */
  int MapAnnotationLabels(
    svtkScalarsToColors* lkup, double start, double delta, const double* range);

  /**
   * This method is called by \a ConfigureAnnotationLabels when Orientation is SVTK_ORIENT_VERTICAL.
   */
  int PlaceAnnotationsVertically(
    double barX, double barY, double barWidth, double barHeight, double delta, double pad);
  /**
   * This method is called by \a ConfigureAnnotationLabels when Orientation is
   * SVTK_ORIENT_HORIZONTAL.
   */
  int PlaceAnnotationsHorizontally(
    double barX, double barY, double barWidth, double barHeight, double delta, double pad);

  /// User-changeable settings
  //@{
  int MaximumNumberOfColors;
  int NumberOfLabels;
  int NumberOfLabelsBuilt;
  int Orientation;
  svtkTypeBool DrawBackground; // off by default
  svtkTypeBool DrawFrame;      // off by default
  svtkTypeBool DrawColorBar;   // on by default
  svtkTypeBool DrawTickLabels; // on by default
  svtkTypeBool DrawAnnotations;
  svtkTypeBool DrawNanAnnotation;
  svtkTypeBool AnnotationTextScaling; // off by default
  svtkTypeBool FixedAnnotationLeaderLineColor;
  svtkProperty2D* BackgroundProperty;
  svtkProperty2D* FrameProperty;
  char* Title;
  char* ComponentTitle;
  char* LabelFormat;
  svtkTypeBool UseOpacity; // off by default
  double TextureGridWidth;
  int TextPosition;
  char* NanAnnotation;
  char* BelowRangeAnnotation;
  char* AboveRangeAnnotation;
  double AnnotationLeaderPadding;
  int MaximumWidthInPixels;
  int MaximumHeightInPixels;
  int TextPad;
  int VerticalTitleSeparation;
  double BarRatio;
  double TitleRatio;
  bool UnconstrainedFontSize; // off by default

  bool DrawBelowRangeSwatch;
  bool DrawAboveRangeSwatch;
  //@}

  /// Internal state used for rendering
  //@{
  svtkTimeStamp BuildTime; //!< Last time internal state changed.
  int LastSize[2];        //!< Projected size in viewport coordinates of last build.
  int LastOrigin[2];      //!< Projected origin (viewport coordinates) of last build.

  svtkScalarBarActorInternal* P; //!< Containers shared with subclasses

  svtkScalarsToColors* LookupTable; //!< The object this actor illustrates

  svtkTextProperty* TitleTextProperty;      //!< Font for the legend title.
  svtkTextProperty* LabelTextProperty;      //!< Font for tick labels.
  svtkTextProperty* AnnotationTextProperty; //!< Font for annotation labels.
  svtkTextActor* TitleActor;                //!< The legend title text renderer.

  svtkPolyData* ScalarBar;               //!< Polygon(s) colored by \a LookupTable.
  svtkPolyDataMapper2D* ScalarBarMapper; //!< Mapper for \a ScalarBar.
  svtkActor2D* ScalarBarActor;           //!< Actor for \a ScalarBar.
  svtkPolyData* TexturePolyData;         //!< Polygon colored when UseOpacity is true.
  svtkTexture* Texture;                  //!< Color data for \a TexturePolyData.
  svtkTexturedActor2D* TextureActor;     //!< Actor for \a TexturePolyData.

  svtkPolyData* Background;               //!< Polygon used to fill the background.
  svtkPolyDataMapper2D* BackgroundMapper; //!< Mapper for \a Background.
  svtkActor2D* BackgroundActor;           //!< Actor for \a Background.

  svtkPolyData* Frame;               //!< Polyline used to highlight frame.
  svtkPolyDataMapper2D* FrameMapper; //!< Mapper for \a Frame.
  svtkActor2D* FrameActor;           //!< Actor for \a Frame.
  //@}

private:
  svtkScalarBarActor(const svtkScalarBarActor&) = delete;
  void operator=(const svtkScalarBarActor&) = delete;
};

#endif
