/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianCubeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGaussianCubeReader
 * @brief   read ASCII Gaussian Cube Data files
 *
 * svtkGaussianCubeReader is a source object that reads ASCII files following
 * the description in http://www.gaussian.com/00000430.htm
 * The FileName must be specified.
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class.
 */

#ifndef svtkGaussianCubeReader_h
#define svtkGaussianCubeReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkMoleculeReaderBase.h"

class svtkImageData;
class svtkTransform;

class SVTKIOGEOMETRY_EXPORT svtkGaussianCubeReader : public svtkMoleculeReaderBase
{
public:
  static svtkGaussianCubeReader* New();
  svtkTypeMacro(svtkGaussianCubeReader, svtkMoleculeReaderBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkGetObjectMacro(Transform, svtkTransform);
  svtkImageData* GetGridOutput();

protected:
  svtkGaussianCubeReader();
  ~svtkGaussianCubeReader() override;

  svtkTransform* Transform;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ReadSpecificMolecule(FILE* fp) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkGaussianCubeReader(const svtkGaussianCubeReader&) = delete;
  void operator=(const svtkGaussianCubeReader&) = delete;
};

#endif
