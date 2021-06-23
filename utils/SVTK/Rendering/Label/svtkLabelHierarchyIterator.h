/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabelHierarchyIterator.h

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
 * @class   svtkLabelHierarchyIterator
 * @brief   iterator over svtkLabelHierarchy
 *
 *
 * Abstract superclass for iterators over svtkLabelHierarchy.
 */

#ifndef svtkLabelHierarchyIterator_h
#define svtkLabelHierarchyIterator_h

#include "svtkObject.h"
#include "svtkRenderingLabelModule.h" // For export macro
#include "svtkStdString.h"            // for std string
#include "svtkUnicodeString.h"        // for unicode string

class svtkIdTypeArray;
class svtkLabelHierarchy;
class svtkPolyData;

class SVTKRENDERINGLABEL_EXPORT svtkLabelHierarchyIterator : public svtkObject
{
public:
  svtkTypeMacro(svtkLabelHierarchyIterator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Initializes the iterator. lastLabels is an array holding labels
   * which should be traversed before any other labels in the hierarchy.
   * This could include labels placed during a previous rendering or
   * a label located under the mouse pointer. You may pass a null pointer.
   */
  virtual void Begin(svtkIdTypeArray*) {}

  /**
   * Advance the iterator.
   */
  virtual void Next() {}

  /**
   * Returns true if the iterator is at the end.
   */
  virtual bool IsAtEnd() { return true; }

  /**
   * Retrieves the current label location.
   */
  virtual void GetPoint(double x[3]);

  /**
   * Retrieves the current label size.
   */
  virtual void GetSize(double sz[2]);

  /**
   * Retrieves the current label maximum width in world coordinates.
   */
  virtual void GetBoundedSize(double sz[2]);

  /**
   * Retrieves the current label type.
   */
  virtual int GetType();

  /**
   * Retrieves the current label string.
   */
  virtual svtkStdString GetLabel();

  /**
   * Retrieves the current label as a unicode string.
   */
  virtual svtkUnicodeString GetUnicodeLabel();

  /**
   * Retrieves the current label orientation.
   */
  virtual double GetOrientation();

  /**
   * Retrieves the current label id.
   */
  virtual svtkIdType GetLabelId() { return -1; }

  //@{
  /**
   * Get the label hierarchy associated with the current label.
   */
  svtkGetObjectMacro(Hierarchy, svtkLabelHierarchy);
  //@}

  /**
   * Sets a polydata to fill with geometry representing
   * the bounding boxes of the traversed octree nodes.
   */
  virtual void SetTraversedBounds(svtkPolyData*);

  /**
   * Retrieve the coordinates of the center of the current hierarchy node
   * and the size of the node.
   * Nodes are n-cubes, so the size is the length of any edge of the cube.
   * This is used by BoxNode().
   */
  virtual void GetNodeGeometry(double ctr[3], double& size) = 0;

  /**
   * Add a representation to TraversedBounds for the current octree node.
   * This should be called by subclasses inside Next().
   * Does nothing if TraversedBounds is NULL.
   */
  virtual void BoxNode();

  /**
   * Add a representation for all existing octree nodes to the specified polydata.
   * This is equivalent to setting TraversedBounds, iterating over the entire hierarchy,
   * and then resetting TraversedBounds to its original value.
   */
  virtual void BoxAllNodes(svtkPolyData*);

  //@{
  /**
   * Set/get whether all nodes in the hierarchy should be added to the TraversedBounds
   * polydata or only those traversed.
   * When non-zero, all nodes will be added.
   * By default, AllBounds is 0.
   */
  svtkSetMacro(AllBounds, int);
  svtkGetMacro(AllBounds, int);
  //@}

protected:
  svtkLabelHierarchyIterator();
  ~svtkLabelHierarchyIterator() override;

  void BoxNodeInternal3(const double* ctr, double sz);
  void BoxNodeInternal2(const double* ctr, double sz);

  /**
   * The hierarchy being traversed by this iterator.
   */
  virtual void SetHierarchy(svtkLabelHierarchy* h);

  svtkLabelHierarchy* Hierarchy;
  svtkPolyData* TraversedBounds;
  double BoundsFactor;
  int AllBounds;
  int AllBoundsRecorded;

private:
  svtkLabelHierarchyIterator(const svtkLabelHierarchyIterator&) = delete;
  void operator=(const svtkLabelHierarchyIterator&) = delete;
};

#endif // svtkLabelHierarchyIterator_h
