/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreadedSynchronizedTemplatesCutter3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkThreadedSynchronizedTemplatesCutter3D
 * @brief   generate cut surface from structured points
 *
 *
 * svtkThreadedSynchronizedTemplatesCutter3D is an implementation of the
 * synchronized template algorithm.
 *
 * @sa
 * svtkContourFilter svtkSynchronizedTemplates3D svtkThreadedSynchronizedTemplates3D
 * svtkSynchronizedTemplatesCutter3D
 */

#ifndef svtkThreadedSynchronizedTemplatesCutter3D_h
#define svtkThreadedSynchronizedTemplatesCutter3D_h

#include "svtkFiltersSMPModule.h" // For export macro
#include "svtkThreadedSynchronizedTemplates3D.h"

class svtkImplicitFunction;

#if !defined(SVTK_LEGACY_REMOVE)
class SVTKFILTERSSMP_EXPORT svtkThreadedSynchronizedTemplatesCutter3D
  : public svtkThreadedSynchronizedTemplates3D
{
public:
  static svtkThreadedSynchronizedTemplatesCutter3D* New();

  svtkTypeMacro(svtkThreadedSynchronizedTemplatesCutter3D, svtkThreadedSynchronizedTemplates3D);
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

  /**
   * Override GetMTime because we delegate to svtkContourValues and refer to
   * svtkImplicitFunction.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkThreadedSynchronizedTemplatesCutter3D();
  ~svtkThreadedSynchronizedTemplatesCutter3D() override;

  svtkImplicitFunction* CutFunction;
  int OutputPointsPrecision;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkThreadedSynchronizedTemplatesCutter3D(
    const svtkThreadedSynchronizedTemplatesCutter3D&) = delete;
  void operator=(const svtkThreadedSynchronizedTemplatesCutter3D&) = delete;
};

#endif // SVTK_LEGACY_REMOVE
#endif
