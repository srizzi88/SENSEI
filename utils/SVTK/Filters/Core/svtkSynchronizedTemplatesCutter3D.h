/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSynchronizedTemplatesCutter3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSynchronizedTemplatesCutter3D
 * @brief   generate cut surface from structured points
 *
 *
 * svtkSynchronizedTemplatesCutter3D is an implementation of the synchronized
 * template algorithm. Note that svtkCutFilter will automatically
 * use this class when appropriate.
 *
 * @sa
 * svtkContourFilter svtkSynchronizedTemplates3D
 */

#ifndef svtkSynchronizedTemplatesCutter3D_h
#define svtkSynchronizedTemplatesCutter3D_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkSynchronizedTemplates3D.h"

class svtkImplicitFunction;

class SVTKFILTERSCORE_EXPORT svtkSynchronizedTemplatesCutter3D : public svtkSynchronizedTemplates3D
{
public:
  static svtkSynchronizedTemplatesCutter3D* New();

  svtkTypeMacro(svtkSynchronizedTemplatesCutter3D, svtkSynchronizedTemplates3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Needed by templated functions.
   */
  void ThreadedExecute(svtkImageData* data, svtkInformation* outInfo, int);

  //@{
  /**
   * Specify the implicit function to perform the cutting.
   */
  virtual void SetCutFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(CutFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkSynchronizedTemplatesCutter3D();
  ~svtkSynchronizedTemplatesCutter3D() override;

  svtkImplicitFunction* CutFunction;
  int OutputPointsPrecision;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkSynchronizedTemplatesCutter3D(const svtkSynchronizedTemplatesCutter3D&) = delete;
  void operator=(const svtkSynchronizedTemplatesCutter3D&) = delete;
};

#endif
