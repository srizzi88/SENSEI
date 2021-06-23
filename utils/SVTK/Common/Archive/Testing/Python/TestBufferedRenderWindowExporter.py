#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBufferedRenderWindowExporter.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import ctypes
import io
import json
from random import random, seed
import shutil
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetTempDir
import zipfile

SVTK_TEMP_DIR = svtkGetTempDir()

# Construct a render window and write it to disk and to buffer. Decompress the
# buffer and compare its contents to the files on disk.
class TestBufferedRenderWindowExporter(svtk.test.Testing.svtkTest):

    def testBufferedRenderWindowExporter(self):

        cone = svtk.svtkConeSource()

        coneMapper = svtk.svtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = svtk.svtkActor()
        coneActor.SetMapper(coneMapper)

        ren = svtk.svtkRenderer()
        ren.AddActor(coneActor)
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)

        ren.ResetCamera()
        renWin.Render()

        seed(0)
        archiveName = SVTK_TEMP_DIR + '/scene_' + str(random())

        exporter = svtk.svtkJSONRenderWindowExporter()
        exporter.GetArchiver().SetArchiveName(archiveName)
        exporter.SetRenderWindow(renWin)
        exporter.Write()

        bufferedExporter = svtk.svtkJSONRenderWindowExporter()
        bufferedArchiver = svtk.svtkBufferedArchiver()
        bufferedExporter.SetArchiver(bufferedArchiver)
        bufferedExporter.SetRenderWindow(renWin)
        bufferedExporter.Write()

        ptr = bufferedArchiver.GetBufferAddress()
        address = int(ptr[1:-7], 16)
        ArrayType = ctypes.c_byte*bufferedArchiver.GetBufferSize()
        b = ArrayType.from_address(address)

        stream = io.BytesIO(b)

        with zipfile.ZipFile(stream) as zf:
            for info in zf.infolist():
                data = zf.read(info.filename)
                fdata = open(archiveName + '/' + info.filename, 'rb').read()
                if data != fdata:
                    print('Buffered file %s does not match file on disk' % info.filename)
                    exit(1)

        shutil.rmtree(archiveName)


if __name__ == "__main__":
     svtk.test.Testing.main([(TestBufferedRenderWindowExporter, 'test')])
