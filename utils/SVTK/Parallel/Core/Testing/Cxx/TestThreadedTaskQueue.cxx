/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreadedTaskQueue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkThreadedTaskQueue.h"

#include "svtkLogger.h"
#include <chrono>
#include <thread>
int TestThreadedTaskQueue(int, char*[])
{
  svtkThreadedTaskQueue<double, int, double> queue(
    [](int i, double v) {
      svtkLogF(INFO, "hey: %d, %f", i, v);
      return i * v;
    },
    false, 2, 1);
  queue.Push(1, 1.0);
  queue.Push(2, 2.0);
  queue.Push(3, 3.0);

  while (!queue.IsEmpty())
  {
    double r;
    if (queue.Pop(r))
    {
      svtkLogF(INFO, "result: %f", r);
    }
    else
    {
      svtkLogF(ERROR, "failed to pop!");
    }
  }

  svtkThreadedTaskQueue<void, int> queue2([](int id) -> void { svtkLogF(INFO, "hi: %d", id); });
  queue2.Push(0);
  queue2.Push(1);
  queue2.Push(2);
  queue2.Flush();
  return EXIT_SUCCESS;
}
