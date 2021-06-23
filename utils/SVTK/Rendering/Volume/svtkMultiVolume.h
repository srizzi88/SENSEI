/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkMultiVolume
 * @brief Represents a world axis-aligned bounding-box containing a set of
 * volumes in a rendered scene.
 *
 * svtkVolume instances registered in this class can be overlapping. They are
 * intended to be all rendered simultaneously by a svtkGPUVolumeRayCastMapper
 * (inputs should be set directly in the mapper).
 *
 * This class holds the full transformation of a bounding-box containing
 * all of the registered volumes.
 *
 *      + TexToBBox : Texture-to-Data (scaling)
 *      + Matrix : Data-to-World (translation)
 *
 * @note This class is intended to be used only by mappers supporting multiple
 * inputs.
 *
 * @sa svtkVolume svtkAbstractVolumeMapper svtkGPUVolumeRayCastMapper
 */
#ifndef svtkMultiVolume_h
#define svtkMultiVolume_h
#include <array>         // for std::array
#include <unordered_map> // For std::unordered_map

#include "svtkMatrix4x4.h"             // For Matrix
#include "svtkRenderingVolumeModule.h" // For export macro
#include "svtkSmartPointer.h"          // For svtkSmartPointer
#include "svtkVolume.h"

class svtkAbstractVolumeMapper;
class svtkBoundingBox;
class svtkMatrix4x4;
class svtkRenderer;
class svtkVolumeProperty;
class svtkWindow;
class svtkVolumeProperty;
class svtkAbstractVolumeMapper;

class SVTKRENDERINGVOLUME_EXPORT svtkMultiVolume : public svtkVolume
{
public:
  static svtkMultiVolume* New();
  svtkTypeMacro(svtkMultiVolume, svtkVolume);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Add / Remove a svtkVolume instance.
   */
  void SetVolume(svtkVolume* volume, int port = 0);
  svtkVolume* GetVolume(int port = 0);
  void RemoveVolume(int port) { this->SetVolume(nullptr, port); }
  //@}

  //@{
  /**
   * Given that this class represents a bounding-box only there is no property
   * directly associated with it (a cannot be set directly).
   * This instance will return the property of the volume registered in the 0th
   * port (or nullptr if no volume has been set).
   * \sa svtkVolume
   */
  void SetProperty(svtkVolumeProperty* property) override;
  svtkVolumeProperty* GetProperty() override;
  //@}

  /**
   * Computes the bounds of the box containing all of the svtkVolume instances.
   * Returns the bounds (svtkVolume::Bounds) in world coordinates [xmin, xmax,
   * ymin, ymax, zmin, zmax] but also keeps cached the bounds in data coordinates
   * (svtkMultiVolume::DataBounds).
   */
  double* GetBounds() override;

  /**
   * \sa svtkVolume
   */
  svtkMTimeType GetMTime() override;

  /**
   * Checks whether the svtkProp passed is another svtkMultiVolume and tries to
   * copy accordingly. Otherwise it falls back to svtkVolume::ShallowCopy.
   * \sa svtkVolume
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * As with other svtkProp3D, Matrix holds the transformation from data
   * coordinates to world coordinates.  Since this class represents an
   * axis-aligned bounding-box, this transformation only contains a translation
   * vector. Each registered svtkVolume contains its own transformation with
   * respect to the world coordinate system.
   * \sa svtkProp3D svtkVolume
   */
  using svtkVolume::GetMatrix;
  svtkMatrix4x4* GetMatrix() override { return this->Matrix; }

  /**
   * Returns the transformation from texture coordinates to data cooridinates
   * of the bounding-box. Since this class represents an axis-aligned bounding
   * -boxThis, this transformation only contains a scaling diagonal.
   */
  svtkMatrix4x4* GetTextureMatrix() { return this->TexToBBox.GetPointer(); }

  /**
   * Total bounds in data coordinates.
   */
  double* GetDataBounds() { return this->DataBounds.data(); }

  svtkMTimeType GetBoundsTime() { return this->BoundsComputeTime.GetMTime(); }

  /**
   * Since svtkMultiVolume acts like a proxy volume to compute the bounding box
   * for its internal svtkVolume instances, there are no properties to be set directly
   * in this instance. For that reason, this override ignores the svtkVolumeProperty
   * check.
   */
  int RenderVolumetricGeometry(svtkViewport* vp) override;

  /**
   * Return the eight corners of the volume
   */
  double* GetDataGeometry() { return this->DataGeometry.data(); }

protected:
  svtkMultiVolume();
  ~svtkMultiVolume() override;

  /**
   * The transformation matrix of this svtkProp3D is not user-definable,
   * (only the registered svtkVolume instances define the total bounding-box).
   * For that reason this method does nothing.
   * \sa svtkProp3D
   */
  void ComputeMatrix() override {}

  /**
   * Returns the svtkVolume registered in port.
   */
  svtkVolume* FindVolume(int port);

  /**
   * Checks for changes in the registered svtkVolume instances which could
   * required the bounding-box to be recomputed.
   */
  bool VolumesChanged();

  /**
   * For a box defined by bounds in coordinate system X, compute its
   * axis-aligned bounds in coordinate system Y. T defines the transformation
   * from X to Y and bounds ([x_min, x_max, y_min, y_max, z_min, z_max])
   * the box in X.
   */
  std::array<double, 6> ComputeAABounds(double bounds[6], svtkMatrix4x4* T) const;

  std::array<double, 6> DataBounds;
  std::array<double, 24> DataGeometry;
  std::unordered_map<int, svtkVolume*> Volumes;
  svtkTimeStamp BoundsComputeTime;
  svtkSmartPointer<svtkMatrix4x4> TexToBBox;

private:
  svtkMultiVolume(const svtkMultiVolume&) = delete;
  void operator=(const svtkMultiVolume&) = delete;
};
#endif
