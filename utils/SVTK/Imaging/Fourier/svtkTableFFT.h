// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   svtkTableFFT
 * @brief   FFT for table columns
 *
 *
 *
 * svtkTableFFT performs the Fast Fourier Transform on the columns of a table.
 * Internally, it shoves each column into an image data and then uses
 * svtkImageFFT to perform the actual FFT.
 *
 *
 * @sa
 * svtkImageFFT
 *
 */

#ifndef svtkTableFFT_h
#define svtkTableFFT_h

#include "svtkImagingFourierModule.h" // For export macro
#include "svtkSmartPointer.h"         // For internal method.
#include "svtkTableAlgorithm.h"

class SVTKIMAGINGFOURIER_EXPORT svtkTableFFT : public svtkTableAlgorithm
{
public:
  svtkTypeMacro(svtkTableFFT, svtkTableAlgorithm);
  static svtkTableFFT* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTableFFT();
  ~svtkTableFFT() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Perform the FFT on the given data array.
   */
  virtual svtkSmartPointer<svtkDataArray> DoFFT(svtkDataArray* input);

private:
  svtkTableFFT(const svtkTableFFT&) = delete;
  void operator=(const svtkTableFFT&) = delete;
};

#endif // svtkTableFFT_h
