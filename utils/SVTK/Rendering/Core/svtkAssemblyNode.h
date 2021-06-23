/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssemblyNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAssemblyNode
 * @brief   represent a node in an assembly
 *
 * svtkAssemblyNode represents a node in an assembly. It is used by
 * svtkAssemblyPath to create hierarchical assemblies of props. The
 * props can be either 2D or 3D.
 *
 * An assembly node refers to a svtkProp, and possibly a svtkMatrix4x4.
 * Nodes are used by svtkAssemblyPath to build fully evaluated path
 * (matrices are concatenated through the path) that is used by picking
 * and other operations involving assemblies.
 *
 * @warning
 * The assembly node is guaranteed to contain a reference to an instance
 * of svtkMatrix4x4 if the prop referred to by the node is of type
 * svtkProp3D (or subclass). The matrix is evaluated through the assembly
 * path, so the assembly node's matrix is a function of its location in
 * the svtkAssemblyPath.
 *
 * @warning
 * svtkAssemblyNode does not reference count its association with svtkProp.
 * Therefore, do not create an assembly node, associate a prop with it,
 * delete the prop, and then try to dereference the prop. The program
 * will break! (Reason: svtkAssemblyPath (which uses svtkAssemblyNode)
 * create self-referencing loops that destroy reference counting.)
 *
 * @sa
 * svtkAssemblyPath svtkProp svtkPicker svtkMatrix4x4
 */

#ifndef svtkAssemblyNode_h
#define svtkAssemblyNode_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkProp;
class svtkMatrix4x4;

class SVTKRENDERINGCORE_EXPORT svtkAssemblyNode : public svtkObject
{
public:
  /**
   * Create an assembly node.
   */
  static svtkAssemblyNode* New();

  svtkTypeMacro(svtkAssemblyNode, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the prop that this assembly node refers to.
   */
  virtual void SetViewProp(svtkProp* prop);
  svtkGetObjectMacro(ViewProp, svtkProp);
  //@}

  //@{
  /**
   * Specify a transformation matrix associated with the prop.
   * Note: if the prop is not a type of svtkProp3D, then the
   * transformation matrix is ignored (and expected to be NULL).
   * Also, internal to this object the matrix is copied because
   * the matrix is used for computation by svtkAssemblyPath.
   */
  void SetMatrix(svtkMatrix4x4* matrix);
  svtkGetObjectMacro(Matrix, svtkMatrix4x4);
  //@}

  /**
   * Override the standard GetMTime() to check for the modified times
   * of the prop and matrix.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkAssemblyNode();
  ~svtkAssemblyNode() override;

private:
  svtkProp* ViewProp;    // reference to svtkProp
  svtkMatrix4x4* Matrix; // associated matrix

private:
  void operator=(const svtkAssemblyNode&) = delete;
  svtkAssemblyNode(const svtkAssemblyNode&) = delete;
};

#endif
