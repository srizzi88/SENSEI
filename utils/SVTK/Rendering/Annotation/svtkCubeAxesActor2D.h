/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCubeAxesActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCubeAxesActor2D
 * @brief   create a 2D plot of a bounding box edges - used for navigation
 *
 * svtkCubeAxesActor2D is a composite actor that draws three axes of the
 * bounding box of an input dataset. The axes include labels and titles
 * for the x-y-z axes. The algorithm selects the axes that are on the
 * "exterior" of the bounding box, exterior as determined from examining
 * outer edges of the bounding box in projection (display) space. Alternatively,
 * the edges closest to the viewer (i.e., camera position) can be drawn.
 *
 * To use this object you must define a bounding box and the camera used
 * to render the svtkCubeAxesActor2D. The camera is used to control the
 * scaling and position of the svtkCubeAxesActor2D so that it fits in the
 * viewport and always remains visible.)
 *
 * The font property of the axes titles and labels can be modified through the
 * AxisTitleTextProperty and AxisLabelTextProperty attributes. You may also
 * use the GetXAxisActor2D, GetYAxisActor2D or GetZAxisActor2D methods
 * to access each individual axis actor to modify their font properties.
 *
 * The bounding box to use is defined in one of three ways. First, if the Input
 * ivar is defined, then the input dataset's bounds is used. If the Input is
 * not defined, and the Prop (superclass of all actors) is defined, then the
 * Prop's bounds is used. If neither the Input or Prop is defined, then the
 * Bounds instance variable (an array of six doubles) is used.
 *
 * @sa
 * svtkActor2D svtkAxisActor2D svtkXYPlotActor svtkTextProperty
 */

#ifndef svtkCubeAxesActor2D_h
#define svtkCubeAxesActor2D_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAlgorithmOutput;
class svtkAxisActor2D;
class svtkCamera;
class svtkCubeAxesActor2DConnection;
class svtkDataSet;
class svtkTextProperty;

class SVTKRENDERINGANNOTATION_EXPORT svtkCubeAxesActor2D : public svtkActor2D
{
public:
  svtkTypeMacro(svtkCubeAxesActor2D, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with bold, italic, and shadow enabled; font family
   * set to Arial; and label format "6.3g". The number of labels per axis
   * is set to 3.
   */
  static svtkCubeAxesActor2D* New();

  //@{
  /**
   * Draw the axes as per the svtkProp superclass' API.
   */
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  //@{
  /**
   * Use the bounding box of this input dataset to draw the cube axes. If this
   * is not specified, then the class will attempt to determine the bounds from
   * the defined Prop or Bounds.
   */
  virtual void SetInputConnection(svtkAlgorithmOutput*);
  virtual void SetInputData(svtkDataSet*);
  virtual svtkDataSet* GetInput();
  //@}

  //@{
  /**
   * Use the bounding box of this prop to draw the cube axes. The
   * ViewProp is used to determine the bounds only if the Input is not
   * defined.
   */
  void SetViewProp(svtkProp* prop);
  svtkGetObjectMacro(ViewProp, svtkProp);
  //@}

  //@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds is used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   */
  svtkSetVector6Macro(Bounds, double);
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetBounds(
    double& xmin, double& xmax, double& ymin, double& ymax, double& zmin, double& zmax);
  void GetBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Explicitly specify the range of values used on the bounds.
   * The ranges are specified according to (xmin,xmax, ymin,ymax, zmin,zmax),
   * making sure that the min's are less than the max's.
   */
  svtkSetVector6Macro(Ranges, double);
  double* GetRanges() SVTK_SIZEHINT(6);
  void GetRanges(
    double& xmin, double& xmax, double& ymin, double& ymax, double& zmin, double& zmax);
  void GetRanges(double ranges[6]);
  //@}

  //@{
  /**
   * Explicitly specify an origin for the axes. These usually intersect at one of the
   * corners of the bounding box, however users have the option to override this if
   * necessary
   */
  svtkSetMacro(XOrigin, double);
  svtkSetMacro(YOrigin, double);
  svtkSetMacro(ZOrigin, double);
  //@}

  //@{
  /**
   * Set/Get a flag that controls whether the axes use the data ranges
   * or the ranges set by SetRanges. By default the axes use the data
   * ranges.
   */
  svtkSetMacro(UseRanges, svtkTypeBool);
  svtkGetMacro(UseRanges, svtkTypeBool);
  svtkBooleanMacro(UseRanges, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * svtkCubeAxesActor2D.
   */
  virtual void SetCamera(svtkCamera*);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  enum FlyMode
  {
    SVTK_FLY_OUTER_EDGES = 0,
    SVTK_FLY_CLOSEST_TRIAD = 1,
    SVTK_FLY_NONE = 2
  };

  //@{
  /**
   * Specify a mode to control how the axes are drawn: either outer edges
   * or closest triad to the camera position, or you may also disable flying
   * of the axes.
   */
  svtkSetClampMacro(FlyMode, int, SVTK_FLY_OUTER_EDGES, SVTK_FLY_NONE);
  svtkGetMacro(FlyMode, int);
  void SetFlyModeToOuterEdges() { this->SetFlyMode(SVTK_FLY_OUTER_EDGES); }
  void SetFlyModeToClosestTriad() { this->SetFlyMode(SVTK_FLY_CLOSEST_TRIAD); }
  void SetFlyModeToNone() { this->SetFlyMode(SVTK_FLY_NONE); }
  //@}

  //@{
  /**
   * Set/Get a flag that controls whether the axes are scaled to fit in
   * the viewport. If off, the axes size remains constant (i.e., stay the
   * size of the bounding box). By default scaling is on so the axes are
   * scaled to fit inside the viewport.
   */
  svtkSetMacro(Scaling, svtkTypeBool);
  svtkGetMacro(Scaling, svtkTypeBool);
  svtkBooleanMacro(Scaling, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the number of annotation labels to show along the x, y, and
   * z axes. This values is a suggestion: the number of labels may vary
   * depending on the particulars of the data.
   */
  svtkSetClampMacro(NumberOfLabels, int, 0, 50);
  svtkGetMacro(NumberOfLabels, int);
  //@}

  //@{
  /**
   * Set/Get the labels for the x, y, and z axes. By default,
   * use "X", "Y" and "Z".
   */
  svtkSetStringMacro(XLabel);
  svtkGetStringMacro(XLabel);
  svtkSetStringMacro(YLabel);
  svtkGetStringMacro(YLabel);
  svtkSetStringMacro(ZLabel);
  svtkGetStringMacro(ZLabel);
  //@}

  /**
   * Retrieve handles to the X, Y and Z axis (so that you can set their text
   * properties for example)
   */
  svtkAxisActor2D* GetXAxisActor2D() { return this->XAxis; }
  svtkAxisActor2D* GetYAxisActor2D() { return this->YAxis; }
  svtkAxisActor2D* GetZAxisActor2D() { return this->ZAxis; }

  //@{
  /**
   * Set/Get the title text property of all axes. Note that each axis can
   * be controlled individually through the GetX/Y/ZAxisActor2D() methods.
   */
  virtual void SetAxisTitleTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(AxisTitleTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the labels text property of all axes. Note that each axis can
   * be controlled individually through the GetX/Y/ZAxisActor2D() methods.
   */
  virtual void SetAxisLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(AxisLabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on each of the
   * x-y-z axes.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set/Get the factor that controls the overall size of the fonts used
   * to label and title the axes.
   */
  svtkSetClampMacro(FontFactor, double, 0.1, 2.0);
  svtkGetMacro(FontFactor, double);
  //@}

  //@{
  /**
   * Set/Get the inertial factor that controls how often (i.e, how
   * many renders) the axes can switch position (jump from one axes
   * to another).
   */
  svtkSetClampMacro(Inertia, int, 1, SVTK_INT_MAX);
  svtkGetMacro(Inertia, int);
  //@}

  //@{
  /**
   * Set/Get the variable that controls whether the actual
   * bounds of the dataset are always shown. Setting this variable
   * to 1 means that clipping is disabled and that the actual
   * value of the bounds is displayed even with corner offsets
   * Setting this variable to 0 means these axis will clip
   * themselves and show variable bounds (legacy mode)
   */
  svtkSetClampMacro(ShowActualBounds, int, 0, 1);
  svtkGetMacro(ShowActualBounds, int);
  //@}

  //@{
  /**
   * Specify an offset value to "pull back" the axes from the corner at
   * which they are joined to avoid overlap of axes labels. The
   * "CornerOffset" is the fraction of the axis length to pull back.
   */
  svtkSetMacro(CornerOffset, double);
  svtkGetMacro(CornerOffset, double);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * Turn on and off the visibility of each axis.
   */
  svtkSetMacro(XAxisVisibility, svtkTypeBool);
  svtkGetMacro(XAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(XAxisVisibility, svtkTypeBool);
  svtkSetMacro(YAxisVisibility, svtkTypeBool);
  svtkGetMacro(YAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(YAxisVisibility, svtkTypeBool);
  svtkSetMacro(ZAxisVisibility, svtkTypeBool);
  svtkGetMacro(ZAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(ZAxisVisibility, svtkTypeBool);
  //@}

  /**
   * Shallow copy of a CubeAxesActor2D.
   */
  void ShallowCopy(svtkCubeAxesActor2D* actor);

protected:
  svtkCubeAxesActor2D();
  ~svtkCubeAxesActor2D() override;

  svtkCubeAxesActor2DConnection* ConnectionHolder;

  svtkProp* ViewProp;     // Define bounds from actor/assembly, or
  double Bounds[6];      // Define bounds explicitly
  double Ranges[6];      // Define ranges explicitly
  svtkTypeBool UseRanges; // Flag to use ranges or not

  svtkCamera* Camera;
  int FlyMode;
  svtkTypeBool Scaling;

  svtkAxisActor2D* XAxis;
  svtkAxisActor2D* YAxis;
  svtkAxisActor2D* ZAxis;

  svtkTextProperty* AxisTitleTextProperty;
  svtkTextProperty* AxisLabelTextProperty;

  svtkTimeStamp BuildTime;

  int NumberOfLabels;
  char* XLabel;
  char* YLabel;
  char* ZLabel;
  char* Labels[3];

  svtkTypeBool XAxisVisibility;
  svtkTypeBool YAxisVisibility;
  svtkTypeBool ZAxisVisibility;

  char* LabelFormat;
  double FontFactor;
  double CornerOffset;
  int Inertia;
  int RenderCount;
  int InertiaAxes[8];

  int RenderSomething;

  // Always show the actual bounds of the object
  int ShowActualBounds;

  double XOrigin;
  double YOrigin;
  double ZOrigin;

  // various helper methods
  void TransformBounds(svtkViewport* viewport, double bounds[6], double pts[8][3]);
  int ClipBounds(svtkViewport* viewport, double pts[8][3], double bounds[6]);
  double EvaluatePoint(double planes[24], double x[3]);
  double EvaluateBounds(double planes[24], double bounds[6]);
  void AdjustAxes(double pts[8][3], double bounds[6], int idx, int xIdx, int yIdx, int zIdx,
    int zIdx2, int xAxes, int yAxes, int zAxes, double xCoords[4], double yCoords[4],
    double zCoords[4], double xRange[2], double yRange[2], double zRange[2]);

private:
  // hide the superclass' ShallowCopy() from the user and the compiler.
  void ShallowCopy(svtkProp* prop) override { this->svtkProp::ShallowCopy(prop); }

private:
  svtkCubeAxesActor2D(const svtkCubeAxesActor2D&) = delete;
  void operator=(const svtkCubeAxesActor2D&) = delete;
};

#endif
