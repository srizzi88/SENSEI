//
//  SVTKLoader.h
//  SVTKViewer
//
//  Created by Alexis Girault on 11/17/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "svtkActor.h"
#include "svtkSmartPointer.h"

@interface SVTKLoader : NSObject

+ (svtkSmartPointer<svtkActor>)loadFromURL:(NSURL*)url;

@end
