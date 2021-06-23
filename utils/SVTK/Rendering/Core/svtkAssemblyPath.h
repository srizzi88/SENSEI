/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssemblyPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAssemblyPath
 * @brief   a list of nodes that form an assembly path
 *
 * svtkAssemblyPath represents an ordered list of assembly nodes that
 * represent a fully evaluated assembly path. This class is used primarily
 * for picking. Note that the use of this class is to add one or more
 * assembly nodes to form the path. (An assembly node consists of an instance
 * of svtkProp and svtkMatrix4x4, the matrix may be NULL.) As each node is
 * added, the matrices are concatenated to create a final, evaluated matrix.
 *
 * @sa
 * svtkAssemblyNode svtkAssembly svtkActor svtkMatrix4x4 svtkProp svtkAbstractPicker
 */

#ifndef svtkAssemblyPath_h
#define svtkAssemblyPath_h

#include "svtkAssemblyNode.h" // used for inlines
#include "svtkCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkMatrix4x4;
class svtkTransform;
class svtkProp;

class SVTKRENDERINGCORE_EXPORT svtkAssemblyPath : public svtkCollection
{
public:
  svtkTypeMacro(svtkAssemblyPath, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate empty path with identify matrix.
   */
  static svtkAssemblyPath* New();

  /**
   * Convenience method adds a prop and matrix together,
   * creating an assembly node transparently. The matrix
   * pointer m may be NULL. Note: that matrix is the one,
   * if any, associated with the prop.
   */
  void AddNode(svtkProp* p, svtkMatrix4x4* m);

  /**
   * Get the next assembly node in the list. The node returned
   * contains a pointer to a prop and a 4x4 matrix. The matrix
   * is evaluated based on the preceding assembly hierarchy
   * (i.e., the matrix is not necessarily as the same as the
   * one that was added with AddNode() because of the
   * concatenation of matrices in the assembly hierarchy).
   */
  svtkAssemblyNode* GetNextNode();

  /**
   * Get the first assembly node in the list. See the comments for
   * GetNextNode() regarding the contents of the returned node. (Note: This
   * node corresponds to the svtkProp associated with the svtkRenderer.
   */
  svtkAssemblyNode* GetFirstNode();

  /**
   * Get the last assembly node in the list. See the comments
   * for GetNextNode() regarding the contents of the returned node.
   */
  svtkAssemblyNode* GetLastNode();

  /**
   * Delete the last assembly node in the list. This is like
   * a stack pop.
   */
  void DeleteLastNode();

  /**
   * Perform a shallow copy (reference counted) on the
   * incoming path.
   */
  void ShallowCopy(svtkAssemblyPath* path);

  /**
   * Override the standard GetMTime() to check for the modified times
   * of the nodes in this path.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkAssemblyNode* GetNextNode(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkAssemblyNode*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkAssemblyPath();
  ~svtkAssemblyPath() override;

  void AddNode(svtkAssemblyNode* n); // Internal method adds assembly node
  svtkTransform* Transform;          // Used to perform matrix concatenation
  svtkProp* TransformedProp;         // A transformed prop used to do the rendering

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

private:
  svtkAssemblyPath(const svtkAssemblyPath&) = delete;
  void operator=(const svtkAssemblyPath&) = delete;
};

#endif
