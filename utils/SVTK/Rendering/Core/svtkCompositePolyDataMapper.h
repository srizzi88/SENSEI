/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositePolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositePolyDataMapper
 * @brief   a class that renders hierarchical polygonal data
 *
 * This class uses a set of svtkPolyDataMappers to render input data
 * which may be hierarchical. The input to this mapper may be
 * either svtkPolyData or a svtkCompositeDataSet built from
 * polydata. If something other than svtkPolyData is encountered,
 * an error message will be produced.
 * @sa
 * svtkPolyDataMapper
 */

#ifndef svtkCompositePolyDataMapper_h
#define svtkCompositePolyDataMapper_h

#include "svtkMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkPolyDataMapper;
class svtkInformation;
class svtkRenderer;
class svtkActor;
class svtkCompositePolyDataMapperInternals;

class SVTKRENDERINGCORE_EXPORT svtkCompositePolyDataMapper : public svtkMapper
{

public:
  static svtkCompositePolyDataMapper* New();
  svtkTypeMacro(svtkCompositePolyDataMapper, svtkMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Standard method for rendering a mapper. This method will be
   * called by the actor.
   */
  void Render(svtkRenderer* ren, svtkActor* a) override;

  //@{
  /**
   * Standard svtkProp method to get 3D bounds of a 3D prop
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) override { this->Superclass::GetBounds(bounds); }
  //@}

  /**
   * Release the underlying resources associated with this mapper
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * Some introspection on the type of data the mapper will render
   * used by props to determine if they should invoke the mapper
   * on a specific rendering pass.
   */
  bool HasOpaqueGeometry() override;
  bool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkCompositePolyDataMapper();
  ~svtkCompositePolyDataMapper() override;

  /**
   * We need to override this method because the standard streaming
   * demand driven pipeline is not what we want - we are expecting
   * hierarchical data as input
   */
  svtkExecutive* CreateDefaultExecutive() override;

  /**
   * Need to define the type of data handled by this mapper.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * This is the build method for creating the internal polydata
   * mapper that do the actual work
   */
  void BuildPolyDataMapper();

  /**
   * BuildPolyDataMapper uses this for each mapper. It is broken out so we can change types.
   */
  virtual svtkPolyDataMapper* MakeAMapper();

  /**
   * Need to loop over the hierarchy to compute bounds
   */
  void ComputeBounds();

  /**
   * Time stamp for computation of bounds.
   */
  svtkTimeStamp BoundsMTime;

  /**
   * These are the internal polydata mapper that do the
   * rendering. We save then so that they can keep their
   * display lists.
   */
  svtkCompositePolyDataMapperInternals* Internal;

  /**
   * Time stamp for when we need to update the
   * internal mappers
   */
  svtkTimeStamp InternalMappersBuildTime;

private:
  svtkCompositePolyDataMapper(const svtkCompositePolyDataMapper&) = delete;
  void operator=(const svtkCompositePolyDataMapper&) = delete;
};

#endif
