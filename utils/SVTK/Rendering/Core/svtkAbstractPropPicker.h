/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractPropPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractPropPicker
 * @brief   abstract API for pickers that can pick an instance of svtkProp
 *
 * svtkAbstractPropPicker is an abstract superclass for pickers that can pick
 * an instance of svtkProp. Some pickers, like svtkWorldPointPicker (not a
 * subclass of this class), cannot identify the prop that is
 * picked. Subclasses of svtkAbstractPropPicker return a prop in the form of a
 * svtkAssemblyPath when a pick is invoked. Note that an svtkAssemblyPath
 * contain a list of svtkAssemblyNodes, each of which in turn contains a
 * reference to a svtkProp and a 4x4 transformation matrix. The path fully
 * describes the entire pick path, so you can pick assemblies or portions of
 * assemblies, or just grab the tail end of the svtkAssemblyPath (which is the
 * picked prop).
 *
 * @warning
 * Because a svtkProp can be placed into different assemblies, or even in
 * different leaf positions of the same assembly, the svtkAssemblyPath is
 * used to fully qualify exactly which use of the svtkProp was picked,
 * including its position (since svtkAssemblyPath includes a transformation
 * matrix per node).
 *
 * @warning
 * The class returns information about picked actors, props, etc. Note that
 * what is returned by these methods is the top level of the assembly
 * path. This can cause a lot of confusion! For example, if you pick a
 * svtkAssembly, and the returned svtkAssemblyPath has as a leaf a svtkActor,
 * then if you invoke GetActor(), you will get NULL, even though an actor was
 * indeed picked. (GetAssembly() will return something.) Note that the safest
 * thing to do is to do a GetViewProp(), which will always return something if
 * something was picked. A better way to manage picking is to work with
 * svtkAssemblyPath, since this completely defines the pick path from top to
 * bottom in a assembly hierarchy, and avoids confusion when the same prop is
 * used in different assemblies.
 *
 * @warning
 * The returned assembly paths refer to assembly nodes that in turn refer
 * to svtkProp and svtkMatrix. This association to svtkProp is not a reference
 * counted association, meaning that dangling references are possible if
 * you do a pick, get an assembly path, and then delete a svtkProp. (Reason:
 * assembly paths create many self-referencing loops that destroy reference
 * counting.)
 *
 * @sa
 * svtkPropPicker svtkPicker svtkWorldPointPicker svtkCellPicker svtkPointPicker
 * svtkAssemblyPath svtkAssemblyNode svtkAssemblyPaths svtkAbstractPicker
 * svtkRenderer
 */

#ifndef svtkAbstractPropPicker_h
#define svtkAbstractPropPicker_h

#include "svtkAbstractPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkProp;
class svtkPropAssembly;
class svtkAssembly;
class svtkActor;
class svtkVolume;
class svtkProp3D;
class svtkAssemblyPath;
class svtkActor2D;

class SVTKRENDERINGCORE_EXPORT svtkAbstractPropPicker : public svtkAbstractPicker
{
public:
  svtkTypeMacro(svtkAbstractPropPicker, svtkAbstractPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Return the svtkAssemblyPath that has been picked. The assembly path lists
   * all the svtkProps that form an assembly. If no assembly is present, then
   * the assembly path will have one node (which is the picked prop). The
   * set method is used internally to set the path. (Note: the structure of
   * an assembly path is a collection of svtkAssemblyNode, each node pointing
   * to a svtkProp and (possibly) a transformation matrix.)
   */
  virtual void SetPath(svtkAssemblyPath*);
  svtkGetObjectMacro(Path, svtkAssemblyPath);
  //@}

  // The following are convenience methods to maintain API with older
  // versions of SVTK, and to allow query for the return type of a pick. Note:
  // the functionality of these methods can also be obtained by using the
  // returned svtkAssemblyPath and using the IsA() to determine type.

  /**
   * Return the svtkProp that has been picked. If NULL, nothing was picked.
   * If anything at all was picked, this method will return something.
   */
  virtual svtkProp* GetViewProp();

  /**
   * Return the svtkProp that has been picked. If NULL, no svtkProp3D was picked.
   */
  virtual svtkProp3D* GetProp3D();

  /**
   * Return the svtkActor that has been picked. If NULL, no actor was picked.
   */
  virtual svtkActor* GetActor();

  /**
   * Return the svtkActor2D that has been picked. If NULL, no actor2D was
   * picked.
   */
  virtual svtkActor2D* GetActor2D();

  /**
   * Return the svtkVolume that has been picked. If NULL, no volume was picked.
   */
  virtual svtkVolume* GetVolume();

  /**
   * Return the svtkAssembly that has been picked. If NULL, no assembly
   * was picked. (Note: the returned assembly is the first node in the
   * assembly path. If the path is one node long, then the assembly and
   * the prop are the same, assuming that the first node is a svtkAssembly.)
   */
  virtual svtkAssembly* GetAssembly();

  /**
   * Return the svtkPropAssembly that has been picked. If NULL, no prop
   * assembly was picked. (Note: the returned prop assembly is the first node
   * in the assembly path. If the path is one node long, then the prop
   * assembly and the prop are the same, assuming that the first node is a
   * svtkPropAssembly.)
   */
  virtual svtkPropAssembly* GetPropAssembly();

protected:
  svtkAbstractPropPicker();
  ~svtkAbstractPropPicker() override;

  void Initialize() override;

  svtkAssemblyPath* Path; // this is what is picked, and includes the prop
private:
  svtkAbstractPropPicker(const svtkAbstractPropPicker&) = delete;
  void operator=(const svtkAbstractPropPicker&) = delete;
};

#endif
