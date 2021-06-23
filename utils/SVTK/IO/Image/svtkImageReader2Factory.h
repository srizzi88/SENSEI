/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReader2Factory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageReader2Factory
 * @brief   Superclass of binary file readers.
 *
 * svtkImageReader2Factory: This class is used to create a svtkImageReader2
 * object given a path name to a file.  It calls CanReadFile on all
 * available readers until one of them returns true.  The available reader
 * list comes from three places.  In the InitializeReaders function of this
 * class, built-in SVTK classes are added to the list, users can call
 * RegisterReader, or users can create a svtkObjectFactory that has
 * CreateObject method that returns a new svtkImageReader2 sub class when
 * given the string "svtkImageReaderObject".  This way applications can be
 * extended with new readers via a plugin dll or by calling RegisterReader.
 * Of course all of the readers that are part of the svtk release are made
 * automatically available.
 *
 * @sa
 * svtkImageReader2
 */

#ifndef svtkImageReader2Factory_h
#define svtkImageReader2Factory_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkObject.h"

class svtkImageReader2;
class svtkImageReader2Collection;
class svtkImageReader2FactoryCleanup;

class SVTKIOIMAGE_EXPORT svtkImageReader2Factory : public svtkObject
{
public:
  static svtkImageReader2Factory* New();
  svtkTypeMacro(svtkImageReader2Factory, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * registered readers will be queried in CreateImageReader2 to
   * see if they can load a given file.
   */
  static void RegisterReader(svtkImageReader2* r);

  /**
   * open the image file, it is the callers responsibility to call
   * Delete on the returned object.   If no reader is found, null
   * is returned.
   */
  SVTK_NEWINSTANCE
  static svtkImageReader2* CreateImageReader2(const char* path);

  /**
   * The caller must allocate the svtkImageReader2Collection and pass in the
   * pointer to this method.
   */
  static void GetRegisteredReaders(svtkImageReader2Collection*);

protected:
  svtkImageReader2Factory();
  ~svtkImageReader2Factory() override;

  static void InitializeReaders();

private:
  static svtkImageReader2Collection* AvailableReaders;
  svtkImageReader2Factory(const svtkImageReader2Factory&) = delete;
  void operator=(const svtkImageReader2Factory&) = delete;

  friend class svtkImageReader2FactoryCleanup;
};

#endif
