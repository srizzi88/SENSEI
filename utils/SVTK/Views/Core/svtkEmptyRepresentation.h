/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEmptyRepresentation.h

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
 * @class   svtkEmptyRepresentation
 *
 *
 */

#ifndef svtkEmptyRepresentation_h
#define svtkEmptyRepresentation_h

#include "svtkDataRepresentation.h"
#include "svtkSmartPointer.h"    // For SP ivars
#include "svtkViewsCoreModule.h" // For export macro

class svtkConvertSelectionDomain;

class SVTKVIEWSCORE_EXPORT svtkEmptyRepresentation : public svtkDataRepresentation
{
public:
  static svtkEmptyRepresentation* New();
  svtkTypeMacro(svtkEmptyRepresentation, svtkDataRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Since this representation has no inputs, override superclass
   * implementation with one that ignores "port" and "conn" and still allows it
   * to have an annotation output.
   */
  svtkAlgorithmOutput* GetInternalAnnotationOutputPort() override
  {
    return this->GetInternalAnnotationOutputPort(0);
  }
  svtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port) override
  {
    return this->GetInternalAnnotationOutputPort(port, 0);
  }
  svtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port, int conn) override;

protected:
  svtkEmptyRepresentation();
  ~svtkEmptyRepresentation() override;

private:
  svtkEmptyRepresentation(const svtkEmptyRepresentation&) = delete;
  void operator=(const svtkEmptyRepresentation&) = delete;

  svtkSmartPointer<svtkConvertSelectionDomain> ConvertDomains;
};

#endif
