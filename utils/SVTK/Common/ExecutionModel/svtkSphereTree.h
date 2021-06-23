/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSphereTree
 * @brief   class to build and traverse sphere trees
 *
 * svtkSphereTree is a helper class used to build and traverse sphere
 * trees. Various types of trees can be constructed for different SVTK
 * dataset types, as well well as different approaches to organize
 * the tree into hierarchies.
 *
 * Typically building a complete sphere tree consists of two parts: 1)
 * creating spheres for each cell in the dataset, then 2) creating an
 * organizing hierarchy. The structure of the hierarchy varies depending on
 * the topological characteristics of the dataset.
 *
 * Once the tree is constructed, various geometric operations are available
 * for quickly selecting cells based on sphere tree operations; for example,
 * process all cells intersecting a plane (i.e., use the sphere tree to identify
 * candidate cells for plane intersection).
 *
 * This class does not necessarily create optimal sphere trees because
 * some of its requirements (fast build time, provide simple reference
 * code, a single bounding sphere per cell, etc.) precludes optimal
 * performance. It is also oriented to computing on cells versus the
 * classic problem of collision detection for polygonal models. For
 * more information you want to read Gareth Bradshaw's PhD thesis
 * "Bounding Volume Hierarchies for Level-of-Detail Collision
 * Handling" which does a nice job of laying out the challenges and
 * important algorithms relative to sphere trees and BVH (bounding
 * volume hierarchies).
 *
 * @sa
 * svtkSphereTreeFilter svtkPlaneCutter
 */

#ifndef svtkSphereTree_h
#define svtkSphereTree_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkObject.h"
#include "svtkPlane.h" // to specify the cutting plane

class svtkDoubleArray;
class svtkDataArray;
class svtkIdList;
class svtkDataSet;
class svtkStructuredGrid;
class svtkUnstructuredGrid;
class svtkTimeStamp;
struct svtkSphereTreeHierarchy;

#define SVTK_MAX_SPHERE_TREE_RESOLUTION 10
#define SVTK_MAX_SPHERE_TREE_LEVELS 20

// SVTK Class proper
class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkSphereTree : public svtkObject
{
public:
  /**
   * Instantiate the sphere tree.
   */
  static svtkSphereTree* New();

  //@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  svtkTypeMacro(svtkSphereTree, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the dataset from which to build the sphere tree.
   */
  virtual void SetDataSet(svtkDataSet*);
  svtkGetObjectMacro(DataSet, svtkDataSet);
  //@}

  //@{
  /**
   * Build the sphere tree (if necessary) from the data set specified. The
   * build time is recorded so the sphere tree will only build if something has
   * changed. An alternative method is available to both set the dataset and
   * then build the sphere tree.
   */
  void Build();
  void Build(svtkDataSet* input);
  //@}

  //@{
  /**
   * Control whether the tree hierarchy is built. If not, then just
   * cell spheres are created (one for each cell).
   */
  svtkSetMacro(BuildHierarchy, bool);
  svtkGetMacro(BuildHierarchy, bool);
  svtkBooleanMacro(BuildHierarchy, bool);
  //@}

  //@{
  /**
   * Methods for cell selection based on a geometric query. Internally
   * different methods are used depending on the dataset type. The array
   * returned is set to non-zero for each cell that intersects the geometric
   * entity. SelectPoint marks all cells with a non-zero value that may
   * contain a point. SelectLine marks all cells that may intersect an
   * infinite line. SelectPlane marks all cells that may intersect with an
   * infinite plane.
   */
  const unsigned char* SelectPoint(double point[3], svtkIdType& numSelected);
  const unsigned char* SelectLine(double origin[3], double ray[3], svtkIdType& numSelected);
  const unsigned char* SelectPlane(double origin[3], double normal[3], svtkIdType& numSelected);
  //@}

  //@{
  /**
   * Methods for cell selection based on a geometric query. Internally
   * different methods are used depending on the dataset type. The method
   * pupulates an svtkIdList with cell ids that may satisfy the geometric
   * query (the user must provide a svtkLdList which the methods fill in).
   * SelectPoint lists all cells with a non-zero value that may contain a
   * point. SelectLine lists all cells that may intersect an infinite
   * line. SelectPlane lists all cells that may intersect with an infinite
   * plane.
   */
  void SelectPoint(double point[3], svtkIdList* cellIds);
  void SelectLine(double origin[3], double ray[3], svtkIdList* cellIds);
  void SelectPlane(double origin[3], double normal[3], svtkIdList* cellIds);
  //@}

  //@{
  /**
   * Sphere tree creation requires gathering spheres into groups. The
   * Resolution variable is a rough guide to the size of each group (the size
   * different meanings depending on the type of data (structured versus
   * unstructured). For example, in 3D structured data, blocks of resolution
   * Resolution^3 are created. By default the Resolution is three.
   */
  svtkSetClampMacro(Resolution, int, 2, SVTK_MAX_SPHERE_TREE_RESOLUTION);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Specify the maximum number of levels for the tree. By default, the
   * number of levels is set to ten. If the number of levels is set to one or
   * less, then no hierarchy is built (i.e., just the spheres for each cell
   * are created). Note that the actual level of the tree may be less than
   * this value depending on the number of cells and Resolution factor.
   */
  svtkSetClampMacro(MaxLevel, int, 1, SVTK_MAX_SPHERE_TREE_LEVELS);
  svtkGetMacro(MaxLevel, int);
  //@}

  /**
   * Get the current depth of the sphere tree. This value may change each
   * time the sphere tree is built and the branching factor (i.e.,
   * resolution) changes. Note that after building the sphere tree there are
   * [0,this->NumberOfLevels) defined levels.
   */
  svtkGetMacro(NumberOfLevels, int);

  //@{
  /**
   * Special methods to retrieve the sphere tree data. This is
   * generally used for debugging or with filters like
   * svtkSphereTreeFilter. Both methods return an array of double*
   * where four doubles represent a sphere (center + radius). In the
   * first method a sphere per cell is returned. In the second method
   * the user must also specify a level in the sphere tree (used to
   * retrieve the hierarchy of the tree). Note that null pointers can
   * be returned if the request is not consistent with the state of
   * the sphere tree.
   */
  const double* GetCellSpheres();
  const double* GetTreeSpheres(int level, svtkIdType& numSpheres);
  //@}

protected:
  svtkSphereTree();
  ~svtkSphereTree() override;

  // Data members
  svtkDataSet* DataSet;
  unsigned char* Selected;
  int Resolution;
  int MaxLevel;
  int NumberOfLevels;
  bool BuildHierarchy;

  // The tree and its hierarchy
  svtkDoubleArray* Tree;
  double* TreePtr;
  svtkSphereTreeHierarchy* Hierarchy;

  // Supporting data members
  double AverageRadius;   // average radius of cell sphere
  double SphereBounds[6]; // the dataset bounds computed from cell spheres
  svtkTimeStamp BuildTime; // time at which tree was built

  // Supporting methods
  void BuildTreeSpheres(svtkDataSet* input);
  void ExtractCellIds(const unsigned char* selected, svtkIdList* cellIds, svtkIdType numSelected);

  void BuildTreeHierarchy(svtkDataSet* input);
  void BuildStructuredHierarchy(svtkStructuredGrid* input, double* tree);
  void BuildUnstructuredHierarchy(svtkDataSet* input, double* tree);
  int SphereTreeType; // keep track of the type of tree hierarchy generated

private:
  svtkSphereTree(const svtkSphereTree&) = delete;
  void operator=(const svtkSphereTree&) = delete;
};

#endif
