/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArcPlotter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkArcPlotter
 * @brief   plot data along an arbitrary polyline
 *
 * svtkArcPlotter performs plotting of attribute data along polylines defined
 * with an input svtkPolyData data object. Any type of attribute data can be
 * plotted including scalars, vectors, tensors, normals, texture coordinates,
 * and field data. Either one or multiple data components can be plotted.
 *
 * To use this class you must specify an input data set that contains one or
 * more polylines, and some attribute data including which component of the
 * attribute data. (By default, this class processes the first component of
 * scalar data.) You will also need to set an offset radius (the distance
 * of the polyline to the median line of the plot), a width for the plot
 * (the distance that the minimum and maximum plot values are mapped into),
 * an possibly an offset (used to offset attribute data with multiple
 * components).
 *
 * Normally the filter automatically computes normals for generating the
 * offset arc plot. However, you can specify a default normal and use that
 * instead.
 *
 * @sa
 * svtkXYPlotActor
 */

#ifndef svtkArcPlotter_h
#define svtkArcPlotter_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

#define SVTK_PLOT_SCALARS 1
#define SVTK_PLOT_VECTORS 2
#define SVTK_PLOT_NORMALS 3
#define SVTK_PLOT_TCOORDS 4
#define SVTK_PLOT_TENSORS 5
#define SVTK_PLOT_FIELD_DATA 6

class svtkCamera;
class svtkDataArray;
class svtkPointData;
class svtkPoints;

class SVTKRENDERINGANNOTATION_EXPORT svtkArcPlotter : public svtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate with no default camera and plot mode set to
   * SVTK_SCALARS.
   */
  static svtkArcPlotter* New();

  svtkTypeMacro(svtkArcPlotter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify a camera used to orient the plot along the arc. If no camera
   * is specified, then the orientation of the plot is arbitrary.
   */
  virtual void SetCamera(svtkCamera*);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  //@{
  /**
   * Specify which data to plot: scalars, vectors, normals, texture coords,
   * tensors, or field data. If the data has more than one component, use
   * the method SetPlotComponent to control which component to plot.
   */
  svtkSetMacro(PlotMode, int);
  svtkGetMacro(PlotMode, int);
  void SetPlotModeToPlotScalars() { this->SetPlotMode(SVTK_PLOT_SCALARS); }
  void SetPlotModeToPlotVectors() { this->SetPlotMode(SVTK_PLOT_VECTORS); }
  void SetPlotModeToPlotNormals() { this->SetPlotMode(SVTK_PLOT_NORMALS); }
  void SetPlotModeToPlotTCoords() { this->SetPlotMode(SVTK_PLOT_TCOORDS); }
  void SetPlotModeToPlotTensors() { this->SetPlotMode(SVTK_PLOT_TENSORS); }
  void SetPlotModeToPlotFieldData() { this->SetPlotMode(SVTK_PLOT_FIELD_DATA); }
  //@}

  //@{
  /**
   * Set/Get the component number to plot if the data has more than one
   * component. If the value of the plot component is == (-1), then all
   * the components will be plotted.
   */
  svtkSetMacro(PlotComponent, int);
  svtkGetMacro(PlotComponent, int);
  //@}

  //@{
  /**
   * Set the radius of the "median" value of the first plotted component.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set the height of the plot. (The radius combined with the height
   * define the location of the plot relative to the generating polyline.)
   */
  svtkSetClampMacro(Height, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Height, double);
  //@}

  //@{
  /**
   * Specify an offset that translates each subsequent plot (if there is
   * more than one component plotted) from the defining arc (i.e., polyline).
   */
  svtkSetClampMacro(Offset, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Offset, double);
  //@}

  //@{
  /**
   * Set a boolean to control whether to use default normals.
   * By default, normals are automatically computed from the generating
   * polyline and camera.
   */
  svtkSetMacro(UseDefaultNormal, svtkTypeBool);
  svtkGetMacro(UseDefaultNormal, svtkTypeBool);
  svtkBooleanMacro(UseDefaultNormal, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the default normal to use if you do not wish automatic normal
   * calculation. The arc plot will be generated using this normal.
   */
  svtkSetVector3Macro(DefaultNormal, float);
  svtkGetVectorMacro(DefaultNormal, float, 3);
  //@}

  //@{
  /**
   * Set/Get the field data array to plot. This instance variable is
   * only applicable if field data is plotted.
   */
  svtkSetClampMacro(FieldDataArray, int, 0, SVTK_INT_MAX);
  svtkGetMacro(FieldDataArray, int);
  //@}

  /**
   * New GetMTime because of camera dependency.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkArcPlotter();
  ~svtkArcPlotter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  svtkIdType OffsetPoint(svtkIdType ptId, svtkPoints* inPts, double n[3], svtkPoints* newPts,
    double offset, double* range, double val);
  int ProcessComponents(svtkIdType numPts, svtkPointData* pd);

  svtkCamera* Camera;
  int PlotMode;
  int PlotComponent;
  double Radius;
  double Height;
  double Offset;
  float DefaultNormal[3];
  svtkTypeBool UseDefaultNormal;
  int FieldDataArray;

private:
  svtkDataArray* Data;
  double* DataRange;
  double* Tuple;
  int NumberOfComponents;
  int ActiveComponent;
  int StartComp;
  int EndComp;

private:
  svtkArcPlotter(const svtkArcPlotter&) = delete;
  void operator=(const svtkArcPlotter&) = delete;
};

#endif
