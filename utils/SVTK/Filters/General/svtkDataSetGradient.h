/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   svtkDataSetGradient
 * @brief   computes scalar field gradient
 *
 *
 * svtkDataSetGradient Computes per cell gradient of point scalar field
 * or per point gradient of cell scalar field.
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm
 * contributed by CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
 * F-91297 Arpajon, France. <br> Implementation by Thierry Carrard (CEA)
 */

#ifndef svtkDataSetGradient_h
#define svtkDataSetGradient_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkDataSetGradient : public svtkDataSetAlgorithm
{
public:
  static svtkDataSetGradient* New();
  svtkTypeMacro(svtkDataSetGradient, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the name of computed vector array.
   */
  svtkSetStringMacro(ResultArrayName);
  svtkGetStringMacro(ResultArrayName);
  //@}

protected:
  svtkDataSetGradient();
  ~svtkDataSetGradient() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* ResultArrayName;

private:
  svtkDataSetGradient(const svtkDataSetGradient&) = delete;
  void operator=(const svtkDataSetGradient&) = delete;
};

#endif /* SVTK_DATA_SET_GRADIENT_H */
