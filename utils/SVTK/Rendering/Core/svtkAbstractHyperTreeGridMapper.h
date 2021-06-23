/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractHyperTreeGridMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractHyperTreeGridMapper
 * @brief   Abstract class for a HyperTreeGrid mapper
 *
 * svtkAbstractHyperTreeGridMapper is the abstract definition of a HyperTreeGrid mapper.
 * Several  basic types of volume mappers are supported.
 *
 * @sa
 * svtkHyperTreeGrid svtkUniformHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay and Meriadeg Perrinel,
 * NexGen Analytics 2018
 * This worked was based on an idea of Guenole Harel and Jacques-Bernard Lekien
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkAbstractHyperTreeGridMapper_h
#define svtkAbstractHyperTreeGridMapper_h

#include "svtkAbstractVolumeMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkBitArray;
class svtkDataArray;
class svtkMatrix4x4;
class svtkScalarsToColors;
class svtkRenderer;
class svtkUniformHyperTreeGrid;

class SVTKRENDERINGCORE_EXPORT svtkAbstractHyperTreeGridMapper : public svtkAbstractVolumeMapper
{
public:
  svtkTypeMacro(svtkAbstractHyperTreeGridMapper, svtkAbstractVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the input data or connection
   */
  virtual void SetInputData(svtkUniformHyperTreeGrid*);
  void SetInputConnection(int, svtkAlgorithmOutput*) override;
  void SetInputConnection(svtkAlgorithmOutput* input) override
  {
    this->SetInputConnection(0, input);
  }
  svtkUniformHyperTreeGrid* GetInput();
  //@}

  //@{
  /**
   * Set/Get the renderer attached to this HyperTreeGrid mapper
   */
  void SetRenderer(svtkRenderer*);
  svtkGetObjectMacro(Renderer, svtkRenderer);
  //@}

  /**
   * Set the scale factor
   */
  svtkSetMacro(Scale, double);

  //@{
  /**
   * Set/Get the color map attached to this HyperTreeGrid mapper
   * A linear lookup table is provided by default
   */
  void SetColorMap(svtkScalarsToColors*);
  svtkGetObjectMacro(ColorMap, svtkScalarsToColors);
  //@}

  //@{
  /**
   * Specify range in terms of scalar minimum and maximum.
   * These values are used to map scalars into lookup table
   * Has no effect when dimension > 2
   * Used only when ColorMap is a lookup table instance
   */
  void SetScalarRange(double, double);
  void SetScalarRange(double*);
  svtkGetVectorMacro(ScalarRange, double, 2);
  //@}

  /**
   * Get image size
   */
  svtkGetVectorMacro(ViewportSize, int, 2);

  /**
   * Get the mtime of this object.
   */
  svtkMTimeType GetMTime() override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override {}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  void Render(svtkRenderer*, svtkVolume*) override = 0;

protected:
  svtkAbstractHyperTreeGridMapper();
  ~svtkAbstractHyperTreeGridMapper() override;

  /**
   * Restrict input type to svtkUniformHyperTreeGrid instances
   */
  int FillInputPortInformation(int, svtkInformation*) override;

  /**
   * Reference to input scalars
   */
  svtkDataArray* Scalars;

  //@{
  /**
   * Keep track of coordinate conversion matrices
   */
  svtkMatrix4x4* WorldToViewMatrix;
  svtkMatrix4x4* ViewToWorldMatrix;
  //@}

  /**
   * Keep track of whether pixelize grid is current
   */
  bool MustUpdateGrid;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation;

  /**
   * Reference to the renderer being used
   */
  svtkRenderer* Renderer;

  /**
   * Scalar range for color lookup table when dimension < 3
   */
  double ScalarRange[2];

  /**
   * Color map used only when dimension < 3
   */
  svtkScalarsToColors* ColorMap;

  /**
   * Scale factor for adaptive view
   */
  double Scale;

  /**
   * Radius parameter for adaptive view
   */
  double Radius;

  /**
   * First axis parameter for adaptive view
   */
  unsigned int Axis1;

  /**
   * Second axis parameter for adaptive view
   */
  unsigned int Axis2;

  /**
   * Maximum depth parameter for adaptive view
   */
  int LevelMax;

  /**
   * Parallel projection parameter for adaptive view
   */
  bool ParallelProjection;

  /**
   * Last camera parallel scale for adaptive view
   */
  double LastCameraParallelScale;

  /**
   * Viewport size for computed image
   */
  int ViewportSize[2];

  /**
   * Last renderer size parameters for adaptive view
   */
  int LastRendererSize[2];

  /**
   * Last camera focal point coordinates for adaptive view
   */
  double LastCameraFocalPoint[3];

  /**
   * Keep track of current view orientation
   */
  int ViewOrientation;

  /**
   * Internal frame buffer
   */
  unsigned char* FrameBuffer;

  /**
   * Internal z-buffer
   */
  float* ZBuffer;

private:
  svtkAbstractHyperTreeGridMapper(const svtkAbstractHyperTreeGridMapper&) = delete;
  void operator=(const svtkAbstractHyperTreeGridMapper&) = delete;
};

#endif
