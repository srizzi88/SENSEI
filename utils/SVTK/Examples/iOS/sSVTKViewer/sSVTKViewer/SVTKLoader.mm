//
//  SVTKLoader.m
//  SVTKViewer
//
//  Created by Alexis Girault on 11/17/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import "SVTKLoader.h"

#include "svtkCubeSource.h"
#include "svtkDataSetMapper.h"
#include "svtkGenericDataObjectReader.h"
#include "svtkOBJReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSTLReader.h"
#include "svtkXMLGenericDataObjectReader.h"

@implementation SVTKLoader

+ (svtkSmartPointer<svtkActor>)loadFromURL:(NSURL*)url
{
  // Setup file path
  const char* cpath = [[url path] UTF8String];

  // Setup proper reader
  BOOL polydata = NO;
  svtkSmartPointer<svtkAlgorithm> source;
  if ([self fileAtURL:url matchesExtension:@[ @"svtk" ]])
  {
    auto reader = svtkSmartPointer<svtkGenericDataObjectReader>::New();
    reader->SetFileName(cpath);
    polydata = (reader->GetPolyDataOutput() != nullptr);
    source = reader;
  }
  else if ([self fileAtURL:url matchesExtension:@[ @"vti", @"vtp", @"vtr", @"vts", @"vtu" ]])
  {
    auto reader = svtkSmartPointer<svtkXMLGenericDataObjectReader>::New();
    reader->SetFileName(cpath);
    polydata = (reader->GetPolyDataOutput() != nullptr);
    source = reader;
  }
  else if ([self fileAtURL:url matchesExtension:@[ @"obj" ]])
  {
    auto reader = svtkSmartPointer<svtkOBJReader>::New();
    reader->SetFileName(cpath);
    polydata = YES;
    source = reader;
  }
  else if ([self fileAtURL:url matchesExtension:@[ @"stl" ]])
  {
    auto reader = svtkSmartPointer<svtkSTLReader>::New();
    reader->SetFileName(cpath);
    polydata = YES;
    source = reader;
  }
  else
  {
    NSLog(@"No reader found for extension: %@", [url pathExtension]);
    return nil;
  }

  // Check reader worked
  source->Update();
  if (source->GetErrorCode())
  {
    NSLog(@"Error loading file: %@", [url lastPathComponent]);
    return nil;
  }

  // Setup mapper
  svtkSmartPointer<svtkMapper> mapper;
  if (polydata)
  {
    mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  }
  else
  {
    mapper = svtkSmartPointer<svtkDataSetMapper>::New();
  }
  mapper->SetInputConnection(source->GetOutputPort());

  // Setup actor
  auto actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  return actor;
}

+ (BOOL)fileAtURL:(NSURL*)url matchesExtension:(NSArray<NSString*>*)validExtensions
{
  // Actual extension
  NSString* fileExt = [url pathExtension];

  // Check if one of the valid extensions
  for (NSString* validExt in validExtensions)
  {
    // Case insensitive comparison
    if ([fileExt caseInsensitiveCompare:validExt] == NSOrderedSame)
    {
      return YES;
    }
  }
  return NO;
}

@end
