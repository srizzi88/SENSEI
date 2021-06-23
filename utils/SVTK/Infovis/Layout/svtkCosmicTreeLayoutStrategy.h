/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCosmicTreeLayoutStrategy.h

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkCosmicTreeLayoutStrategy
 * @brief   tree layout strategy reminiscent of astronomical systems
 *
 *
 * This layout strategy takes an input tree and places all the children of a
 * node into a containing circle. The placement is such that each child
 * placed can be represented with a circle tangent to the containing circle
 * and (usually) 2 other children. The interior of the circle is left empty
 * so that graph edges drawn on top of the tree will not obfuscate the tree.
 * However, when one child is much larger than all the others, it may
 * encroach on the center of the containing circle; that's OK, because it's
 * large enough not to be obscured by edges drawn atop it.
 *
 * @par Thanks:
 * Thanks to the galaxy and David Thompson hierarchically nested inside it
 * for inspiring this layout strategy.
 */

#ifndef svtkCosmicTreeLayoutStrategy_h
#define svtkCosmicTreeLayoutStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkDoubleArray;
class svtkDataArray;
class svtkPoints;
class svtkTree;

class SVTKINFOVISLAYOUT_EXPORT svtkCosmicTreeLayoutStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkCosmicTreeLayoutStrategy* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkCosmicTreeLayoutStrategy, svtkGraphLayoutStrategy);

  /**
   * Perform the layout.
   */
  void Layout() override;

  //@{
  /**
   * Should node size specifications be obeyed at leaf nodes only or
   * (with scaling as required to meet constraints) at every node in
   * the tree?
   * This defaults to true, so that leaf nodes are scaled according to
   * the size specification provided, and the parent node sizes are
   * calculated by the algorithm.
   */
  svtkSetMacro(SizeLeafNodesOnly, svtkTypeBool);
  svtkGetMacro(SizeLeafNodesOnly, svtkTypeBool);
  svtkBooleanMacro(SizeLeafNodesOnly, svtkTypeBool);
  //@}

  //@{
  /**
   * How many levels of the tree should be laid out?
   * For large trees, you may wish to set the root and maximum depth
   * in order to retrieve the layout for the visible portion of the tree.
   * When this value is zero or negative, all nodes below and including
   * the LayoutRoot will be presented.
   * This defaults to 0.
   */
  svtkSetMacro(LayoutDepth, int);
  svtkGetMacro(LayoutDepth, int);
  //@}

  //@{
  /**
   * What is the top-most tree node to lay out?
   * This node will become the largest containing circle in the layout.
   * Use this in combination with SetLayoutDepth to retrieve the
   * layout of a subtree of interest for rendering.
   * Setting LayoutRoot to a negative number signals that the root node
   * of the tree should be used as the root node of the layout.
   * This defaults to -1.
   */
  svtkSetMacro(LayoutRoot, svtkIdType);
  svtkGetMacro(LayoutRoot, svtkIdType);
  //@}

  //@{
  /**
   * Set the array to be used for sizing nodes.
   * If this is set to an empty string or nullptr (the default),
   * then all leaf nodes (or all nodes, when SizeLeafNodesOnly is false)
   * will be assigned a unit size.
   */
  svtkSetStringMacro(NodeSizeArrayName);
  svtkGetStringMacro(NodeSizeArrayName);
  //@}

protected:
  /// How are node sizes specified?
  enum RadiusMode
  {
    NONE,   //!< No node sizes specified... unit radius is assumed.
    LEAVES, //!< Only leaf node sizes specified... parents are calculated during layout.
    ALL     //!< All node sizes specified (overconstrained, so a scale factor for each parent is
            //!< calculated during layout).
  };

  svtkCosmicTreeLayoutStrategy();
  ~svtkCosmicTreeLayoutStrategy() override;

  /**
   * Recursive routine used to lay out tree nodes. Called from Layout().
   */
  void LayoutChildren(svtkTree* tree, svtkPoints* newPoints, svtkDoubleArray* radii,
    svtkDoubleArray* scale, svtkIdType root, int depth, RadiusMode mode);

  /**
   * Recursive routine that adds each parent node's (x,y) position to its children.
   * This must be done only after all the children have been laid out at the origin
   * since we will not know the parent's position until after the child radii have
   * been determined.
   */
  void OffsetChildren(svtkTree* tree, svtkPoints* pts, svtkDoubleArray* radii, svtkDoubleArray* scale,
    double parent[4], svtkIdType root, int depth, RadiusMode mode);

  /**
   * Create an array to hold radii, named appropriately (depends on \a NodeSizeArrayName)
   * and initialized to either (a) -1.0 for each node or (b) a deep copy of an existing array.
   * @param numVertices  The number of vertices on the tree.
   * @param initialValue The starting value of each node's radius. Only used when \a inputRadii is
   * nullptr.
   * @param inputRadii   Either nullptr or the address of another array to be copied into the output
   * array
   * @retval             The array of node radii to be set on the output
   */
  svtkDoubleArray* CreateRadii(svtkIdType numVertices, double initialValue, svtkDataArray* inputRadii);

  /**
   * Create an array to hold scale factors, named appropriately (depends on \a NodeSizeArrayName)
   * and initialized to -1.0.
   * @param numVertices The number of vertices on the tree.
   * @retval            The array of node scale factors to be set on the output
   */
  svtkDoubleArray* CreateScaleFactors(svtkIdType numVertices);

  svtkTypeBool SizeLeafNodesOnly;
  int LayoutDepth;
  svtkIdType LayoutRoot;
  char* NodeSizeArrayName;

private:
  svtkCosmicTreeLayoutStrategy(const svtkCosmicTreeLayoutStrategy&) = delete;
  void operator=(const svtkCosmicTreeLayoutStrategy&) = delete;
};

#endif // svtkCosmicTreeLayoutStrategy_h
