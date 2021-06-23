/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetGradientPrecompute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   svtkDataSetGradientPrecompute
 *
 *
 * Computes a geometry based vector field that the DataSetGradient filter uses to accelerate
 * gradient computation. This vector field is added to FieldData since it has a different
 * value for each vertex of each cell (a vertex shared by two cell has two different values).
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm
 * contributed by CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
 * F-91297 Arpajon, France. <br> Implementation by Thierry Carrard (CEA)
 */

#ifndef svtkDataSetGradientPrecompute_h
#define svtkDataSetGradientPrecompute_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkDataSetGradientPrecompute : public svtkDataSetAlgorithm
{
public:
  static svtkDataSetGradientPrecompute* New();
  svtkTypeMacro(svtkDataSetGradientPrecompute, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static int GradientPrecompute(svtkDataSet* ds);

protected:
  svtkDataSetGradientPrecompute();
  ~svtkDataSetGradientPrecompute() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkDataSetGradientPrecompute(const svtkDataSetGradientPrecompute&) = delete;
  void operator=(const svtkDataSetGradientPrecompute&) = delete;
};

#endif /* SVTK_DATA_SET_GRADIENT_PRECOMPUTE_H */
