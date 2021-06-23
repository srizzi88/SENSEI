/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSpiderPlotActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSpiderPlotActor
 * @brief   create a spider plot from input field
 *
 * svtkSpiderPlotActor generates a spider plot from an input field (i.e.,
 * svtkDataObject). A spider plot represents N-dimensional data by using a set
 * of N axes that originate from the center of a circle, and form the spokes
 * of a wheel (like a spider web).  Each N-dimensional point is plotted as a
 * polyline that forms a closed polygon; the vertices of the polygon
 * are plotted against the radial axes.
 *
 * To use this class, you must specify an input data object. You'll probably
 * also want to specify the position of the plot be setting the Position and
 * Position2 instance variables, which define a rectangle in which the plot
 * lies. Another important parameter is the IndependentVariables ivar, which
 * tells the instance how to interpret the field data (independent variables
 * as the rows or columns of the field). There are also many other instance
 * variables that control the look of the plot includes its title and legend.
 *
 * Set the text property/attributes of the title and the labels through the
 * svtkTextProperty objects associated with these components.
 *
 * @warning
 * Field data is not necessarily "rectangular" in shape. In these cases, some
 * of the data may not be plotted.
 *
 * @warning
 * Field data can contain non-numeric arrays (i.e. arrays not subclasses of
 * svtkDataArray). Such arrays are skipped.
 *
 * @sa
 * svtkParallelCoordinatesActor svtkXYPlotActor2D
 */

#ifndef svtkSpiderPlotActor_h
#define svtkSpiderPlotActor_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAlgorithmOutput;
class svtkAxisActor2D;
class svtkDataObject;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTextMapper;
class svtkTextProperty;
class svtkLegendBoxActor;
class svtkGlyphSource2D;
class svtkAxisLabelArray;
class svtkAxisRanges;
class svtkSpiderPlotActorConnection;

#define SVTK_IV_COLUMN 0
#define SVTK_IV_ROW 1

class SVTKRENDERINGANNOTATION_EXPORT svtkSpiderPlotActor : public svtkActor2D
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkSpiderPlotActor, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Instantiate this class.
   */
  static svtkSpiderPlotActor* New();

  //@{
  /**
   * Set the input to the pie chart actor. SetInputData()
   * does not connect the pipeline whereas SetInputConnection()
   * does.
   */
  virtual void SetInputData(svtkDataObject*);
  virtual void SetInputConnection(svtkAlgorithmOutput*);
  //@}

  /**
   * Get the input data object to this actor.
   */
  virtual svtkDataObject* GetInput();

  //@{
  /**
   * Specify whether to use the rows or columns as independent variables.
   * If columns, then each row represents a separate point. If rows, then
   * each column represents a separate point.
   */
  svtkSetClampMacro(IndependentVariables, int, SVTK_IV_COLUMN, SVTK_IV_ROW);
  svtkGetMacro(IndependentVariables, int);
  void SetIndependentVariablesToColumns() { this->SetIndependentVariables(SVTK_IV_COLUMN); }
  void SetIndependentVariablesToRows() { this->SetIndependentVariables(SVTK_IV_ROW); }
  //@}

  //@{
  /**
   * Enable/Disable the display of a plot title.
   */
  svtkSetMacro(TitleVisibility, svtkTypeBool);
  svtkGetMacro(TitleVisibility, svtkTypeBool);
  svtkBooleanMacro(TitleVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the title of the spider plot.
   */
  svtkSetStringMacro(Title);
  svtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the title text property.
   */
  virtual void SetTitleTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TitleTextProperty, svtkTextProperty);
  //@}

  // Enable/Disable the display axes titles. These are arranged on the end
  // of each radial axis on the circumference of the spider plot. The label
  // text strings are derived from the names of the data object arrays
  // associated with the input.
  svtkSetMacro(LabelVisibility, svtkTypeBool);
  svtkGetMacro(LabelVisibility, svtkTypeBool);
  svtkBooleanMacro(LabelVisibility, svtkTypeBool);

  //@{
  /**
   * Enable/Disable the creation of a legend. If on, the legend labels will
   * be created automatically unless the per plot legend symbol has been
   * set.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Specify the number of circumferential rings. If set to zero, then
   * none will be shown; otherwise the specified number will be shown.
   */
  svtkSetClampMacro(NumberOfRings, int, 0, SVTK_INT_MAX);
  svtkGetMacro(NumberOfRings, int);
  //@}

  //@{
  /**
   * Specify the names of the radial spokes (i.e., the radial axes). If
   * not specified, then an integer number is automatically generated.
   */
  void SetAxisLabel(const int i, const char*);
  const char* GetAxisLabel(int i);
  //@}

  //@{
  /**
   * Specify the range of data on each radial axis. If not specified,
   * then the range is computed automatically.
   */
  void SetAxisRange(int i, double min, double max);
  void SetAxisRange(int i, double range[2]);
  void GetAxisRange(int i, double range[2]);
  //@}

  //@{
  /**
   * Specify colors for each plot. If not specified, they are automatically generated.
   */
  void SetPlotColor(int i, double r, double g, double b);
  void SetPlotColor(int i, const double color[3])
  {
    this->SetPlotColor(i, color[0], color[1], color[2]);
  }
  double* GetPlotColor(int i);
  //@}

  //@{
  /**
   * Enable/Disable the creation of a legend. If on, the legend labels will
   * be created automatically unless the per plot legend symbol has been
   * set.
   */
  svtkSetMacro(LegendVisibility, svtkTypeBool);
  svtkGetMacro(LegendVisibility, svtkTypeBool);
  svtkBooleanMacro(LegendVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Retrieve handles to the legend box. This is useful if you would like
   * to manually control the legend appearance.
   */
  svtkGetObjectMacro(LegendActor, svtkLegendBoxActor);
  //@}

  //@{
  /**
   * Draw the spider plot.
   */
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
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

protected:
  svtkSpiderPlotActor();
  ~svtkSpiderPlotActor() override;

private:
  svtkSpiderPlotActorConnection* ConnectionHolder;

  int IndependentVariables;    // Use column or row
  svtkTypeBool TitleVisibility; // Should I see the title?
  char* Title;                 // The title string
  svtkTextProperty* TitleTextProperty;
  svtkTypeBool LabelVisibility;
  svtkTextProperty* LabelTextProperty;
  svtkAxisLabelArray* Labels;
  svtkTypeBool LegendVisibility;
  svtkLegendBoxActor* LegendActor;
  svtkGlyphSource2D* GlyphSource;
  int NumberOfRings;

  // Local variables needed to plot
  svtkIdType N;  // The number of independent variables
  double* Mins; // Minimum data value along this row/column
  double* Maxs; // Maximum data value along this row/column
  svtkAxisRanges* Ranges;

  svtkTextMapper** LabelMappers; // a label for each radial spoke
  svtkActor2D** LabelActors;

  svtkTextMapper* TitleMapper;
  svtkActor2D* TitleActor;

  svtkPolyData* WebData; // The web of the spider plot
  svtkPolyDataMapper2D* WebMapper;
  svtkActor2D* WebActor;

  svtkPolyData* PlotData; // The lines drawn within the axes
  svtkPolyDataMapper2D* PlotMapper;
  svtkActor2D* PlotActor;

  svtkTimeStamp BuildTime;

  double Center[3];
  double Radius;
  double Theta;

  int LastPosition[2];
  int LastPosition2[2];
  double P1[3];
  double P2[3];

  void Initialize();
  int PlaceAxes(svtkViewport* viewport, const int* size);
  int BuildPlot(svtkViewport*);

private:
  svtkSpiderPlotActor(const svtkSpiderPlotActor&) = delete;
  void operator=(const svtkSpiderPlotActor&) = delete;
};

#endif
