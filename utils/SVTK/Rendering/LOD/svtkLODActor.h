/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLODActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLODActor
 * @brief   an actor that supports multiple levels of detail
 *
 * svtkLODActor is an actor that stores multiple levels of detail (LOD) and
 * can automatically switch between them. It selects which level of detail to
 * use based on how much time it has been allocated to render. Currently a
 * very simple method of TotalTime/NumberOfActors is used. (In the future
 * this should be modified to dynamically allocate the rendering time between
 * different actors based on their needs.)
 *
 * There are three levels of detail by default. The top level is just the
 * normal data. The lowest level of detail is a simple bounding box outline
 * of the actor. The middle level of detail is a point cloud of a fixed
 * number of points that have been randomly sampled from the mapper's input
 * data. Point attributes are copied over to the point cloud. These two
 * lower levels of detail are accomplished by creating instances of a
 * svtkOutlineFilter (low-res) and svtkMaskPoints (medium-res). Additional
 * levels of detail can be add using the AddLODMapper() method.
 *
 * To control the frame rate, you typically set the svtkRenderWindowInteractor
 * DesiredUpdateRate and StillUpdateRate. This then will cause svtkLODActor
 * to adjust its LOD to fulfill the requested update rate.
 *
 * For greater control on levels of detail, see also svtkLODProp3D. That
 * class allows arbitrary definition of each LOD.
 *
 * @warning
 * If you provide your own mappers, you are responsible for setting its
 * ivars correctly, such as ScalarRange, LookupTable, and so on.
 *
 * @warning
 * On some systems the point cloud rendering (the default, medium level of
 * detail) can result in points so small that they can hardly be seen. In
 * this case, use the GetProperty()->SetPointSize() method to increase the
 * rendered size of the points.
 *
 * @sa
 * svtkActor svtkRenderer svtkLODProp3D
 */

#ifndef svtkLODActor_h
#define svtkLODActor_h

#include "svtkActor.h"
#include "svtkRenderingLODModule.h" // For export macro

class svtkMapper;
class svtkMapperCollection;
class svtkPolyDataAlgorithm;
class svtkPolyDataMapper;
class svtkRenderer;
class svtkViewport;
class svtkWindow;

class SVTKRENDERINGLOD_EXPORT svtkLODActor : public svtkActor
{
public:
  svtkTypeMacro(svtkLODActor, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a svtkLODActor with the following defaults: origin(0,0,0)
   * position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
   * orientation=(0,0,0). NumberOfCloudPoints is set to 150.
   */
  static svtkLODActor* New();

  /**
   * This causes the actor to be rendered.
   * It, in turn, will render the actor's property and then mapper.
   */
  void Render(svtkRenderer*, svtkMapper*) override;

  /**
   * This method is used internally by the rendering process. We override
   * the superclass method to properly set the estimated render time.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Add another level of detail.
   * They do not have to be in any order of complexity.
   */
  void AddLODMapper(svtkMapper* mapper);

  //@{
  /**
   * You may plug in your own filters to decimate/subsample the input.
   * The default is to use a svtkOutlineFilter (low-res) and svtkMaskPoints
   * (medium-res).
   */
  virtual void SetLowResFilter(svtkPolyDataAlgorithm*);
  virtual void SetMediumResFilter(svtkPolyDataAlgorithm*);
  svtkGetObjectMacro(LowResFilter, svtkPolyDataAlgorithm);
  svtkGetObjectMacro(MediumResFilter, svtkPolyDataAlgorithm);
  //@}

  //@{
  /**
   * Set/Get the number of random points for the point cloud.
   */
  svtkGetMacro(NumberOfCloudPoints, int);
  svtkSetMacro(NumberOfCloudPoints, int);
  //@}

  //@{
  /**
   * All the mappers for different LODs are stored here.
   * The order is not important.
   */
  svtkGetObjectMacro(LODMappers, svtkMapperCollection);
  //@}

  /**
   * When this objects gets modified, this method also modifies the object.
   */
  void Modified() override;

  /**
   * Shallow copy of an LOD actor. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

protected:
  svtkLODActor();
  ~svtkLODActor() override;

  svtkActor* Device;
  svtkMapperCollection* LODMappers;

  // We can create our own LOD filters. The default is to use a
  //
  svtkPolyDataAlgorithm* LowResFilter;
  svtkPolyDataAlgorithm* MediumResFilter;
  svtkPolyDataMapper* LowMapper;
  svtkPolyDataMapper* MediumMapper;

  svtkTimeStamp BuildTime;
  int NumberOfCloudPoints;

  virtual void CreateOwnLODs();
  virtual void UpdateOwnLODs();
  virtual void DeleteOwnLODs();

private:
  svtkLODActor(const svtkLODActor&) = delete;
  void operator=(const svtkLODActor&) = delete;
};

#endif
