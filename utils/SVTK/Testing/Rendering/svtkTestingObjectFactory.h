/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTestingObjectFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/
#ifndef svtkTestingObjectFactory_h
#define svtkTestingObjectFactory_h

/**
 * @class   svtkTestingObjectFactory
 * @brief   Object overrides used during testing
 *
 * Some svtk examples and tests need to perform differently when they
 * are run as tests versus when they are run as individual
 * programs. Many tests/examples are interactive and eventually call
 * svtkRenderWindowInteration::Start() to initialie the
 * interaction. But, when run as tests, these programs should
 * exit. This factory overrides svtkRenderWindowInteractor so that the
 * Start() method just returns.
 * To use this factory:
 * \code
 */

#include "svtkTestingRenderingModule.h" // For export macro
//   #include "svtkTestingObjectFactory.h"
//   svtkTestingObjectFactory* factory = svtkTestingObjectFactory::New();
//   svtkObjectFactory::RegisterFactory(factory);
// \endcode

#include "svtkObjectFactory.h"

#include "svtkSmartPointer.h"      // Required for testing framework
#include "svtkTestDriver.h"        // Required for testing framework
#include "svtkTesting.h"           // Required for testing framework
#include "svtkTestingInteractor.h" // Required for testing framework

class SVTKTESTINGRENDERING_EXPORT svtkTestingObjectFactory : public svtkObjectFactory
{
public:
  static svtkTestingObjectFactory* New();
  svtkTypeMacro(svtkTestingObjectFactory, svtkObjectFactory);
  const char* GetSVTKSourceVersion() override;
  const char* GetDescription() override { return "Factory for overrides during testing"; }
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  /**
   * Register objects that override svtk objects whem they are run as tests.
   */
  svtkTestingObjectFactory();

private:
  svtkTestingObjectFactory(const svtkTestingObjectFactory&) = delete;
  void operator=(const svtkTestingObjectFactory&) = delete;
};
#endif
