/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPerturbCoincidentVertices.h

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
 * @class   svtkPerturbCoincidentVertices
 * @brief   Perturbs vertices that are coincident.
 *
 *
 * This filter perturbs vertices in a graph that have coincident coordinates.
 * In particular this happens all the time with graphs that are georeferenced,
 * so we need a nice scheme to perturb the vertices so that when the user
 * zooms in the vertices can be distiquished.
 */

#ifndef svtkPerturbCoincidentVertices_h
#define svtkPerturbCoincidentVertices_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkSmartPointer.h"        // for ivars

class svtkCoincidentPoints;
class svtkDataSet;

class SVTKINFOVISLAYOUT_EXPORT svtkPerturbCoincidentVertices : public svtkGraphAlgorithm
{
public:
  static svtkPerturbCoincidentVertices* New();
  svtkTypeMacro(svtkPerturbCoincidentVertices, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the perturbation factor (defaults to 1.0)
   */
  svtkSetMacro(PerturbFactor, double);
  svtkGetMacro(PerturbFactor, double);
  //@}

protected:
  svtkPerturbCoincidentVertices();
  ~svtkPerturbCoincidentVertices() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  // This class might have more than one method of coincident resolution
  void SpiralPerturbation(svtkGraph* input, svtkGraph* output);
  void SimpleSpiralPerturbation(svtkGraph* input, svtkGraph* output, float perturbFactor);

  float PerturbFactor;

  svtkPerturbCoincidentVertices(const svtkPerturbCoincidentVertices&) = delete;
  void operator=(const svtkPerturbCoincidentVertices&) = delete;
};

#endif
