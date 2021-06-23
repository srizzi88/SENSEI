/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConvertSelectionDomain.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkConvertSelectionDomain
 * @brief   Convert a selection from one domain to another
 *
 *
 * svtkConvertSelectionDomain converts a selection from one domain to another
 * using known domain mappings. The domain mappings are described by a
 * svtkMultiBlockDataSet containing one or more svtkTables.
 *
 * The first input port is for the input selection (or collection of annotations
 * in a svtkAnnotationLayers object), while the second port
 * is for the multi-block of mappings, and the third port is for the
 * data that is being selected on.
 *
 * If the second or third port is not set, this filter will pass the
 * selection/annotation to the output unchanged.
 *
 * The second output is the selection associated with the "current annotation"
 * normally representing the current interactive selection.
 */

#ifndef svtkConvertSelectionDomain_h
#define svtkConvertSelectionDomain_h

#include "svtkPassInputTypeAlgorithm.h"
#include "svtkViewsCoreModule.h" // For export macro

class svtkAnnotation;

class SVTKVIEWSCORE_EXPORT svtkConvertSelectionDomain : public svtkPassInputTypeAlgorithm
{
public:
  static svtkConvertSelectionDomain* New();
  svtkTypeMacro(svtkConvertSelectionDomain, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkConvertSelectionDomain();
  ~svtkConvertSelectionDomain() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkConvertSelectionDomain(const svtkConvertSelectionDomain&) = delete;
  void operator=(const svtkConvertSelectionDomain&) = delete;
};

#endif
