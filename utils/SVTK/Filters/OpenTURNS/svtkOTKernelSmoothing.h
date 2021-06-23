/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTKernelSmoothing.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOTKernelSmoothing
 * @brief
 * A SVTK Filter to compute Kernel Smoothing
 * using PDF computation from openturns.
 */

#ifndef svtkOTKernelSmoothing_h
#define svtkOTKernelSmoothing_h

#include "svtkFiltersOpenTURNSModule.h" // For export macro
#include "svtkOTFilter.h"

namespace OT
{
class KernelSmoothing;
}

class SVTKFILTERSOPENTURNS_EXPORT svtkOTKernelSmoothing : public svtkOTFilter
{
public:
  static svtkOTKernelSmoothing* New();
  svtkTypeMacro(svtkOTKernelSmoothing, svtkOTFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Methods to set / get number of points to compute, 129 by default
   */
  svtkSetClampMacro(PointNumber, int, 1, SVTK_INT_MAX);
  svtkGetMacro(PointNumber, int);
  //@}

  //@{
  /**
   * Methods to set / get flag that triggers
   * Gaussian PDF computation, true by default
   */
  svtkSetMacro(GaussianPDF, bool);
  svtkGetMacro(GaussianPDF, bool);
  //@}

  //@{
  /**
   * Methods to set / get flag that triggers
   * Triangular PDF computation, true by default
   */
  svtkSetMacro(TriangularPDF, bool);
  svtkGetMacro(TriangularPDF, bool);
  //@}

  //@{
  /**
   * Methods to set / get flag that triggers
   * Epanechnikov PDF computation, true by default
   */
  svtkSetMacro(EpanechnikovPDF, bool);
  svtkGetMacro(EpanechnikovPDF, bool);
  //@}

  //@{
  /**
   * Methods to set / get the boundary correction, false by default
   */
  svtkSetMacro(BoundaryCorrection, bool);
  svtkGetMacro(BoundaryCorrection, bool);
  //@}

protected:
  svtkOTKernelSmoothing();
  ~svtkOTKernelSmoothing() override;

  /**
   * Do the actual computation and store it in output
   */
  virtual int Process(OT::Sample* input) override;

  void ComputePDF(OT::Sample* input, OT::KernelSmoothing* ks, double* range, const char* pdfName);

  int PointNumber;
  bool GaussianPDF;
  bool TriangularPDF;
  bool EpanechnikovPDF;
  bool BoundaryCorrection;

private:
  void operator=(const svtkOTKernelSmoothing&) = delete;
  svtkOTKernelSmoothing(const svtkOTKernelSmoothing&) = delete;
};

#endif
