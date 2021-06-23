/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssembly.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAssembly
 * @brief   create hierarchies of svtkProp3Ds (transformable props)
 *
 * svtkAssembly is an object that groups svtkProp3Ds, its subclasses, and
 * other assemblies into a tree-like hierarchy. The svtkProp3Ds and
 * assemblies can then be transformed together by transforming just the root
 * assembly of the hierarchy.
 *
 * A svtkAssembly object can be used in place of an svtkProp3D since it is a
 * subclass of svtkProp3D. The difference is that svtkAssembly maintains a list
 * of svtkProp3D instances (its "parts") that form the assembly. Then, any
 * operation that transforms (i.e., scales, rotates, translates) the parent
 * assembly will transform all its parts.  Note that this process is
 * recursive: you can create groups consisting of assemblies and/or
 * svtkProp3Ds to arbitrary depth.
 *
 * To add an assembly to the renderer's list of props, you only need to
 * add the root of the assembly. During rendering, the parts of the
 * assembly are rendered during a hierarchical traversal process.
 *
 * @warning
 * Collections of assemblies are slower to render than an equivalent list
 * of actors. This is because to support arbitrary nesting of assemblies,
 * the state of the assemblies (i.e., transformation matrices) must
 * be propagated through the assembly hierarchy.
 *
 * @warning
 * Assemblies can consist of hierarchies of assemblies, where one actor or
 * assembly used in one hierarchy is also used in other hierarchies. However,
 * make that there are no cycles (e.g., parent->child->parent), this will
 * cause program failure.
 *
 * @warning
 * If you wish to create assemblies without any transformation (using the
 * assembly strictly as a grouping mechanism), then you may wish to
 * consider using svtkPropAssembly.
 *
 * @sa
 * svtkActor svtkTransform svtkMapper svtkPolyDataMapper svtkPropAssembly
 */

#ifndef svtkAssembly_h
#define svtkAssembly_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkAssemblyPaths;
class svtkProp3DCollection;
class svtkMapper;
class svtkProperty;
class svtkActor;

class SVTKRENDERINGCORE_EXPORT svtkAssembly : public svtkProp3D
{
public:
  static svtkAssembly* New();

  svtkTypeMacro(svtkAssembly, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a part to the list of parts.
   */
  void AddPart(svtkProp3D*);

  /**
   * Remove a part from the list of parts,
   */
  void RemovePart(svtkProp3D*);

  /**
   * Return the parts (direct descendants) of this assembly.
   */
  svtkProp3DCollection* GetParts() { return this->Parts; }

  //@{
  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(svtkPropCollection*) override;
  void GetVolumes(svtkPropCollection*) override;
  //@}

  //@{
  /**
   * Render this assembly and all its parts.
   * The rendering process is recursive.
   * Note that a mapper need not be defined. If not defined, then no geometry
   * will be drawn for this assembly. This allows you to create "logical"
   * assemblies; that is, assemblies that only serve to group and transform
   * its parts.
   */
  int RenderOpaqueGeometry(svtkViewport* ren) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* ren) override;
  int RenderVolumetricGeometry(svtkViewport* ren) override;
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

  //@{
  /**
   * Methods to traverse the parts of an assembly. Each part (starting from
   * the root) will appear properly transformed and with the correct
   * properties (depending upon the ApplyProperty and ApplyTransform ivars).
   * Note that the part appears as an instance of svtkProp. These methods
   * should be contrasted to those that traverse the list of parts using
   * GetParts().  GetParts() returns a list of children of this assembly, not
   * necessarily with the correct transformation or properties. To use the
   * methods below - first invoke InitPathTraversal() followed by repeated
   * calls to GetNextPath().  GetNextPath() returns a NULL pointer when the
   * list is exhausted.
   */
  void InitPathTraversal() override;
  svtkAssemblyPath* GetNextPath() override;
  int GetNumberOfPaths() override;
  //@}

  /**
   * Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  void GetBounds(double bounds[6]) { this->svtkProp3D::GetBounds(bounds); }
  double* GetBounds() SVTK_SIZEHINT(6) override;

  /**
   * Override default GetMTime method to also consider all of the
   * assembly's parts.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Shallow copy of an assembly. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE DO NOT USE THIS
   * METHOD OUTSIDE OF THE RENDERING PROCESS Overload the superclass' svtkProp
   * BuildPaths() method. Paths consist of an ordered sequence of actors,
   * with transformations properly concatenated.
   */
  void BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path) override;

protected:
  svtkAssembly();
  ~svtkAssembly() override;

  // Keep a list of direct descendants of the assembly hierarchy
  svtkProp3DCollection* Parts;

  // Support the BuildPaths() method. Caches last paths built for
  // performance.
  svtkTimeStamp PathTime;
  virtual void UpdatePaths(); // apply transformations and properties recursively

private:
  svtkAssembly(const svtkAssembly&) = delete;
  void operator=(const svtkAssembly&) = delete;
};

#endif
