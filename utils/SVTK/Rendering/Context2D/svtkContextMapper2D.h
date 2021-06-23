/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextMapper2D
 * @brief   Abstract class for 2D context mappers.
 *
 *
 *
 * This class provides an abstract base for 2D context mappers. They currently
 * only accept svtkTable objects as input.
 */

#ifndef svtkContextMapper2D_h
#define svtkContextMapper2D_h

#include "svtkAlgorithm.h"
#include "svtkRenderingContext2DModule.h" // For export macro

class svtkContext2D;
class svtkTable;
class svtkDataArray;
class svtkAbstractArray;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextMapper2D : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkContextMapper2D, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkContextMapper2D* New();

  //@{
  /**
   * Set/Get the input for this object - only accepts svtkTable as input.
   */
  virtual void SetInputData(svtkTable* input);
  virtual svtkTable* GetInput();
  //@}

  /**
   * Make the arrays accessible to the plot objects.
   */
  svtkDataArray* GetInputArrayToProcess(int idx, svtkDataObject* input)
  {
    return this->svtkAlgorithm::GetInputArrayToProcess(idx, input);
  }

  svtkAbstractArray* GetInputAbstractArrayToProcess(int idx, svtkDataObject* input)
  {
    return this->svtkAlgorithm::GetInputAbstractArrayToProcess(idx, input);
  }

protected:
  svtkContextMapper2D();
  ~svtkContextMapper2D() override;

  /**
   * Specify the types of input we can handle.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkContextMapper2D(const svtkContextMapper2D&) = delete;
  void operator=(const svtkContextMapper2D&) = delete;
};

#endif // svtkContextMapper2D_h
