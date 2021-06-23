/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkTextCodecFactory.h

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
 * @class   svtkTextCodecFactory
 * @brief   maintain a list of text codecs and return instances
 *
 *
 * A single class to hold registered codecs and return instances of them based
 * on either a decriptive name (UTF16 or latin-1) or by asking who can handle a
 * given std::vector<unsigned char>
 *
 * @par Thanks:
 * Thanks to Tim Shed from Sandia National Laboratories for his work
 * on the concepts and to Marcus Hanwell and Jeff Baumes of Kitware for
 * keeping me out of the weeds
 *
 * @sa
 * svtkTextCodec
 *
 */

#ifndef svtkTextCodecFactory_h
#define svtkTextCodecFactory_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkObject.h"

class svtkTextCodec;

class SVTKIOCORE_EXPORT svtkTextCodecFactory : public svtkObject
{
public:
  svtkTypeMacro(svtkTextCodecFactory, svtkObject);
  static svtkTextCodecFactory* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Type for Creation callback.
   */
  typedef svtkTextCodec* (*CreateFunction)();

  //@{
  /**
   * Provides mechanism to register/unregister additional callbacks to create
   * concrete subclasses of svtkTextCodecFactory to handle different protocols.
   * The registered callbacks are tried in the order they are registered.
   */
  static void RegisterCreateCallback(CreateFunction callback);
  static void UnRegisterCreateCallback(CreateFunction callback);
  static void UnRegisterAllCreateCallbacks();
  //@}

  /**
   * Given a codec/storage name try to find one of our registered codecs that
   * can handle it.  This is non-deterministic, very messy and should not be
   * your first thing to try.
   * The registered callbacks are tried in the order they are registered.
   */
  static svtkTextCodec* CodecForName(const char* CodecName);

  /**
   * Given a snippet of the stored data name try to find one of our registered
   * codecs that can handle transforming it into unicode.
   * The registered callbacks are tried in the order they are registered.
   */
  static svtkTextCodec* CodecToHandle(istream& InputStream);

  /**
   * Initialize core text codecs - needed for the static compilation case.
   */
  static void Initialize();

protected:
  svtkTextCodecFactory();
  ~svtkTextCodecFactory() override;

private:
  svtkTextCodecFactory(const svtkTextCodecFactory&) = delete;
  void operator=(const svtkTextCodecFactory&) = delete;

  //@{
  /**
   * Data structure used to store registered callbacks.
   */
  class CallbackVector;
  static CallbackVector* Callbacks;
  //@}
};

#endif // svtkTextCodecFactory_h
