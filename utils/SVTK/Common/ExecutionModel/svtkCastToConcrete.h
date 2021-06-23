/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCastToConcrete.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCastToConcrete
 * @brief   works around type-checking limitations
 *
 * svtkCastToConcrete is a filter that works around type-checking limitations
 * in the filter classes. Some filters generate abstract types on output,
 * and cannot be connected to the input of filters requiring a concrete
 * input type. For example, svtkElevationFilter generates svtkDataSet for output,
 * and cannot be connected to svtkDecimate, because svtkDecimate requires
 * svtkPolyData as input. This is true even though (in this example) the input
 * to svtkElevationFilter is of type svtkPolyData, and you know the output of
 * svtkElevationFilter is the same type as its input.
 *
 * svtkCastToConcrete performs run-time checking to insure that output type
 * is of the right type. An error message will result if you try to cast
 * an input type improperly. Otherwise, the filter performs the appropriate
 * cast and returns the data.
 *
 * @warning
 * You must specify the input before you can get the output. Otherwise an
 * error results.
 *
 * @sa
 * svtkDataSetAlgorithm svtkPointSetToPointSetFilter
 */

#ifndef svtkCastToConcrete_h
#define svtkCastToConcrete_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkDataSetAlgorithm.h"

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkCastToConcrete : public svtkDataSetAlgorithm
{

public:
  static svtkCastToConcrete* New();
  svtkTypeMacro(svtkCastToConcrete, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkCastToConcrete() {}
  ~svtkCastToConcrete() override {}

  int RequestData(svtkInformation*, svtkInformationVector**,
    svtkInformationVector*) override; // insures compatibility; satisfies abstract api in svtkFilter
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkCastToConcrete(const svtkCastToConcrete&) = delete;
  void operator=(const svtkCastToConcrete&) = delete;
};

#endif
