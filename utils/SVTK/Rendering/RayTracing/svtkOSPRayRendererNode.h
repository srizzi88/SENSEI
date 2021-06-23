/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayRendererNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayRendererNode
 * @brief   links svtkRenderers to OSPRay
 *
 * Translates svtkRenderer state into OSPRay rendering calls
 */

#ifndef svtkOSPRayRendererNode_h
#define svtkOSPRayRendererNode_h

#include "svtkRendererNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro
#include <vector>                         // for ivars

#include "RTWrapper/RTWrapper.h" // for handle types

#ifdef SVTKOSPRAY_ENABLE_DENOISER
#include <OpenImageDenoise/oidn.hpp> // for denoiser structures
#endif

class svtkInformationDoubleKey;
class svtkInformationDoubleVectorKey;
class svtkInformationIntegerKey;
class svtkInformationObjectBaseKey;
class svtkInformationStringKey;
class svtkMatrix4x4;
class svtkOSPRayRendererNodeInternals;
class svtkOSPRayMaterialLibrary;
class svtkRenderer;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayRendererNode : public svtkRendererNode
{
public:
  static svtkOSPRayRendererNode* New();
  svtkTypeMacro(svtkOSPRayRendererNode, svtkRendererNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Builds myself.
   */
  virtual void Build(bool prepass) override;

  /**
   * Traverse graph in ospray's preferred order and render
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

  /**
   * Put my results into the correct place in the provided pixel buffer.
   */
  virtual void WriteLayer(unsigned char* buffer, float* zbuffer, int buffx, int buffy, int layer);

  // state beyond rendering core...

  /**
   * When present on renderer, controls the number of primary rays
   * shot per pixel
   * default is 1
   */
  static svtkInformationIntegerKey* SAMPLES_PER_PIXEL();

  //@{
  /**
   * Convenience method to set/get SAMPLES_PER_PIXEL on a svtkRenderer.
   */
  static void SetSamplesPerPixel(int, svtkRenderer* renderer);
  static int GetSamplesPerPixel(svtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, samples are clamped to this value before they
   * are accumulated into the framebuffer
   * default is 2.0
   */
  static svtkInformationDoubleKey* MAX_CONTRIBUTION();

  //@{
  /**
   * Convenience method to set/get MAX_CONTRIBUTION on a svtkRenderer.
   */
  static void SetMaxContribution(double, svtkRenderer* renderer);
  static double GetMaxContribution(svtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the maximum ray recursion depth
   * default is 20
   */
  static svtkInformationIntegerKey* MAX_DEPTH();

  //@{
  /**
   * Convenience method to set/get MAX_DEPTH on a svtkRenderer.
   */
  static void SetMaxDepth(int, svtkRenderer* renderer);
  static int GetMaxDepth(svtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, sample contributions below this value will be
   * neglected to speedup rendering
   * default is 0.01
   */
  static svtkInformationDoubleKey* MIN_CONTRIBUTION();

  //@{
  /**
   * Convenience method to set/get MIN_CONTRIBUTION on a svtkRenderer.
   */
  static void SetMinContribution(double, svtkRenderer* renderer);
  static double GetMinContribution(svtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the ray recursion depth at which to
   * start Russian roulette termination
   * default is 5
   */
  static svtkInformationIntegerKey* ROULETTE_DEPTH();

  //@{
  /**
   * Convenience method to set/get ROULETTE_DEPTH on a svtkRenderer.
   */
  static void SetRouletteDepth(int, svtkRenderer* renderer);
  static int GetRouletteDepth(svtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the threshold for adaptive accumulation
   * default is 0.3
   */
  static svtkInformationDoubleKey* VARIANCE_THRESHOLD();

  //@{
  /**
   * Convenience method to set/get VARIANCE_THRESHOLD on a svtkRenderer.
   */
  static void SetVarianceThreshold(double, svtkRenderer* renderer);
  static double GetVarianceThreshold(svtkRenderer* renderer);
  //@}

  //@{
  /**
   * When present on renderer, controls the number of ospray render calls
   * for each refresh.
   * default is 1
   */
  static svtkInformationIntegerKey* MAX_FRAMES();
  static void SetMaxFrames(int, svtkRenderer* renderer);
  static int GetMaxFrames(svtkRenderer* renderer);
  //@}

  //@{
  /**
   * Set the OSPRay renderer type to use (e.g. scivis vs. pathtracer)
   * default is scivis
   */
  static svtkInformationStringKey* RENDERER_TYPE();
  static void SetRendererType(std::string name, svtkRenderer* renderer);
  static std::string GetRendererType(svtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the number of ambient occlusion
   * samples shot per hit.
   * default is 4
   */
  static svtkInformationIntegerKey* AMBIENT_SAMPLES();
  //@{
  /**
   * Convenience method to set/get AMBIENT_SAMPLES on a svtkRenderer.
   */
  static void SetAmbientSamples(int, svtkRenderer* renderer);
  static int GetAmbientSamples(svtkRenderer* renderer);
  //@}

  /**
   * used to make the renderer add ospray's content onto GL rendered
   * content on the window
   */
  static svtkInformationIntegerKey* COMPOSITE_ON_GL();
  //@{
  /**
   * Convenience method to set/get COMPOSITE_ON_GL on a svtkRenderer.
   */
  static void SetCompositeOnGL(int, svtkRenderer* renderer);
  static int GetCompositeOnGL(svtkRenderer* renderer);
  //@}

  /**
   * World space direction of north pole for gradient and texture background.
   */
  static svtkInformationDoubleVectorKey* NORTH_POLE();
  //@{
  /**
   * Convenience method to set/get NORTH_POLE on a svtkRenderer.
   */
  static void SetNorthPole(double*, svtkRenderer* renderer);
  static double* GetNorthPole(svtkRenderer* renderer);
  //@}

  /**
   * World space direction of east pole for texture background.
   */
  static svtkInformationDoubleVectorKey* EAST_POLE();
  //@{
  /**
   * Convenience method to set/get EAST_POLE on a svtkRenderer.
   */
  static void SetEastPole(double*, svtkRenderer* renderer);
  static double* GetEastPole(svtkRenderer* renderer);
  //@}

  /**
   * Material Library attached to the renderer.
   */
  static svtkInformationObjectBaseKey* MATERIAL_LIBRARY();

  //@{
  /**
   * Convenience method to set/get Material library on a renderer.
   */
  static void SetMaterialLibrary(svtkOSPRayMaterialLibrary*, svtkRenderer* renderer);
  static svtkOSPRayMaterialLibrary* GetMaterialLibrary(svtkRenderer* renderer);
  //@}

  /**
   * Requested time to show in a renderer and to lookup in a temporal cache.
   */
  static svtkInformationDoubleKey* VIEW_TIME();
  //@{
  /**
   * Convenience method to set/get VIEW_TIME on a svtkRenderer.
   */
  static void SetViewTime(double, svtkRenderer* renderer);
  static double GetViewTime(svtkRenderer* renderer);
  //@}

  /**
   * Temporal cache size..
   */
  static svtkInformationIntegerKey* TIME_CACHE_SIZE();
  //@{
  /**
   * Convenience method to set/get TIME_CACHE_SIZE on a svtkRenderer.
   */
  static void SetTimeCacheSize(int, svtkRenderer* renderer);
  static int GetTimeCacheSize(svtkRenderer* renderer);
  //@}

  /**
   * Methods for other nodes to access
   */
  OSPModel GetOModel() { return this->OModel; }
  OSPRenderer GetORenderer() { return this->ORenderer; }
  void AddLight(OSPLight light) { this->Lights.push_back(light); }

  /**
   * Get the last rendered ColorBuffer
   */
  virtual void* GetBuffer() { return this->Buffer.data(); }

  /**
   * Get the last rendered ZBuffer
   */
  virtual float* GetZBuffer() { return this->ZBuffer.data(); }

  // Get the last renderer color buffer as an OpenGL texture.
  virtual int GetColorBufferTextureGL() { return this->ColorBufferTex; }

  // Get the last renderer depth buffer as an OpenGL texture.
  virtual int GetDepthBufferTextureGL() { return this->DepthBufferTex; }

  // if you want to traverse your children in a specific order
  // or way override this method
  virtual void Traverse(int operation) override;

  /**
   * Convenience method to get and downcast renderable.
   */
  static svtkOSPRayRendererNode* GetRendererNode(svtkViewNode*);
  svtkRenderer* GetRenderer();
  RTW::Backend* GetBackend();

  /**
   * Accumulation threshold when above which denoising kicks in.
   */
  static svtkInformationIntegerKey* DENOISER_THRESHOLD();
  //@{
  /**
   * Convenience method to set/get DENOISER_THRESHOLD on a svtkRenderer.
   */
  static void SetDenoiserThreshold(int, svtkRenderer* renderer);
  static int GetDenoiserThreshold(svtkRenderer* renderer);
  //@}

  //@{
  /**
   * Enable denoising (if supported).
   */
  static svtkInformationIntegerKey* ENABLE_DENOISER();
  /**
   * Convenience method to set/get ENABLE_DENOISER on a svtkRenderer.
   */
  static void SetEnableDenoiser(int, svtkRenderer* renderer);
  static int GetEnableDenoiser(svtkRenderer* renderer);
  //@}

  //@{
  /**
   * Control use of the path tracer backplate and environmental background.
   * 0 means neither is shown, 1 means only backplate is shown,
   * 2 (the default) means only environment is shown, 3 means that
   * both are enabled and therefore backblate shows on screen but
   * actors acquire color from the environment.
   */
  static svtkInformationIntegerKey* BACKGROUND_MODE();
  static void SetBackgroundMode(int, svtkRenderer* renderer);
  static int GetBackgroundMode(svtkRenderer* renderer);
  //@}
protected:
  svtkOSPRayRendererNode();
  ~svtkOSPRayRendererNode() override;

  /**
   * Denoise the colors stored in ColorBuffer and put into Buffer
   */
  void Denoise();

  // internal structures
#ifdef SVTKOSPRAY_ENABLE_DENOISER
  std::vector<float> Buffer;
#else
  std::vector<unsigned char> Buffer;
#endif
  std::vector<float> ZBuffer;

  int ColorBufferTex;
  int DepthBufferTex;

  OSPModel OModel;
  OSPRenderer ORenderer;
  OSPFrameBuffer OFrameBuffer;
  OSPData OLightArray;
  int ImageX, ImageY;
  std::vector<OSPLight> Lights;
  int NumActors;
  bool ComputeDepth;
  bool Accumulate;
  bool CompositeOnGL;
  std::vector<float> ODepthBuffer;
  int AccumulateCount;
  int ActorCount;
  svtkMTimeType AccumulateTime;
  svtkMatrix4x4* AccumulateMatrix;
  svtkOSPRayRendererNodeInternals* Internal;
  std::string PreviousType;

#ifdef SVTKOSPRAY_ENABLE_DENOISER
  oidn::DeviceRef DenoiserDevice;
  oidn::FilterRef DenoiserFilter;
#endif
  bool DenoiserDirty{ true };
  std::vector<osp::vec4f> ColorBuffer;
  std::vector<osp::vec3f> NormalBuffer;
  std::vector<osp::vec3f> AlbedoBuffer;
  std::vector<osp::vec4f> DenoisedBuffer;

private:
  svtkOSPRayRendererNode(const svtkOSPRayRendererNode&) = delete;
  void operator=(const svtkOSPRayRendererNode&) = delete;
};

#endif
