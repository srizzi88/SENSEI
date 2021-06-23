/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCutMaterial.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCutMaterial
 * @brief   Automatically computes the cut plane for a material array pair.
 *
 * svtkCutMaterial computes a cut plane based on an up vector, center of the bounding box
 * and the location of the maximum variable value.
 *  These computed values are available so that they can be used to set the camera
 * for the best view of the plane.
 */

#ifndef svtkCutMaterial_h
#define svtkCutMaterial_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPlane;

class SVTKFILTERSPARALLEL_EXPORT svtkCutMaterial : public svtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkCutMaterial, svtkPolyDataAlgorithm);
  static svtkCutMaterial* New();

  //@{
  /**
   * Cell array that contains the material values.
   */
  svtkSetStringMacro(MaterialArrayName);
  svtkGetStringMacro(MaterialArrayName);
  //@}

  //@{
  /**
   * Material to probe.
   */
  svtkSetMacro(Material, int);
  svtkGetMacro(Material, int);
  //@}

  //@{
  /**
   * For now, we just use the cell values.
   * The array name to cut.
   */
  svtkSetStringMacro(ArrayName);
  svtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * The last piece of information that specifies the plane.
   */
  svtkSetVector3Macro(UpVector, double);
  svtkGetVector3Macro(UpVector, double);
  //@}

  //@{
  /**
   * Accesses to the values computed during the execute method.  They
   * could be used to get a good camera view for the resulting plane.
   */
  svtkGetVector3Macro(MaximumPoint, double);
  svtkGetVector3Macro(CenterPoint, double);
  svtkGetVector3Macro(Normal, double);
  //@}

protected:
  svtkCutMaterial();
  ~svtkCutMaterial() override;

  int RequestData(svtkInformation*, svtkInformationVector**,
    svtkInformationVector*) override; // generate output data
  int FillInputPortInformation(int port, svtkInformation* info) override;
  void ComputeMaximumPoint(svtkDataSet* input);
  void ComputeNormal();

  char* MaterialArrayName;
  int Material;
  char* ArrayName;
  double UpVector[3];
  double MaximumPoint[3];
  double CenterPoint[3];
  double Normal[3];

  svtkPlane* PlaneFunction;

private:
  svtkCutMaterial(const svtkCutMaterial&) = delete;
  void operator=(const svtkCutMaterial&) = delete;
};

#endif
