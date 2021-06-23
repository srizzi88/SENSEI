/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphItem
 * @brief   A 2D graphics item for rendering a graph.
 *
 *
 * This item draws a graph as a part of a svtkContextScene. This simple
 * class has minimal state and delegates the determination of visual
 * vertex and edge properties like color, size, width, etc. to
 * a set of virtual functions. To influence the rendering of the graph,
 * subclass this item and override the property functions you wish to
 * customize.
 */

#ifndef svtkGraphItem_h
#define svtkGraphItem_h

#include "svtkContextItem.h"
#include "svtkViewsInfovisModule.h" // For export macro

#include "svtkColor.h"  // For color types in API
#include "svtkNew.h"    // For svtkNew ivars
#include "svtkVector.h" // For vector types in API

class svtkGraph;
class svtkImageData;
class svtkIncrementalForceLayout;
class svtkRenderWindowInteractor;
class svtkTooltipItem;

class SVTKVIEWSINFOVIS_EXPORT svtkGraphItem : public svtkContextItem
{
public:
  static svtkGraphItem* New();
  svtkTypeMacro(svtkGraphItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The graph that this item draws.
   */
  virtual void SetGraph(svtkGraph* graph);
  svtkGetObjectMacro(Graph, svtkGraph);
  //@}

  /**
   * Exposes the incremental graph layout for updating parameters.
   */
  virtual svtkIncrementalForceLayout* GetLayout();

  //@{
  /**
   * Begins or ends the layout animation.
   */
  virtual void StartLayoutAnimation(svtkRenderWindowInteractor* interactor);
  virtual void StopLayoutAnimation();
  //@}

  /**
   * Incrementally updates the graph layout.
   */
  virtual void UpdateLayout();

protected:
  svtkGraphItem();
  ~svtkGraphItem() override;

  /**
   * Paints the graph. This method will call RebuildBuffers()
   * if the graph is dirty, then call PaintBuffers().
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Builds a cache of data from the graph by calling the virtual functions
   * such as VertexColor(), EdgeColor(), etc. This will only get called when
   * the item is dirty (i.e. IsDirty() returns true).
   */
  virtual void RebuildBuffers();

  /**
   * Efficiently draws the contents of the buffers built in RebuildBuffers.
   * This occurs once per frame.
   */
  virtual void PaintBuffers(svtkContext2D* painter);

  /**
   * Returns true if the underlying svtkGraph has been modified since the last
   * RebuildBuffers, signalling a new RebuildBuffers is needed. When the graph
   * was modified, it assumes the buffers will be rebuilt, so it updates
   * the modified time of the last build. Override this function if you have
   * a subclass that uses any information in addition to the svtkGraph to determine
   * visual properties that may be dynamic.
   */
  virtual bool IsDirty();

  /**
   * Returns the number of vertices in the graph. Generally you do not need
   * to override this method as it simply queries the underlying svtkGraph.
   */
  virtual svtkIdType NumberOfVertices();

  /**
   * Returns the number of edges in the graph. Generally you do not need
   * to override this method as it simply queries the underlying svtkGraph.
   */
  virtual svtkIdType NumberOfEdges();

  /**
   * Returns the number of edge control points for a particular edge. The
   * implementation returns GetNumberOfEdgePoints(edge) + 2 for the specified edge
   * to incorporate the source and target vertex positions as initial
   * and final edge points.
   */
  virtual svtkIdType NumberOfEdgePoints(svtkIdType edge);

  /**
   * Returns the edge width. Override in a subclass to change the edge width.
   * Note: The item currently supports one width per edge, queried on the first point.
   */
  virtual float EdgeWidth(svtkIdType edge, svtkIdType point);

  /**
   * Returns the edge color. Override in a subclass to change the edge color.
   * Each edge control point may be rendered with a separate color with interpolation
   * on line segments between points.
   */
  virtual svtkColor4ub EdgeColor(svtkIdType edge, svtkIdType point);

  /**
   * Returns the edge control point positions. You generally do not need to
   * override this method, instead change the edge control points on the
   * underlying svtkGraph with SetEdgePoint(), AddEdgePoint(), etc., then call
   * Modified() on the svtkGraph and re-render the scene.
   */
  virtual svtkVector2f EdgePosition(svtkIdType edge, svtkIdType point);

  /**
   * Returns the vertex size in pixels, which is remains the same at any zoom level.
   * Override in a subclass to change the graph vertex size.
   * Note: The item currently supports one size per graph, queried on the first vertex.
   */
  virtual float VertexSize(svtkIdType vertex);

  /**
   * Returns the color of each vertex. Override in a subclass to change the
   * graph vertex colors.
   */
  virtual svtkColor4ub VertexColor(svtkIdType vertex);

  /**
   * Returns the marker type for each vertex, as defined in svtkMarkerUtilities.
   * Override in a subclass to change the graph marker type.
   * Note: The item currently supports one marker type for all vertices,
   * queried on the first vertex.
   */
  virtual int VertexMarker(svtkIdType vertex);

  /**
   * Returns the position of each vertex. You generally do not need to override
   * this method. Instead, change the vertex positions with svtkGraph's SetPoint(),
   * then call Modified() on the graph and re-render the scene.
   */
  virtual svtkVector2f VertexPosition(svtkIdType vertex);

  /**
   * Returns the tooltip for each vertex. Override in a subclass to change the tooltip
   * text.
   */
  virtual svtkStdString VertexTooltip(svtkIdType vertex);

  /**
   * Process events and dispatch to the appropriate member functions.
   */
  static void ProcessEvents(
    svtkObject* caller, unsigned long event, void* clientData, void* callerData);

  /**
   * Return index of hit vertex, or -1 if no hit.
   */
  virtual svtkIdType HitVertex(const svtkVector2f& pos);

  //@{
  /**
   * Handle mouse events.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& event) override;
  bool MouseLeaveEvent(const svtkContextMouseEvent& event) override;
  bool MouseEnterEvent(const svtkContextMouseEvent& event) override;
  bool MouseButtonPressEvent(const svtkContextMouseEvent& event) override;
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& event) override;
  bool MouseWheelEvent(const svtkContextMouseEvent& event, int delta) override;
  //@}

  /**
   * Whether this graph item is hit.
   */
  bool Hit(const svtkContextMouseEvent& event) override;

  /**
   * Change the position of the tooltip based on the vertex hovered.
   */
  virtual void PlaceTooltip(svtkIdType v);

private:
  svtkGraphItem(const svtkGraphItem&) = delete;
  void operator=(const svtkGraphItem&) = delete;

  struct Internals;
  Internals* Internal;

  svtkGraph* Graph;
  svtkMTimeType GraphBuildTime;
  svtkNew<svtkImageData> Sprite;
  svtkNew<svtkIncrementalForceLayout> Layout;
  svtkNew<svtkTooltipItem> Tooltip;
};

#endif
