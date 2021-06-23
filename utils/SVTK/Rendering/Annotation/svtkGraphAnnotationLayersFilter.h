/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkGraphAnnotationLayersFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/**
 * @class   svtkGraphAnnotationLayersFilter
 * @brief   Produce filled convex hulls around
 * subsets of vertices in a svtkGraph.
 *
 *
 * Produces a svtkPolyData comprised of filled polygons of the convex hull
 * of a cluster. Alternatively, you may choose to output bounding rectangles.
 * Clusters with fewer than three vertices are artificially expanded to
 * ensure visibility (see svtkConvexHull2D).
 *
 * The first input is a svtkGraph with points, possibly set by
 * passing the graph through svtkGraphLayout (z-values are ignored). The second
 * input is a svtkAnnotationsLayer containing svtkSelectionNodeS of vertex
 * ids (the 'clusters' output of svtkTulipReader for example).
 *
 * Setting OutlineOn() additionally produces outlines of the clusters on
 * output port 1.
 *
 * Three arrays are added to the cells of the output: "Hull id"; "Hull name";
 * and "Hull color".
 *
 * Note: This filter operates in the x,y-plane and as such works best with an
 * interactor style that does not allow camera rotation, such as
 * svtkInteractorStyleRubberBand2D.
 *
 * @sa
 * svtkContext2D
 *
 * @par Thanks:
 * Thanks to Colin Myers, University of Leeds for providing this implementation.
 */

#ifndef svtkGraphAnnotationLayersFilter_h
#define svtkGraphAnnotationLayersFilter_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingAnnotationModule.h" // For export macro
#include "svtkSmartPointer.h"              // needed for ivars

class svtkAppendPolyData;
class svtkConvexHull2D;
class svtkRenderer;

class SVTKRENDERINGANNOTATION_EXPORT svtkGraphAnnotationLayersFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkGraphAnnotationLayersFilter* New();
  svtkTypeMacro(svtkGraphAnnotationLayersFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Produce outlines of the hulls on output port 1.
   */
  void OutlineOn();
  void OutlineOff();
  void SetOutline(bool b);
  //@}

  /**
   * Scale each hull by the amount specified. Defaults to 1.0.
   */
  void SetScaleFactor(double scale);

  /**
   * Set the shape of the hulls to bounding rectangle.
   */
  void SetHullShapeToBoundingRectangle();

  /**
   * Set the shape of the hulls to convex hull. Default.
   */
  void SetHullShapeToConvexHull();

  /**
   * Set the minimum x,y-dimensions of each hull in world coordinates. Defaults
   * to 1.0. Set to 0.0 to disable.
   */
  void SetMinHullSizeInWorld(double size);

  /**
   * Set the minimum x,y-dimensions of each hull in pixels. You must also set a
   * svtkRenderer. Defaults to 1. Set to 0 to disable.
   */
  void SetMinHullSizeInDisplay(int size);

  /**
   * Renderer needed for MinHullSizeInDisplay calculation. Not reference counted.
   */
  void SetRenderer(svtkRenderer* renderer);

  /**
   * The modified time of this filter.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkGraphAnnotationLayersFilter();
  ~svtkGraphAnnotationLayersFilter() override;

  /**
   * This is called by the superclass. This is the method you should override.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the input to svtkGraph and svtkAnnotationLayers.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkGraphAnnotationLayersFilter(const svtkGraphAnnotationLayersFilter&) = delete;
  void operator=(const svtkGraphAnnotationLayersFilter&) = delete;

  svtkSmartPointer<svtkAppendPolyData> HullAppend;
  svtkSmartPointer<svtkAppendPolyData> OutlineAppend;
  svtkSmartPointer<svtkConvexHull2D> ConvexHullFilter;
};

#endif // svtkGraphAnnotationLayersFilter_h
