/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphLayoutView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkGraphLayoutView
 * @brief   Lays out and displays a graph
 *
 *
 * svtkGraphLayoutView performs graph layout and displays a svtkGraph.
 * You may color and label the vertices and edges using fields in the graph.
 * If coordinates are already assigned to the graph vertices in your graph,
 * set the layout strategy to PassThrough in this view. The default layout
 * is Fast2D which is fast but not that good, for better layout set the
 * layout to Simple2D or ForceDirected. There are also tree and circle
 * layout strategies. :)
 *
 * .SEE ALSO
 * svtkFast2DLayoutStrategy
 * svtkSimple2DLayoutStrategy
 * svtkForceDirectedLayoutStrategy
 *
 * @par Thanks:
 * Thanks a bunch to the holographic unfolding pattern.
 */

#ifndef svtkGraphLayoutView_h
#define svtkGraphLayoutView_h

#include "svtkRenderView.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkEdgeLayoutStrategy;
class svtkGraphLayoutStrategy;
class svtkRenderedGraphRepresentation;
class svtkViewTheme;

class SVTKVIEWSINFOVIS_EXPORT svtkGraphLayoutView : public svtkRenderView
{
public:
  static svtkGraphLayoutView* New();
  svtkTypeMacro(svtkGraphLayoutView, svtkRenderView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The array to use for vertex labeling.  Default is "VertexDegree".
   */
  void SetVertexLabelArrayName(const char* name);
  const char* GetVertexLabelArrayName();
  //@}

  //@{
  /**
   * The array to use for edge labeling.  Default is "LabelText".
   */
  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();
  //@}

  //@{
  /**
   * Whether to show vertex labels.  Default is off.
   */
  void SetVertexLabelVisibility(bool vis);
  bool GetVertexLabelVisibility();
  svtkBooleanMacro(VertexLabelVisibility, bool);
  //@}

  //@{
  /**
   * Whether to hide vertex labels during mouse interactions.  Default is off.
   */
  void SetHideVertexLabelsOnInteraction(bool vis);
  bool GetHideVertexLabelsOnInteraction();
  svtkBooleanMacro(HideVertexLabelsOnInteraction, bool);
  //@}

  //@{
  /**
   * Whether to show the edges at all. Default is on
   */
  void SetEdgeVisibility(bool vis);
  bool GetEdgeVisibility();
  svtkBooleanMacro(EdgeVisibility, bool);
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
   * Whether to hide edge labels during mouse interactions.  Default is off.
   */
  void SetHideEdgeLabelsOnInteraction(bool vis);
  bool GetHideEdgeLabelsOnInteraction();
  svtkBooleanMacro(HideEdgeLabelsOnInteraction, bool);
  //@}

  //@{
  /**
   * The array to use for coloring vertices.  The default behavior
   * is to color by vertex degree.
   */
  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();
  //@}

  //@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  svtkBooleanMacro(ColorVertices, bool);
  //@}

  //@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
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
   * Whether edges are selectable. Default is on.
   */
  void SetEdgeSelection(bool vis);
  bool GetEdgeSelection();
  svtkBooleanMacro(EdgeSelection, bool);
  //@}

  //@{
  /**
   * The array to use for coloring edges.
   */
  void SetEnabledEdgesArrayName(const char* name);
  const char* GetEnabledEdgesArrayName();
  //@}

  //@{
  /**
   * Whether to color edges.  Default is off.
   */
  void SetEnableEdgesByArray(bool vis);
  int GetEnableEdgesByArray();
  //@}

  //@{
  /**
   * The array to use for coloring vertices.
   */
  void SetEnabledVerticesArrayName(const char* name);
  const char* GetEnabledVerticesArrayName();
  //@}

  //@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetEnableVerticesByArray(bool vis);
  int GetEnableVerticesByArray();
  //@}

  //@{
  /**
   * The array used for scaling (if ScaledGlyphs is ON)
   */
  void SetScalingArrayName(const char* name);
  const char* GetScalingArrayName();
  //@}

  //@{
  /**
   * Whether to use scaled glyphs or not.  Default is off.
   */
  void SetScaledGlyphs(bool arg);
  bool GetScaledGlyphs();
  svtkBooleanMacro(ScaledGlyphs, bool);
  //@}

  //@{
  /**
   * The layout strategy to use when performing the graph layout.
   * The possible strings are:
   * - "Random"         Randomly places vertices in a box.
   * - "Force Directed" A layout in 3D or 2D simulating forces on edges.
   * - "Simple 2D"      A simple 2D force directed layout.
   * - "Clustering 2D"  A 2D force directed layout that's just like
   * simple 2D but uses some techniques to cluster better.
   * - "Community 2D"   A linear-time 2D layout that's just like
   * Fast 2D but looks for and uses a community
   * array to 'accentuate' clusters.
   * - "Fast 2D"       A linear-time 2D layout.
   * - "Pass Through"  Use locations assigned to the input.
   * - "Circular"      Places vertices uniformly on a circle.
   * - "Cone"          Cone tree layout.
   * - "Span Tree"     Span Tree Layout.
   * Default is "Simple 2D".
   */
  void SetLayoutStrategy(const char* name);
  void SetLayoutStrategyToRandom() { this->SetLayoutStrategy("Random"); }
  void SetLayoutStrategyToForceDirected() { this->SetLayoutStrategy("Force Directed"); }
  void SetLayoutStrategyToSimple2D() { this->SetLayoutStrategy("Simple 2D"); }
  void SetLayoutStrategyToClustering2D() { this->SetLayoutStrategy("Clustering 2D"); }
  void SetLayoutStrategyToCommunity2D() { this->SetLayoutStrategy("Community 2D"); }
  void SetLayoutStrategyToFast2D() { this->SetLayoutStrategy("Fast 2D"); }
  void SetLayoutStrategyToPassThrough() { this->SetLayoutStrategy("Pass Through"); }
  void SetLayoutStrategyToCircular() { this->SetLayoutStrategy("Circular"); }
  void SetLayoutStrategyToTree() { this->SetLayoutStrategy("Tree"); }
  void SetLayoutStrategyToCosmicTree() { this->SetLayoutStrategy("Cosmic Tree"); }
  void SetLayoutStrategyToCone() { this->SetLayoutStrategy("Cone"); }
  void SetLayoutStrategyToSpanTree() { this->SetLayoutStrategy("Span Tree"); }
  const char* GetLayoutStrategyName();
  //@}

  //@{
  /**
   * The layout strategy to use when performing the graph layout.
   * This signature allows an application to create a layout
   * object directly and simply set the pointer through this method.
   */
  svtkGraphLayoutStrategy* GetLayoutStrategy();
  void SetLayoutStrategy(svtkGraphLayoutStrategy* s);
  //@}

  //@{
  /**
   * The layout strategy to use when performing the edge layout.
   * The possible strings are:
   * "Arc Parallel"   - Arc parallel edges and self loops.
   * "Pass Through"   - Use edge routes assigned to the input.
   * Default is "Arc Parallel".
   */
  void SetEdgeLayoutStrategy(const char* name);
  void SetEdgeLayoutStrategyToArcParallel() { this->SetEdgeLayoutStrategy("Arc Parallel"); }
  void SetEdgeLayoutStrategyToPassThrough() { this->SetEdgeLayoutStrategy("Pass Through"); }
  const char* GetEdgeLayoutStrategyName();
  //@}

  //@{
  /**
   * The layout strategy to use when performing the edge layout.
   * This signature allows an application to create a layout
   * object directly and simply set the pointer through this method.
   */
  svtkEdgeLayoutStrategy* GetEdgeLayoutStrategy();
  void SetEdgeLayoutStrategy(svtkEdgeLayoutStrategy* s);
  //@}

  /**
   * Associate the icon at index "index" in the svtkTexture to all vertices
   * containing "type" as a value in the vertex attribute array specified by
   * IconArrayName.
   */
  void AddIconType(const char* type, int index);

  /**
   * Clear all icon mappings.
   */
  void ClearIconTypes();

  /**
   * Specify where the icons should be placed in relation to the vertex.
   * See svtkIconGlyphFilter.h for possible values.
   */
  void SetIconAlignment(int alignment);

  //@{
  /**
   * Whether icons are visible (default off).
   */
  void SetIconVisibility(bool b);
  bool GetIconVisibility();
  svtkBooleanMacro(IconVisibility, bool);
  //@}

  //@{
  /**
   * The array used for assigning icons
   */
  void SetIconArrayName(const char* name);
  const char* GetIconArrayName();
  //@}

  //@{
  /**
   * The type of glyph to use for the vertices
   */
  void SetGlyphType(int type);
  int GetGlyphType();
  //@}

  //@{
  /**
   * The size of the font used for vertex labeling
   */
  virtual void SetVertexLabelFontSize(const int size);
  virtual int GetVertexLabelFontSize();
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
   * Whether the scalar bar for edges is visible.  Default is off.
   */
  void SetEdgeScalarBarVisibility(bool vis);
  bool GetEdgeScalarBarVisibility();
  //@}

  //@{
  /**
   * Whether the scalar bar for vertices is visible.  Default is off.
   */
  void SetVertexScalarBarVisibility(bool vis);
  bool GetVertexScalarBarVisibility();
  //@}

  /**
   * Reset the camera based on the bounds of the selected region.
   */
  void ZoomToSelection();

  /**
   * Is the graph layout complete? This method is useful
   * for when the strategy is iterative and the application
   * wants to show the iterative progress of the graph layout
   * See Also: UpdateLayout();
   */
  virtual int IsLayoutComplete();

  /**
   * This method is useful for when the strategy is iterative
   * and the application wants to show the iterative progress
   * of the graph layout. The application would have something like
   * while(!IsLayoutComplete())
   * {
   * UpdateLayout();
   * }
   * See Also: IsLayoutComplete();
   */
  virtual void UpdateLayout();

protected:
  svtkGraphLayoutView();
  ~svtkGraphLayoutView() override;

  //@{
  /**
   * Overrides behavior in svtkView to create a svtkRenderedGraphRepresentation
   * by default.
   */
  svtkDataRepresentation* CreateDefaultRepresentation(svtkAlgorithmOutput* conn) override;
  virtual svtkRenderedGraphRepresentation* GetGraphRepresentation();
  // Called to process events.  Overrides behavior in svtkRenderView.
  void ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData) override;
  //@}

private:
  svtkGraphLayoutView(const svtkGraphLayoutView&) = delete;
  void operator=(const svtkGraphLayoutView&) = delete;
  bool VertexLabelsRequested;
  bool EdgeLabelsRequested;
};

#endif
