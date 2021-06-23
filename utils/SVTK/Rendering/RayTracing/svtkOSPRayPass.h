/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayPass
 * @brief   a render pass that uses OSPRay instead of GL
 *
 * This is a render pass that can be put into a svtkRenderWindow which makes
 * it use OSPRay instead of OpenGL to render. Adding/Removing the pass
 * will swap back and forth between the two.
 *
 *  OSPRay MPI - OSPRay can use its own internal MPI layer to replicate
 *    the scene data across mpi processes and composite the image.
 *    This results in linear performance scaling and supports secondary
 *    rays.  To run in this mode, a special environment variable is supplied
 *    called SVTKOSPRAY_ARGS where commandline flags can be inserted for
 *    OSPRay's init call.  As an example of this, below is a commandline
 *    for running paraview on localhost, but having OSPRay's rendering
 *    occur on 2 remote nodes.  ospray_mpi_worker is a separate application
 *    supplied with OSPRay binary packages or when built with MPI support
 *    from source.
 *    'mpirun -ppn 1 -hosts localhost SVTKOSPRAY_ARGS="-osp:mpi"
 *      ./paraview : -hosts n1, n2 ./ospray_mpi_worker -osp:mpi'
 */

#ifndef svtkOSPRayPass_h
#define svtkOSPRayPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingRayTracingModule.h" // For export macro

#include <string> // for std::string

class svtkCameraPass;
class svtkLightsPass;
class svtkOSPRayPassInternals;
class svtkOSPRayRendererNode;
class svtkOverlayPass;
class svtkRenderPassCollection;
class svtkSequencePass;
class svtkVolumetricPass;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayPass : public svtkRenderPass
{
public:
  static svtkOSPRayPass* New();
  svtkTypeMacro(svtkOSPRayPass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state s.
   */
  virtual void Render(const svtkRenderState* s) override;

  //@{
  /**
   * Tells the pass what it will render.
   */
  void SetSceneGraph(svtkOSPRayRendererNode*);
  svtkGetObjectMacro(SceneGraph, svtkOSPRayRendererNode);
  //@}

  /**
   * Called by the internals of this class
   */
  virtual void RenderInternal(const svtkRenderState* s);

  //@{
  /**
   * Wrapper around ospray's init and shutdown that protect
   * with a reference count.
   */
  //@}
  static void RTInit();
  static void RTShutdown();

  /**
   * A run time query to see if a particular backend is available.
   * Eg. "OSPRay raycaster", "OSPRay pathtracer" or "OptiX pathtracer".
   */
  static bool IsBackendAvailable(const char* name);

protected:
  /**
   * Default constructor.
   */
  svtkOSPRayPass();

  /**
   * Destructor.
   */
  ~svtkOSPRayPass() override;

  svtkOSPRayRendererNode* SceneGraph;
  svtkCameraPass* CameraPass;
  svtkLightsPass* LightsPass;
  svtkOverlayPass* OverlayPass;
  svtkVolumetricPass* VolumetricPass;
  svtkSequencePass* SequencePass;
  svtkRenderPassCollection* RenderPassCollection;

private:
  svtkOSPRayPass(const svtkOSPRayPass&) = delete;
  void operator=(const svtkOSPRayPass&) = delete;

  svtkOSPRayPassInternals* Internal;
  std::string PreviousType;
  static int RTDeviceRefCount;
};

#endif
