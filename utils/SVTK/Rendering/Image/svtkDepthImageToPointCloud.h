/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDepthImageToPointCloud.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDepthImageToPointCloud
 * @brief   convert a depth image into a point cloud
 *
 *
 * svtkDepthImageToPointCloud is a filter that acquires its input
 * from a depth image and converts it to point cloud represented as a
 * svtkPolyData. This can then be used in a visualization pipeline.
 *
 * The filter takes two input images, one of which is optional. The first
 * image is a (required) depth image containing z-buffer values. The second
 * image is an (optional) scalar image. The information in the z-buffer
 * image, plus a specified camera, is used to generate x-y-z coordinates of
 * the output point cloud (i.e., the points in a svtkPolyData). The second
 * scalar image is (optionally) output as scalars to the output point
 * cloud. Note that the depth image must be a single component image, with
 * values ranging between the near and far clipping range [-1,1].
 *
 * Note that if only a single input is provided, then the input is
 * interpreted in one of two ways. First, if the "ZBuffer" point data is
 * provided, then the input image is assumed to be color scalars with the
 * depth data provided in the "ZBuffer" data array. (This is consistent with
 * the svtkRendererSource filter with DepthValues enabled.) Otherwise, the
 * input image is assumed to be a depth image.
 *
 * It is (optionally) possible to cull points located on the near and far
 * clipping planes. This may better simulate the generation of a scanned
 * object point cloud.
 *
 * @warning
 * For the camera to transform the image depths into a point cloud, this
 * filter makes assumptions about the origin of the depth image (and
 * associated color scalar image). This class performs point by point
 * transformation. The view matrix is used to transform each pixel. IMPORTANT
 * NOTE: The transformation occurs by normalizing the image pixels into the
 * (-1,1) view space (depth values are passed through). The process follows the
 * svtkCoordinate class which is the standard for SVTK rendering
 * transformations. Subtle differences in whether the lower left pixel origin
 * are at the center of the pixel versus the lower-left corner of the pixel
 * will make slight differences in how pixels are transformed. (Similarly for
 * the upper right pixel as well). This half pixel difference can cause
 * transformation issues. (The code is commented appropriately.)
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkRendererSource svtkWindowToImageFilter svtkCamera svtkPolyData
 * svtkCoordinate
 */

#ifndef svtkDepthImageToPointCloud_h
#define svtkDepthImageToPointCloud_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingImageModule.h" // For export macro

class svtkCamera;

class SVTKRENDERINGIMAGE_EXPORT svtkDepthImageToPointCloud : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard instantiation, type and print methods.
   */
  static svtkDepthImageToPointCloud* New();
  svtkTypeMacro(svtkDepthImageToPointCloud, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Return the MTime also considering the camera.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Indicates what camera was used to generate the depth image. The camera
   * parameters define a transformation which is used to perform coordinate
   * conversion into the 3D x-y-z space of the point cloud.
   */
  void SetCamera(svtkCamera*);

  //@{
  /**
   * Returns the camera being used to generate the point cloud from the
   * depth image.
   */
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  //@{
  /**
   * Indicate whether to cull points that are located on the near clipping
   * plane. These typically are points that are part of the clipped foreground. By
   * default this is disabled.
   */
  svtkSetMacro(CullNearPoints, bool);
  svtkGetMacro(CullNearPoints, bool);
  svtkBooleanMacro(CullNearPoints, bool);
  //@}

  //@{
  /**
   * Indicate whether to cull points that are located on the far clipping
   * plane. These typically are points that are part of the background. By
   * default this is enabled.
   */
  svtkSetMacro(CullFarPoints, bool);
  svtkGetMacro(CullFarPoints, bool);
  svtkBooleanMacro(CullFarPoints, bool);
  //@}

  //@{
  /**
   * Indicate whether to output color scalar values along with the
   * point cloud (assuming that the scalar values are available on
   * input). By default this is enabled.
   */
  svtkSetMacro(ProduceColorScalars, bool);
  svtkGetMacro(ProduceColorScalars, bool);
  svtkBooleanMacro(ProduceColorScalars, bool);
  //@}

  //@{
  /**
   * Indicate whether to output a vertex cell array (i.e., Verts) in the
   * output point cloud. Some filters require this vertex cells to be
   * defined in order to execute properly. For example some mappers will
   * only render points if the vertex cells are defined.
   */
  svtkSetMacro(ProduceVertexCellArray, bool);
  svtkGetMacro(ProduceVertexCellArray, bool);
  svtkBooleanMacro(ProduceVertexCellArray, bool);
  //@}

  //@{
  /**
   * Set the desired precision for the output points.
   * See svtkAlgorithm::DesiredOutputPrecision for the available choices.
   * The default is double precision.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkDepthImageToPointCloud();
  ~svtkDepthImageToPointCloud() override;

  svtkCamera* Camera;
  bool CullNearPoints;
  bool CullFarPoints;
  bool ProduceColorScalars;
  bool ProduceVertexCellArray;
  int OutputPointsPrecision;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestUpdateExtent(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkDepthImageToPointCloud(const svtkDepthImageToPointCloud&) = delete;
  void operator=(const svtkDepthImageToPointCloud&) = delete;
};

#endif
