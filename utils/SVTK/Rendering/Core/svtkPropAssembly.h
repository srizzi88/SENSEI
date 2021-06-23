/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropAssembly.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPropAssembly
 * @brief   create hierarchies of props
 *
 * svtkPropAssembly is an object that groups props and other prop assemblies
 * into a tree-like hierarchy. The props can then be treated as a group
 * (e.g., turning visibility on and off).
 *
 * A svtkPropAssembly object can be used in place of an svtkProp since it is a
 * subclass of svtkProp. The difference is that svtkPropAssembly maintains a
 * list of other prop and prop assembly instances (its "parts") that form the
 * assembly. Note that this process is recursive: you can create groups
 * consisting of prop assemblies to arbitrary depth.
 *
 * svtkPropAssembly's and svtkProp's that compose a prop assembly need not be
 * added to a renderer's list of props, as long as the parent assembly is in
 * the prop list. This is because they are automatically renderered during
 * the hierarchical traversal process.
 *
 * @warning
 * svtkPropAssemblies can consist of hierarchies of assemblies, where one
 * actor or assembly used in one hierarchy is also used in other
 * hierarchies. However, make that there are no cycles (e.g.,
 * parent->child->parent), this will cause program failure.
 *
 * @sa
 * svtkProp3D svtkActor svtkAssembly svtkActor2D svtkVolume
 */

#ifndef svtkPropAssembly_h
#define svtkPropAssembly_h

#include "svtkProp.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkPropAssembly : public svtkProp
{
public:
  svtkTypeMacro(svtkPropAssembly, svtkProp);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create with an empty parts list.
   */
  static svtkPropAssembly* New();

  /**
   * Add a part to the list of parts.
   */
  void AddPart(svtkProp*);

  /**
   * Remove a part from the list of parts,
   */
  void RemovePart(svtkProp*);

  /**
   * Return the list of parts.
   */
  svtkPropCollection* GetParts();

  //@{
  /**
   * Render this assembly and all its parts.  The rendering process is
   * recursive. The parts of each assembly are rendered only if the
   * visibility for the prop is turned on.
   */
  int RenderOpaqueGeometry(svtkViewport* ren) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* ren) override;
  int RenderVolumetricGeometry(svtkViewport* ren) override;
  int RenderOverlay(svtkViewport* ren) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Get the bounds for this prop assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   * May return NULL in some cases (meaning the bounds is undefined).
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;

  /**
   * Shallow copy of this svtkPropAssembly.
   */
  void ShallowCopy(svtkProp* Prop) override;

  /**
   * Override default GetMTime method to also consider all of the
   * prop assembly's parts.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Methods to traverse the paths (i.e., leaf nodes) of a prop
   * assembly. These methods should be contrasted to those that traverse the
   * list of parts using GetParts().  GetParts() returns a list of children
   * of this assembly, not necessarily the leaf nodes of the assembly. To use
   * the methods below - first invoke InitPathTraversal() followed by
   * repeated calls to GetNextPath().  GetNextPath() returns a NULL pointer
   * when the list is exhausted. (See the superclass svtkProp for more
   * information about paths.)
   */
  void InitPathTraversal() override;
  svtkAssemblyPath* GetNextPath() override;
  int GetNumberOfPaths() override;
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Overload the superclass' svtkProp BuildPaths() method.
   */
  void BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path) override;

protected:
  svtkPropAssembly();
  ~svtkPropAssembly() override;

  svtkPropCollection* Parts;
  double Bounds[6];

  // Support the BuildPaths() method,
  svtkTimeStamp PathTime;
  void UpdatePaths(); // apply transformations and properties recursively
private:
  svtkPropAssembly(const svtkPropAssembly&) = delete;
  void operator=(const svtkPropAssembly&) = delete;
};

#endif
