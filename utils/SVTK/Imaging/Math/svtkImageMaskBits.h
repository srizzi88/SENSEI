/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMaskBits.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMaskBits
 * @brief   applies a bit-mask pattern to each component.
 *
 *
 * svtkImageMaskBits applies a bit-mask pattern to each component.  The
 * bit-mask can be applied using a variety of boolean bitwise operators.
 */

#ifndef svtkImageMaskBits_h
#define svtkImageMaskBits_h

#include "svtkImageLogic.h"        //For SVTK_AND, SVTK_OR ...
#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMATH_EXPORT svtkImageMaskBits : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageMaskBits* New();
  svtkTypeMacro(svtkImageMaskBits, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the bit-masks. Default is 0xffffffff.
   */
  svtkSetVector4Macro(Masks, unsigned int);
  void SetMask(unsigned int mask) { this->SetMasks(mask, mask, mask, mask); }
  void SetMasks(unsigned int mask1, unsigned int mask2)
  {
    this->SetMasks(mask1, mask2, 0xffffffff, 0xffffffff);
  }
  void SetMasks(unsigned int mask1, unsigned int mask2, unsigned int mask3)
  {
    this->SetMasks(mask1, mask2, mask3, 0xffffffff);
  }
  svtkGetVector4Macro(Masks, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the boolean operator. Default is AND.
   */
  svtkSetMacro(Operation, int);
  svtkGetMacro(Operation, int);
  void SetOperationToAnd() { this->SetOperation(SVTK_AND); }
  void SetOperationToOr() { this->SetOperation(SVTK_OR); }
  void SetOperationToXor() { this->SetOperation(SVTK_XOR); }
  void SetOperationToNand() { this->SetOperation(SVTK_NAND); }
  void SetOperationToNor() { this->SetOperation(SVTK_NOR); }
  //@}

protected:
  svtkImageMaskBits();
  ~svtkImageMaskBits() override {}

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

  unsigned int Masks[4];
  int Operation;

private:
  svtkImageMaskBits(const svtkImageMaskBits&) = delete;
  void operator=(const svtkImageMaskBits&) = delete;
};

#endif
