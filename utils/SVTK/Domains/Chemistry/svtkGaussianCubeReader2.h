/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianCubeReader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGaussianCubeReader2
 * @brief   Read a Gaussian Cube file and output a
 * svtkMolecule object and a svtkImageData
 *
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class.
 */

#ifndef svtkGaussianCubeReader2_h
#define svtkGaussianCubeReader2_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"

class svtkMolecule;
class svtkImageData;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkGaussianCubeReader2 : public svtkMoleculeAlgorithm
{
public:
  static svtkGaussianCubeReader2* New();
  svtkTypeMacro(svtkGaussianCubeReader2, svtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the output (svtkMolecule) that the reader will fill
   */
  svtkMolecule* GetOutput();
  void SetOutput(svtkMolecule*) override;
  //@}

  /**
   * Get/Set the output (svtkImageData) that the reader will fill
   */
  svtkImageData* GetGridOutput();

  //@{
  /**
   * Get/Set the name of the CML file
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkGaussianCubeReader2();
  ~svtkGaussianCubeReader2() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  char* FileName;

private:
  svtkGaussianCubeReader2(const svtkGaussianCubeReader2&) = delete;
  void operator=(const svtkGaussianCubeReader2&) = delete;
};

#endif
