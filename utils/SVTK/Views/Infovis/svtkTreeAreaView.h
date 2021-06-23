/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeAreaView.h

  -------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTreeAreaView
 * @brief   Accepts a graph and a hierarchy - currently
 * a tree - and provides a hierarchy-aware display.  Currently, this means
 * displaying the hierarchy using a tree ring layout, then rendering the graph
 * vertices as leaves of the tree with curved graph edges between leaves.
 *
 *
 * Takes a graph and a hierarchy (currently a tree) and lays out the graph
 * vertices based on their categorization within the hierarchy.
 *
 * .SEE ALSO
 * svtkGraphLayoutView
 *
 * @par Thanks:
 * Thanks to Jason Shepherd for implementing this class
 */

#ifndef svtkTreeAreaView_h
#define svtkTreeAreaView_h

#include "svtkRenderView.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkAreaLayoutStrategy;
class svtkGraph;
class svtkLabeledDataMapper;
class svtkPolyDataAlgorithm;
class svtkRenderedTreeAreaRepresentation;
class svtkTree;

class SVTKVIEWSINFOVIS_EXPORT svtkTreeAreaView : public svtkRenderView
{
public:
  static svtkTreeAreaView* New();
  svtkTypeMacro(svtkTreeAreaView, svtkRenderView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the tree and graph representations to the appropriate input ports.
   */
  svtkDataRepresentation* SetTreeFromInputConnection(svtkAlgorithmOutput* conn);
  svtkDataRepresentation* SetTreeFromInput(svtkTree* input);
  svtkDataRepresentation* SetGraphFromInputConnection(svtkAlgorithmOutput* conn);
  svtkDataRepresentation* SetGraphFromInput(svtkGraph* input);
  //@}

  //@{
  /**
   * The array to use for area labeling.  Default is "label".
   */
  void SetAreaLabelArrayName(const char* name);
  const char* GetAreaLabelArrayName();
  //@}

  //@{
  /**
   * The array to use for area sizes. Default is "size".
   */
  void SetAreaSizeArrayName(const char* name);
  const char* GetAreaSizeArrayName();
  //@}

  //@{
  /**
   * The array to use for area labeling priority.
   * Default is "GraphVertexDegree".
   */
  void SetLabelPriorityArrayName(const char* name);
  const char* GetLabelPriorityArrayName();
  //@}

  //@{
  /**
   * The array to use for edge labeling.  Default is "label".
   */
  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();
  //@}

  //@{
  /**
   * The name of the array whose value appears when the mouse hovers
   * over a rectangle in the treemap.
   * This must be a string array.
   */
  void SetAreaHoverArrayName(const char* name);
  const char* GetAreaHoverArrayName();
  //@}

  //@{
  /**
   * Whether to show area labels.  Default is off.
   */
  void SetAreaLabelVisibility(bool vis);
  bool GetAreaLabelVisibility();
  svtkBooleanMacro(AreaLabelVisibility, bool);
  //@}

  //@{
  /**
   * Whether to show edge labels.  Default is off.
   */
  void SetEdgeLabelVisibility(bool vis);
  bool GetEdgeLabelVisibility();
  svtkBooleanMacro(EdgeLabelVisibility, bool);
  //@}

  //@{
  /**
   * The array to use for coloring vertices.  Default is "color".
   */
  void SetAreaColorArrayName(const char* name);
  const char* GetAreaColorArrayName();
  //@}

  //@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetColorAreas(bool vis);
  bool GetColorAreas();
  svtkBooleanMacro(ColorAreas, bool);
  //@}

  //@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
  //@}

  /**
   * Set the color to be the spline fraction
   */
  void SetEdgeColorToSplineFraction();

  //@{
  /**
   * Set the region shrink percentage between 0.0 and 1.0.
   */
  void SetShrinkPercentage(double value);
  double GetShrinkPercentage();
  //@}

  //@{
  /**
   * Whether to color edges.  Default is off.
   */
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  svtkBooleanMacro(ColorEdges, bool);
  //@}

  //@{
  /**
   * Set the bundling strength.
   */
  void SetBundlingStrength(double strength);
  double GetBundlingStrength();
  //@}

  //@{
  /**
   * The size of the font used for area labeling
   */
  virtual void SetAreaLabelFontSize(const int size);
  virtual int GetAreaLabelFontSize();
  //@}

  //@{
  /**
   * The size of the font used for edge labeling
   */
  virtual void SetEdgeLabelFontSize(const int size);
  virtual int GetEdgeLabelFontSize();
  //@}

  //@{
  /**
   * The layout strategy for producing spatial regions for the tree.
   */
  virtual void SetLayoutStrategy(svtkAreaLayoutStrategy* strategy);
  virtual svtkAreaLayoutStrategy* GetLayoutStrategy();
  //@}

  //@{
  /**
   * Whether the area represents radial or rectangular coordinates.
   */
  virtual void SetUseRectangularCoordinates(bool rect);
  virtual bool GetUseRectangularCoordinates();
  svtkBooleanMacro(UseRectangularCoordinates, bool);
  //@}

  //@{
  /**
   * Visibility of scalar bar actor for edges.
   */
  virtual void SetEdgeScalarBarVisibility(bool b);
  virtual bool GetEdgeScalarBarVisibility();
  //@}

protected:
  svtkTreeAreaView();
  ~svtkTreeAreaView() override;

  //@{
  /**
   * The filter for converting areas to polydata. This may e.g. be
   * svtkTreeMapToPolyData or svtkTreeRingToPolyData.
   * The filter must take a svtkTree as input and produce svtkPolyData.
   */
  virtual void SetAreaToPolyData(svtkPolyDataAlgorithm* areaToPoly);
  virtual svtkPolyDataAlgorithm* GetAreaToPolyData();
  //@}

  //@{
  /**
   * The mapper for rendering labels on areas. This may e.g. be
   * svtkDynamic2DLabelMapper or svtkTreeMapLabelMapper.
   */
  virtual void SetAreaLabelMapper(svtkLabeledDataMapper* mapper);
  virtual svtkLabeledDataMapper* GetAreaLabelMapper();
  //@}

  //@{
  /**
   * Overrides behavior in svtkView to create a svtkRenderedGraphRepresentation
   * by default.
   */
  svtkDataRepresentation* CreateDefaultRepresentation(svtkAlgorithmOutput* conn) override;
  virtual svtkRenderedTreeAreaRepresentation* GetTreeAreaRepresentation();
  //@}

private:
  svtkTreeAreaView(const svtkTreeAreaView&) = delete;
  void operator=(const svtkTreeAreaView&) = delete;
};

#endif
