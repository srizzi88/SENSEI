//
//  SVTKGestureHandler.m
//  SVTKViewer
//
//  Created by Max Smolens on 6/20/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import "SVTKGestureHandler.h"

#import "SVTKView.h"

#include "svtkIOSRenderWindowInteractor.h"

@interface SVTKGestureHandler ()

@property (weak, nonatomic) SVTKView* svtkView;

@property (strong, nonatomic) UITapGestureRecognizer* singleTapRecognizer;
@property (strong, nonatomic) UITapGestureRecognizer* doubleTapRecognizer;
@property (strong, nonatomic) UILongPressGestureRecognizer* longPressRecognizer;
@property (strong, nonatomic) UIPinchGestureRecognizer* pinchRecognizer;
@property (strong, nonatomic) UIRotationGestureRecognizer* rollRecognizer;
@property (strong, nonatomic) UIPanGestureRecognizer* trackballRecognizer;
@property (strong, nonatomic) UIPanGestureRecognizer* panRecognizer;
@property (assign) NSInteger lastNumberOfTouches;

@end

@implementation SVTKGestureHandler

- (id)initWithVtkView:(SVTKView*)svtkView
{
  if (self = [super init])
  {
    self.svtkView = svtkView;
    [self setupGestures];
  }
  return self;
}

- (void)setupGestures
{
  // Add the pinch gesture recognizer
  self.pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self
                                                                   action:@selector(onPinch:)];
  [self.svtkView addGestureRecognizer:self.pinchRecognizer];
  self.pinchRecognizer.delegate = self;

  // Add the 'roll' gesture recognizer
  self.rollRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self
                                                                     action:@selector(onRoll:)];
  [self.svtkView addGestureRecognizer:self.rollRecognizer];
  self.rollRecognizer.delegate = self;

  // Add the pan gesture regognizer (trackball if 1 finger)
  self.panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                               action:@selector(onPan:)];
  [self.panRecognizer setMinimumNumberOfTouches:1];
  [self.panRecognizer setMaximumNumberOfTouches:2];
  [self.svtkView addGestureRecognizer:self.panRecognizer];
  self.panRecognizer.delegate = self;

  self.lastNumberOfTouches = 0;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
  shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer
{
  return YES;
}

- (void)onPinch:(UIPinchGestureRecognizer*)sender
{
  if (sender.numberOfTouches == 1)
  {
    return;
  }

  // Send the gesture position to the interactor
  [self forwardTouchPositionToInteractor:sender];

  // Set the scale, clamping to prevent blowouts
  const CGFloat MaxScale = 3.0;
  const CGFloat MinScale = -3.0;
  CGFloat scale = [sender scale];
  scale = scale > MaxScale ? MaxScale : (scale < MinScale ? MinScale : scale);
  assert(scale <= MaxScale && scale >= MinScale);

  // Send the gesture scale to the interactor
  self.svtkView.interactor->SetScale(scale);

  // Invoke the right event
  switch ([sender state])
  {
    case UIGestureRecognizerStateBegan:
      self.svtkView.interactor->StartPinchEvent();
      break;
    case UIGestureRecognizerStateChanged:
      self.svtkView.interactor->PinchEvent();
      break;
    case UIGestureRecognizerStateEnded:
      self.svtkView.interactor->EndPinchEvent();
      break;
    default:
      break;
  }

  // Render
  [self.svtkView setNeedsDisplay];
}

- (void)onRoll:(UIRotationGestureRecognizer*)sender
{
  if (sender.numberOfTouches == 1)
  {
    return;
  }

  // Send the gesture position to the interactor
  [self forwardTouchPositionToInteractor:sender];

  // Set the rotation angle
  double rotationAngle = -[sender rotation] * 180.0 / M_PI;

  // Send the gesture rotation to the interactor
  self.svtkView.interactor->SetRotation(rotationAngle);

  // Invoke the right event
  switch ([sender state])
  {
    case UIGestureRecognizerStateBegan:
      self.svtkView.interactor->StartRotateEvent();
      break;
    case UIGestureRecognizerStateChanged:
      self.svtkView.interactor->RotateEvent();
      break;
    case UIGestureRecognizerStateEnded:
      self.svtkView.interactor->EndRotateEvent();
      break;
    default:
      break;
  }

  // Render
  [self.svtkView setNeedsDisplay];
}

- (void)onPan:(UIPanGestureRecognizer*)sender
{
  // Send the gesture position to the interactor
  [self forwardTouchPositionToInteractor:sender];

  // Two fingers means panning
  if (sender.numberOfTouches == 2 ||
    (self.lastNumberOfTouches == 2 && [sender state] == UIGestureRecognizerStateEnded))
  {
    // Set the translation vector
    CGPoint d = [sender translationInView:self.svtkView];
    double scaleFactor = self.svtkView.contentScaleFactor;
    double t[] = { scaleFactor * d.x, -scaleFactor * d.y };

    // Send the gesture translation to the interactor
    self.svtkView.interactor->SetTranslation(t);

    if (self.lastNumberOfTouches == 1)
    {
      // If switching from one finger, cancel one finger pan
      // and begin two finger pan
      [self onTrackballMotion:UIGestureRecognizerStateEnded];
      [self onTwoFingerPan:UIGestureRecognizerStateBegan];
    }
    else
    {
      // Invoke the right event
      [self onTwoFingerPan:[sender state]];
    }
    // One finger means trackball
  }
  else if (sender.numberOfTouches == 1 ||
    (self.lastNumberOfTouches == 1 && [sender state] == UIGestureRecognizerStateEnded))
  {
    if (self.lastNumberOfTouches == 2)
    {
      // If switching from two fingers, cancel two fingers pan,
      // roll and pinch, and start one finger pan
      [self onTrackballMotion:UIGestureRecognizerStateBegan];
      [self onTwoFingerPan:UIGestureRecognizerStateEnded];
      self.svtkView.interactor->EndPinchEvent();
      self.svtkView.interactor->EndRotateEvent();
    }
    else
    {
      // Invoke the right event
      [self onTrackballMotion:[sender state]];
    }
  }

  // Update number of touches
  self.lastNumberOfTouches = sender.numberOfTouches;

  // Render
  [self.svtkView setNeedsDisplay];
}

- (void)onTrackballMotion:(UIGestureRecognizerState)state
{
  switch (state)
  {
    case UIGestureRecognizerStateBegan:
      self.svtkView.interactor->InvokeEvent(svtkCommand::LeftButtonPressEvent, NULL);
      break;
    case UIGestureRecognizerStateChanged:
      self.svtkView.interactor->InvokeEvent(svtkCommand::MouseMoveEvent, NULL);
      break;
    case UIGestureRecognizerStateEnded:
      self.svtkView.interactor->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, NULL);
      break;
    default:
      break;
  }
}

- (void)onTwoFingerPan:(UIGestureRecognizerState)state
{
  switch (state)
  {
    case UIGestureRecognizerStateBegan:
      self.svtkView.interactor->StartPanEvent();
      break;
    case UIGestureRecognizerStateChanged:
      self.svtkView.interactor->PanEvent();
      break;
    case UIGestureRecognizerStateEnded:
      self.svtkView.interactor->EndPanEvent();
      break;
    default:
      break;
  }
}

- (void)forwardTouchPositionToInteractor:(UIGestureRecognizer*)sender
{
  // Position in screen
  CGPoint touchPoint = [sender locationInView:self.svtkView];

  // Translate to position in SVTK coordinates
  int position[2] = { 0 };
  [self.svtkView displayCoordinates:position ofTouch:touchPoint];

  // Forward position to SVTK interactor
  self.svtkView.interactor->SetEventInformation(position[0], position[1], 0, 0, 0, 0, 0, 0);
}

@end
