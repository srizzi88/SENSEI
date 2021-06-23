/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointLoad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointLoad
 * @brief   compute stress tensors given point load on semi-infinite domain
 *
 * svtkPointLoad is a source object that computes stress tensors on a volume.
 * The tensors are computed from the application of a point load on a
 * semi-infinite domain. (The analytical results are adapted from Saada - see
 * text.) It also is possible to compute effective stress scalars if desired.
 * This object serves as a specialized data generator for some of the examples
 * in the text.
 *
 * @sa
 * svtkTensorGlyph, svtkHyperStreamline
 */

#ifndef svtkPointLoad_h
#define svtkPointLoad_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingHybridModule.h" // For export macro

class SVTKIMAGINGHYBRID_EXPORT svtkPointLoad : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkPointLoad, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
   * and LoadValue = 1.
   */
  static svtkPointLoad* New();

  //@{
  /**
   * Set/Get value of applied load.
   */
  svtkSetMacro(LoadValue, double);
  svtkGetMacro(LoadValue, double);
  //@}

  /**
   * Specify the dimensions of the volume. A stress tensor will be computed for
   * each point in the volume.
   */
  void SetSampleDimensions(int i, int j, int k);

  //@{
  /**
   * Specify the dimensions of the volume. A stress tensor will be computed for
   * each point in the volume.
   */
  void SetSampleDimensions(int dim[3]);
  svtkGetVectorMacro(SampleDimensions, int, 3);
  //@}

  //@{
  /**
   * Specify the region in space over which the tensors are computed. The point
   * load is assumed to be applied at top center of the volume.
   */
  svtkSetVector6Macro(ModelBounds, double);
  svtkGetVectorMacro(ModelBounds, double, 6);
  //@}

  //@{
  /**
   * Set/Get Poisson's ratio.
   */
  svtkSetMacro(PoissonsRatio, double);
  svtkGetMacro(PoissonsRatio, double);
  //@}

  /**
   * Turn on/off computation of effective stress scalar. These methods do
   * nothing. The effective stress is always computed.
   */
  void SetComputeEffectiveStress(int) {}
  int GetComputeEffectiveStress() { return 1; }
  void ComputeEffectiveStressOn() {}
  void ComputeEffectiveStressOff() {}

protected:
  svtkPointLoad();
  ~svtkPointLoad() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ExecuteDataWithInformation(svtkDataObject*, svtkInformation*) override;

  double LoadValue;
  double PoissonsRatio;
  int SampleDimensions[3];
  double ModelBounds[6];

private:
  svtkPointLoad(const svtkPointLoad&) = delete;
  void operator=(const svtkPointLoad&) = delete;
};

#endif
