/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeOrbitLayoutStrategy.h

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkTreeOrbitLayoutStrategy
 * @brief   hierarchical orbital layout
 *
 *
 * Assigns points to the nodes of a tree to an orbital layout. Each parent
 * is orbited by its children, recursively.
 *
 * @par Thanks:
 * Thanks to the galaxy for inspiring this layout strategy.
 */

#ifndef svtkTreeOrbitLayoutStrategy_h
#define svtkTreeOrbitLayoutStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkPoints;
class svtkTree;

class SVTKINFOVISLAYOUT_EXPORT svtkTreeOrbitLayoutStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkTreeOrbitLayoutStrategy* New();

  svtkTypeMacro(svtkTreeOrbitLayoutStrategy, svtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the orbital layout.
   */
  void Layout() override;

  //@{
  /**
   * The spacing of orbital levels. Levels near zero give more space
   * to levels near the root, while levels near one (the default)
   * create evenly-spaced levels. Levels above one give more space
   * to levels near the leaves.
   */
  svtkSetMacro(LogSpacingValue, double);
  svtkGetMacro(LogSpacingValue, double);
  //@}

  //@{
  /**
   * The spacing of leaves.  Levels near one evenly space leaves
   * with no gaps between subtrees.  Levels near zero creates
   * large gaps between subtrees.
   */
  svtkSetClampMacro(LeafSpacing, double, 0.0, 1.0);
  svtkGetMacro(LeafSpacing, double);
  //@}

  //@{
  /**
   * This is a magic number right now. Controls the radius
   * of the child layout, all of this should be fixed at
   * some point with a more logical layout. Defaults to .5 :)
   */
  svtkSetMacro(ChildRadiusFactor, double);
  svtkGetMacro(ChildRadiusFactor, double);
  //@}

protected:
  svtkTreeOrbitLayoutStrategy();
  ~svtkTreeOrbitLayoutStrategy() override;

  void OrbitChildren(svtkTree* t, svtkPoints* p, svtkIdType parent, double radius);

  double LogSpacingValue;
  double LeafSpacing;
  double ChildRadiusFactor;

private:
  svtkTreeOrbitLayoutStrategy(const svtkTreeOrbitLayoutStrategy&) = delete;
  void operator=(const svtkTreeOrbitLayoutStrategy&) = delete;
};

#endif
