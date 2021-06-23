/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReflectionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkReflectionFilter
 * @brief   reflects a data set across a plane
 *
 * The svtkReflectionFilter reflects a data set across one of the
 * planes formed by the data set's bounding box.
 * Since it converts data sets into unstructured grids, it is not efficient
 * for structured data sets.
 */

#ifndef svtkReflectionFilter_h
#define svtkReflectionFilter_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkUnstructuredGrid;
class svtkDataSet;

class SVTKFILTERSGENERAL_EXPORT svtkReflectionFilter : public svtkDataObjectAlgorithm
{
public:
  static svtkReflectionFilter* New();

  svtkTypeMacro(svtkReflectionFilter, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum ReflectionPlane
  {
    USE_X_MIN = 0,
    USE_Y_MIN = 1,
    USE_Z_MIN = 2,
    USE_X_MAX = 3,
    USE_Y_MAX = 4,
    USE_Z_MAX = 5,
    USE_X = 6,
    USE_Y = 7,
    USE_Z = 8
  };

  //@{
  /**
   * Set the normal of the plane to use as mirror.
   */
  svtkSetClampMacro(Plane, int, 0, 8);
  svtkGetMacro(Plane, int);
  void SetPlaneToX() { this->SetPlane(USE_X); }
  void SetPlaneToY() { this->SetPlane(USE_Y); }
  void SetPlaneToZ() { this->SetPlane(USE_Z); }
  void SetPlaneToXMin() { this->SetPlane(USE_X_MIN); }
  void SetPlaneToYMin() { this->SetPlane(USE_Y_MIN); }
  void SetPlaneToZMin() { this->SetPlane(USE_Z_MIN); }
  void SetPlaneToXMax() { this->SetPlane(USE_X_MAX); }
  void SetPlaneToYMax() { this->SetPlane(USE_Y_MAX); }
  void SetPlaneToZMax() { this->SetPlane(USE_Z_MAX); }
  //@}

  //@{
  /**
   * If the reflection plane is set to X, Y or Z, this variable
   * is use to set the position of the plane.
   */
  svtkSetMacro(Center, double);
  svtkGetMacro(Center, double);
  //@}

  //@{
  /**
   * If on (the default), copy the input geometry to the output. If off,
   * the output will only contain the reflection.
   */
  svtkSetMacro(CopyInput, svtkTypeBool);
  svtkGetMacro(CopyInput, svtkTypeBool);
  svtkBooleanMacro(CopyInput, svtkTypeBool);
  //@}

  //@{
  /**
   * If off (the default), only Vectors, Normals and Tensors will be flipped.
   * If on, all 3-component data arrays ( considered as 3D vectors),
   * 6-component data arrays (considered as symmetric tensors),
   * 9-component data arrays (considered as tensors ) of signed type will be flipped.
   * All other won't be flipped and will only be copied.
   */
  svtkSetMacro(FlipAllInputArrays, bool);
  svtkGetMacro(FlipAllInputArrays, bool);
  svtkBooleanMacro(FlipAllInputArrays, bool);
  //@}

protected:
  svtkReflectionFilter();
  ~svtkReflectionFilter() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   * Overridden to create the correct type of output.
   */
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Actual implementation for reflection.
   */
  virtual int RequestDataInternal(svtkDataSet* input, svtkUnstructuredGrid* output, double bounds[6]);

  /**
   * Internal method to compute bounds.
   */
  virtual int ComputeBounds(svtkDataObject* input, double bounds[6]);

  /**
   * Generate new, non-3D cell and return the generated cells id.
   */
  virtual svtkIdType ReflectNon3DCell(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdType numInputPoints);

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void FlipTuple(double* tuple, int* mirrorDir, int nComp);

  int Plane;
  double Center;
  svtkTypeBool CopyInput;
  bool FlipAllInputArrays;

private:
  svtkReflectionFilter(const svtkReflectionFilter&) = delete;
  void operator=(const svtkReflectionFilter&) = delete;
};

#endif
