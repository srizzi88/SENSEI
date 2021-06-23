/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFlyingEdgesPlaneCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFlyingEdgesPlaneCutter
 * @brief   cut a volume with a plane and generate a
 * polygonal cut surface
 *
 * svtkFlyingEdgesPlaneCutter is a specialization of the FlyingEdges algorithm
 * to cut a volume with a single plane. It is designed for performance and
 * an exploratory, fast workflow.
 *
 * This algorithm is not only fast because it uses flying edges, but also
 * because it plays some "tricks" during processing. For example, rather
 * than evaluate the cut (plane) function on all volume points like svtkCutter
 * and its ilk do, this algorithm intersects the volume x-edges against the
 * plane to (potentially) generate the single intersection point. It then
 * quickly classifies the voxel edges as above, below, or straddling the cut
 * plane. Thus the number of plane evaluations is greatly reduced.
 *
 * For more information see svtkFlyingEdges3D and/or the paper "Flying Edges:
 * A High-Performance Scalable Isocontouring Algorithm" by Schroeder,
 * Maynard, Geveci. Proc. of LDAV 2015. Chicago, IL.
 *
 * @warning
 * This filter is specialized to 3D volumes. This implementation can produce
 * degenerate triangles (i.e., zero-area triangles).
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkFlyingEdges2D svtkFlyingEdges3D
 */

#ifndef svtkFlyingEdgesPlaneCutter_h
#define svtkFlyingEdgesPlaneCutter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImageData;
class svtkPlane;

class SVTKFILTERSCORE_EXPORT svtkFlyingEdgesPlaneCutter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard construction and print methods.
   */
  static svtkFlyingEdgesPlaneCutter* New();
  svtkTypeMacro(svtkFlyingEdgesPlaneCutter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * The modified time depends on the delegated cut plane.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the plane (an implicit function) to perform the cutting. The
   * definition of the plane (its origin and normal) is controlled via this
   * instance of svtkPlane.
   */
  virtual void SetPlane(svtkPlane*);
  svtkGetObjectMacro(Plane, svtkPlane);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. The normal generated is simply the
   * cut plane normal. By default this is disabled.
   */
  svtkSetMacro(ComputeNormals, svtkTypeBool);
  svtkGetMacro(ComputeNormals, svtkTypeBool);
  svtkBooleanMacro(ComputeNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to interpolate other attribute data besides the input
   * scalars (which are required). That is, as the isosurface is generated,
   * interpolate all other point attribute data across intersected edges.
   */
  svtkSetMacro(InterpolateAttributes, svtkTypeBool);
  svtkGetMacro(InterpolateAttributes, svtkTypeBool);
  svtkBooleanMacro(InterpolateAttributes, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

protected:
  svtkFlyingEdgesPlaneCutter();
  ~svtkFlyingEdgesPlaneCutter() override;

  svtkPlane* Plane;
  svtkTypeBool ComputeNormals;
  svtkTypeBool InterpolateAttributes;
  int ArrayComponent;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkFlyingEdgesPlaneCutter(const svtkFlyingEdgesPlaneCutter&) = delete;
  void operator=(const svtkFlyingEdgesPlaneCutter&) = delete;
};

#endif
