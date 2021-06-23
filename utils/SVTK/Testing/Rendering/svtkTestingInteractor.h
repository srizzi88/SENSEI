/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTestingInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/
/**
 * @class   svtkTestingInteractor
 * @brief   A RenderWindowInteractor for testing
 *
 * Provides a Start() method that passes arguments to a test for
 * regression testing and returns. This permits programs that
 * run as tests to exit gracefully during the test run without needing
 * interaction.
 * @sa
 * svtkTestingObjectFactory
 */

#ifndef svtkTestingInteractor_h
#define svtkTestingInteractor_h

#include "svtkObjectFactoryCollection.h" // Generated object overrides
#include "svtkRenderWindowInteractor.h"
#include "svtkTestingRenderingModule.h" // For export macro

#include <string> // STL Header; Required for string

class SVTKTESTINGRENDERING_EXPORT svtkTestingInteractor : public svtkRenderWindowInteractor
{
public:
  /**
   * Standard object factory instantiation method.
   */
  static svtkTestingInteractor* New();

  //@{
  /**
   * Type and printing information.
   */
  svtkTypeMacro(svtkTestingInteractor, svtkRenderWindowInteractor);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  void Start() override;

  static int TestReturnStatus;      // Return status of the test
  static double ErrorThreshold;     // Error Threshold
  static std::string ValidBaseline; // Name of the Baseline image
  static std::string TempDirectory; // Location of Testing/Temporary
  static std::string DataDirectory; // Location of SVTKData

protected:
  svtkTestingInteractor() {}

private:
  svtkTestingInteractor(const svtkTestingInteractor&) = delete;
  void operator=(const svtkTestingInteractor&) = delete;
};

#endif
