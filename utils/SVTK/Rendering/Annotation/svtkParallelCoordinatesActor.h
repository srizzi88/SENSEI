/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelCoordinatesActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParallelCoordinatesActor
 * @brief   create parallel coordinate display from input field
 *
 * svtkParallelCoordinatesActor generates a parallel coordinates plot from an
 * input field (i.e., svtkDataObject). Parallel coordinates represent
 * N-dimensional data by using a set of N parallel axes (not orthogonal like
 * the usual x-y-z Cartesian axes). Each N-dimensional point is plotted as a
 * polyline, were each of the N components of the point lie on one of the
 * N axes, and the components are connected by straight lines.
 *
 * To use this class, you must specify an input data object. You'll probably
 * also want to specify the position of the plot be setting the Position and
 * Position2 instance variables, which define a rectangle in which the plot
 * lies. Another important parameter is the IndependentVariables ivar, which
 * tells the instance how to interpret the field data (independent variables
 * as the rows or columns of the field). There are also many other instance
 * variables that control the look of the plot includes its title,
 * attributes, number of ticks on the axes, etc.
 *
 * Set the text property/attributes of the title and the labels through the
 * svtkTextProperty objects associated to this actor.
 *
 * @warning
 * Field data is not necessarily "rectangular" in shape. In these cases, some
 * of the data may not be plotted.
 *
 * @warning
 * Field data can contain non-numeric arrays (i.e. arrays not subclasses of
 * svtkDataArray). Such arrays are skipped.
 *
 * @warning
 * The early implementation lacks many features that could be added in the
 * future. This includes the ability to "brush" data (choose regions along an
 * axis and highlight any points/lines passing through the region);
 * efficiency is really bad; more control over the properties of the plot
 * (separate properties for each axes,title,etc.; and using the labels found
 * in the field to label each of the axes.
 *
 * @sa
 * svtkAxisActor3D can be used to create axes in world coordinate space.
 * svtkActor2D svtkTextMapper svtkPolyDataMapper2D svtkScalarBarActor
 * svtkCoordinate svtkTextProperty
 */

#ifndef svtkParallelCoordinatesActor_h
#define svtkParallelCoordinatesActor_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAlgorithmOutput;
class svtkAxisActor2D;
class svtkDataObject;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTextMapper;
class svtkTextProperty;
class svtkParallelCoordinatesActorConnection;

#define SVTK_IV_COLUMN 0
#define SVTK_IV_ROW 1

class SVTKRENDERINGANNOTATION_EXPORT svtkParallelCoordinatesActor : public svtkActor2D
{
public:
  svtkTypeMacro(svtkParallelCoordinatesActor, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with autorange computation;
   * the number of labels set to 5 for the x and y axes;
   * a label format of "%-#6.3g"; and x coordinates computed from point
   * ids.
   */
  static svtkParallelCoordinatesActor* New();

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
   * Set/Get the title of the parallel coordinates plot.
   */
  svtkSetStringMacro(Title);
  svtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the number of annotation labels to show along each axis.
   * This values is a suggestion: the number of labels may vary depending
   * on the particulars of the data.
   */
  svtkSetClampMacro(NumberOfLabels, int, 0, 50);
  svtkGetMacro(NumberOfLabels, int);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on the axes.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
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
   * Draw the parallel coordinates plot.
   */
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Set the input to the parallel coordinates actor. Creates
   * a pipeline connection.
   */
  virtual void SetInputConnection(svtkAlgorithmOutput*);

  /**
   * Set the input to the parallel coordinates actor. Does not
   * create a pipeline connection.
   */
  virtual void SetInputData(svtkDataObject*);

  /**
   * Remove a dataset from the list of data to append.
   */
  svtkDataObject* GetInput();

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkParallelCoordinatesActor();
  ~svtkParallelCoordinatesActor() override;

private:
  svtkParallelCoordinatesActorConnection* ConnectionHolder;

  int IndependentVariables; // Use column or row
  svtkIdType N;              // The number of independent variables
  double* Mins;             // Minimum data value along this row/column
  double* Maxs;             // Maximum data value along this row/column
  int* Xs;                  // Axes x-values (in viewport coordinates)
  int YMin;                 // Axes y-min-value (in viewport coordinates)
  int YMax;                 // Axes y-max-value (in viewport coordinates)
  int NumberOfLabels;       // Along each axis
  char* LabelFormat;
  char* Title;

  svtkAxisActor2D** Axes;
  svtkTextMapper* TitleMapper;
  svtkActor2D* TitleActor;

  svtkTextProperty* TitleTextProperty;
  svtkTextProperty* LabelTextProperty;

  svtkPolyData* PlotData; // The lines drawn within the axes
  svtkPolyDataMapper2D* PlotMapper;
  svtkActor2D* PlotActor;

  svtkTimeStamp BuildTime;

  int LastPosition[2];
  int LastPosition2[2];

  void Initialize();
  int PlaceAxes(svtkViewport* viewport, const int* size);

private:
  svtkParallelCoordinatesActor(const svtkParallelCoordinatesActor&) = delete;
  void operator=(const svtkParallelCoordinatesActor&) = delete;
};

#endif
