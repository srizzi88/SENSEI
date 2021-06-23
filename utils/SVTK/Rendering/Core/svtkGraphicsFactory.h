/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphicsFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphicsFactory
 *
 */

#ifndef svtkGraphicsFactory_h
#define svtkGraphicsFactory_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkGraphicsFactory : public svtkObject
{
public:
  static svtkGraphicsFactory* New();
  svtkTypeMacro(svtkGraphicsFactory, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create and return an instance of the named svtk object.
   * This method first checks the svtkObjectFactory to support
   * dynamic loading.
   */
  SVTK_NEWINSTANCE
  static svtkObject* CreateInstance(const char* svtkclassname);

  /**
   * What rendering library has the user requested
   */
  static const char* GetRenderLibrary();

  //@{
  /**
   * This option enables the creation of Mesa classes
   * instead of the OpenGL classes when using mangled Mesa.
   */
  static void SetUseMesaClasses(int use);
  static int GetUseMesaClasses();
  //@}

  //@{
  /**
   * This option enables the off-screen only mode. In this mode no X calls will
   * be made even when interactor is used.
   */
  static void SetOffScreenOnlyMode(int use);
  static int GetOffScreenOnlyMode();
  //@}

protected:
  svtkGraphicsFactory() {}

  static int UseMesaClasses;
  static int OffScreenOnlyMode;

private:
  svtkGraphicsFactory(const svtkGraphicsFactory&) = delete;
  void operator=(const svtkGraphicsFactory&) = delete;
};

#endif
