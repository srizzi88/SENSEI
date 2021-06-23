/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewUpdater.h

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
 * @class   svtkViewUpdater
 * @brief   Updates views automatically
 *
 *
 * svtkViewUpdater registers with annotation change events for a set of
 * annotation links, and updates all views when an annotation link fires an
 * annotation changed event. This is often needed when multiple views share
 * a selection with svtkAnnotationLink.
 */

#ifndef svtkViewUpdater_h
#define svtkViewUpdater_h

#include "svtkObject.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkAnnotationLink;
class svtkView;

class SVTKVIEWSINFOVIS_EXPORT svtkViewUpdater : public svtkObject
{
public:
  static svtkViewUpdater* New();
  svtkTypeMacro(svtkViewUpdater, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void AddView(svtkView* view);
  void RemoveView(svtkView* view);

  void AddAnnotationLink(svtkAnnotationLink* link);

protected:
  svtkViewUpdater();
  ~svtkViewUpdater() override;

private:
  svtkViewUpdater(const svtkViewUpdater&) = delete;
  void operator=(const svtkViewUpdater&) = delete;

  class svtkViewUpdaterInternals;
  svtkViewUpdaterInternals* Internals;
};

#endif
