/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkCocoaMacOSXSDKCompatibility.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCocoaMacOSXSDKCompatibility
 * @brief   Compatibility header
 *
 * SVTK requires the Mac OS X 10.7 SDK or later.
 * However, this file is meant to allow us to use features from newer
 * SDKs by adding workarounds to still support the minimum SDK.
 * It is safe to include this header multiple times.
 */
#ifndef __SVTK_WRAP__

#include <AvailabilityMacros.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
#error SVTK requires the Mac OS X 10.7 SDK or later
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1070
#error SVTK requires a deployment target of Mac OS X 10.7 or later
#endif

#if (MAC_OS_X_VERSION_MAX_ALLOWED < 101200) && !defined(SVTK_DONT_MAP_10_12_ENUMS)
// The 10.12 SDK made a bunch of enum names more logical, map old names to new names to continue
// supporting old SDKs.
#define NSWindowStyleMask NSUInteger
#define NSWindowStyleMaskBorderless NSBorderlessWindowMask
#define NSWindowStyleMaskTitled NSTitledWindowMask
#define NSWindowStyleMaskClosable NSClosableWindowMask
#define NSWindowStyleMaskMiniaturizable NSMiniaturizableWindowMask
#define NSWindowStyleMaskResizable NSResizableWindowMask

#define NSEventModifierFlagShift NSShiftKeyMask
#define NSEventModifierFlagControl NSControlKeyMask
#define NSEventModifierFlagOption NSAlternateKeyMask
#define NSEventModifierFlagCommand NSCommandKeyMask

#define NSEventTypeKeyDown NSKeyDown
#define NSEventTypeKeyUp NSKeyUp
#define NSEventTypeApplicationDefined NSApplicationDefined
#define NSEventTypeFlagsChanged NSFlagsChanged

#define NSEventMaskAny NSAnyEventMask
#endif

// Create handy #defines that indicate the Objective-C memory management model.
// Manual Retain Release, Automatic Reference Counting, or Garbage Collection.
#if defined(__OBJC_GC__)
#define SVTK_OBJC_IS_MRR 0
#define SVTK_OBJC_IS_ARC 0
#define SVTK_OBJC_IS_GC 1
#elif __has_feature(objc_arc)
#define SVTK_OBJC_IS_MRR 0
#define SVTK_OBJC_IS_ARC 1
#define SVTK_OBJC_IS_GC 0
#else
#define SVTK_OBJC_IS_MRR 1
#define SVTK_OBJC_IS_ARC 0
#define SVTK_OBJC_IS_GC 0
#endif

#if __has_feature(objc_arc)
#error SVTK does not yet support ARC memory management
#endif

#endif
// SVTK-HeaderTest-Exclude: svtkCocoaMacOSXSDKCompatibility.h
