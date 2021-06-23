/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkViewNode
 * @brief   a node within a SVTK scene graph
 *
 * This is the superclass for all nodes within a SVTK scene graph. It
 * contains the API for a node. It supports the essential operations such
 * as graph creation, state storage and traversal. Child classes adapt this
 * to SVTK's major rendering classes. Grandchild classes adapt those to
 * for APIs of different rendering libraries.
 */

#ifndef svtkViewNode_h
#define svtkViewNode_h

#include "svtkObject.h"
#include "svtkRenderingSceneGraphModule.h" // For export macro
#include "svtkWeakPointer.h"               //avoid ref loop to parent

class svtkCollection;
class svtkViewNodeFactory;
class svtkViewNodeCollection;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkViewNode : public svtkObject
{
public:
  svtkTypeMacro(svtkViewNode, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This is the SVTK class that this node stands in for.
   */
  svtkGetObjectMacro(Renderable, svtkObject);
  //@}

  /**
   * Builds myself.
   */
  virtual void Build(bool /* prepass */) {}

  /**
   * Ensures that my state agrees with my Renderable's.
   */
  virtual void Synchronize(bool /* prepass */) {}

  /**
   * Makes calls to make self visible.
   */
  virtual void Render(bool /*prepass*/) {}

  /**
   * Clear any cached data.
   */
  virtual void Invalidate(bool /*prepass*/) {}

  //@{
  /**
   * Access the node that owns this one.
   */
  virtual void SetParent(svtkViewNode*);
  virtual svtkViewNode* GetParent();
  //@}

  //@{
  /**
   * Access nodes that this one owns.
   */
  virtual void SetChildren(svtkViewNodeCollection*);
  svtkGetObjectMacro(Children, svtkViewNodeCollection);
  //@}

  //@{
  /**
   * A factory that creates particular subclasses for different
   * rendering back ends.
   */
  virtual void SetMyFactory(svtkViewNodeFactory*);
  svtkGetObjectMacro(MyFactory, svtkViewNodeFactory);
  //@}

  /**
   * Returns the view node that corresponding to the provided object
   * Will return NULL if a match is not found in self or descendents
   */
  svtkViewNode* GetViewNodeFor(svtkObject*);

  /**
   * Find the first parent/grandparent of the desired type
   */
  svtkViewNode* GetFirstAncestorOfType(const char* type);

  /**
   * Find the first child of the desired type
   */
  svtkViewNode* GetFirstChildOfType(const char* type);

  /**
   * Allow explicit setting of the renderable for a
   * view node.
   */
  virtual void SetRenderable(svtkObject*);

  // if you want to traverse your children in a specific order
  // or way override this method
  virtual void Traverse(int operation);

  virtual void TraverseAllPasses();

  /**
   * Allows smart caching
   */
  svtkMTimeType RenderTime;

  /**
   * internal mechanics of graph traversal and actions
   */
  enum operation_type
  {
    noop,
    build,
    synchronize,
    render,
    invalidate
  };

protected:
  svtkViewNode();
  ~svtkViewNode() override;

  static const char* operation_type_strings[];

  void Apply(int operation, bool prepass);

  //@{
  /**
   * convenience method to add node or nodes
   * if missing from our current list
   */
  void AddMissingNode(svtkObject* obj);
  void AddMissingNodes(svtkCollection* col);
  //@}

  //@{
  /**
   * Called first before adding missing nodes.
   * Keeps track of the nodes that should be in the collection
   */
  void PrepareNodes();
  svtkCollection* PreparedNodes;
  //@}

  /**
   * Called after PrepareNodes and AddMissingNodes
   * removes any extra leftover nodes
   */
  void RemoveUnusedNodes();

  /**
   * Create the correct ViewNode subclass for the passed in object.
   */
  virtual svtkViewNode* CreateViewNode(svtkObject* obj);

  svtkObject* Renderable;
  svtkWeakPointer<svtkViewNode> Parent;
  svtkViewNodeCollection* Children;
  svtkViewNodeFactory* MyFactory;

  friend class svtkViewNodeFactory;

private:
  svtkViewNode(const svtkViewNode&) = delete;
  void operator=(const svtkViewNode&) = delete;
};

#endif
