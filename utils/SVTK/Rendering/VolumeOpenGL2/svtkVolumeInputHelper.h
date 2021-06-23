/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeInputHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkVolumeInputHelper
 * @brief Convenience container for internal structures specific to a volume
 * input.
 *
 * This class stores together svtkVolumeTexture, Lookup tables internal
 * and other input specific parameters. It also provides convenience methods
 * to manage the lookup tables.
 *
 * @warning This is an internal class of svtkOpenGLGPUVolumeRayCastMapper. It
 * assumes there is an active OpenGL context in methods involving GL calls
 * (MakeCurrent() is expected to be called in the mapper beforehand).
 *
 * \sa svtkOpenGLGPUVolumeRayCastMapper
 */
#ifndef svtkVolumeInputHelper_h
#define svtkVolumeInputHelper_h
#ifndef __SVTK_WRAP__
#include <map>

#include "svtkOpenGLVolumeLookupTables.h"
#include "svtkSmartPointer.h" // For SmartPointer
#include "svtkTimeStamp.h"    // For TimeStamp

class svtkOpenGLVolumeGradientOpacityTable;
class svtkOpenGLVolumeOpacityTable;
class svtkOpenGLVolumeRGBTable;
class svtkOpenGLVolumeTransferFunction2D;
class svtkRenderer;
class svtkShaderProgram;
class svtkVolume;
class svtkVolumeTexture;
class svtkWindow;

class svtkVolumeInputHelper
{
public:
  svtkVolumeInputHelper() = default;
  svtkVolumeInputHelper(svtkSmartPointer<svtkVolumeTexture> tex, svtkVolume* vol);

  void RefreshTransferFunction(
    svtkRenderer* ren, const int uniformIndex, const int blendMode, const float samplingDist);
  void ForceTransferInit();

  void ActivateTransferFunction(svtkShaderProgram* prog, const int blendMode);
  void DeactivateTransferFunction(const int blendMode);

  void ReleaseGraphicsResources(svtkWindow* window);

  svtkSmartPointer<svtkVolumeTexture> Texture;
  svtkVolume* Volume = nullptr;

  /**
   * Defines the various component modes supported by
   * svtkGPUVolumeRayCastMapper.
   */
  enum ComponentMode
  {
    INVALID = 0,
    INDEPENDENT = 1,
    LA = 2,
    RGBA = 4
  };
  int ComponentMode = INDEPENDENT;

  /**
   * Transfer function internal structures and helpers.
   */
  svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeGradientOpacityTable> >
    GradientOpacityTables;
  svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeOpacityTable> > OpacityTables;
  svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeRGBTable> > RGBTables;
  svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeTransferFunction2D> >
    TransferFunctions2D;

  /**
   * Maps uniform texture variable names to its corresponding texture unit.
   */
  std::map<int, std::string> RGBTablesMap;
  std::map<int, std::string> OpacityTablesMap;
  std::map<int, std::string> GradientOpacityTablesMap;
  std::map<int, std::string> TransferFunctions2DMap;

  /**
   * These values currently stored in svtkGPUVolumeRCMapper but should be moved
   * into svtkVolumeProperty in order to store them closer to the relevant
   * transfer functions and separately for each input.
   */
  int ColorRangeType = 0;           // svtkGPUVolumeRayCastMapper::SCALAR
  int ScalarOpacityRangeType = 0;   // svtkGPUVolumeRayCastMapper::SCALAR
  int GradientOpacityRangeType = 0; // svtkGPUVolumeRayCastMapper::SCALAR

  /**
   * Stores the uniform variable name where the gradient will be stored for
   * this input in the fragment shader.
   */
  std::string GradientCacheName;

protected:
  void InitializeTransferFunction(svtkRenderer* ren, const int index);

  void CreateTransferFunction1D(svtkRenderer* ren, const int index);
  void CreateTransferFunction2D(svtkRenderer* ren, const int index);

  void UpdateTransferFunctions(svtkRenderer* ren, const int blendMode, const float samplingDist);
  int UpdateOpacityTransferFunction(svtkRenderer* ren, svtkVolume* vol, unsigned int component,
    const int blendMode, const float samplingDist);
  int UpdateColorTransferFunction(svtkRenderer* ren, svtkVolume* vol, unsigned int component);
  int UpdateGradientOpacityTransferFunction(
    svtkRenderer* ren, svtkVolume* vol, unsigned int component, const float samplingDist);
  void UpdateTransferFunction2D(svtkRenderer* ren, unsigned int component);

  void ReleaseGraphicsTransfer1D(svtkWindow* window);
  void ReleaseGraphicsTransfer2D(svtkWindow* window);

  svtkTimeStamp LutInit;
  bool InitializeTransfer = true;
};

#endif
#endif // svtkVolumeInputHelper_h
// SVTK-HeaderTest-Exclude: svtkVolumeInputHelper.h
