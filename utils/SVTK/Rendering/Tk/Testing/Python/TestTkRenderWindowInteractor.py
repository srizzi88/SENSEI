import os, sys
import svtk
from svtk.test import Testing

if sys.hexversion < 0x03000000:
    # for Python2
    import Tkinter as tkinter
    from Tkinter import Pack
else:
    # for Python3
    import tkinter
    from tkinter import Pack

from svtk.tk.svtkTkRenderWindowInteractor import svtkTkRenderWindowInteractor


class TestTkRenderWindowInteractor(Testing.svtkTest):

    # Stick your SVTK pipeline here if you want to create the pipeline
    # only once.  If you put it in the constructor or in the function
    # the pipeline will be created afresh for each and every test.

    # create a dummy Tkinter root window.
    root = tkinter.Tk()

    # create a rendering window and renderer
    ren = svtk.svtkRenderer()
    tkrw = svtkTkRenderWindowInteractor(root, width=300, height=300)
    tkrw.Initialize()
    tkrw.pack()
    rw = tkrw.GetRenderWindow()
    rw.AddRenderer(ren)

    # create an actor and give it cone geometry
    cs = svtk.svtkConeSource()
    cs.SetResolution(8)
    map = svtk.svtkPolyDataMapper()
    map.SetInputConnection(cs.GetOutputPort())
    act = svtk.svtkActor()
    act.SetMapper(map)

    # assign our actor to the renderer
    ren.AddActor(act)

    def testsvtkTkRenderWindowInteractor(self):
        "Test if svtkTkRenderWindowInteractor works."
        self.tkrw.Start()
        self.tkrw.Render()
        self.root.update()
        img_file = "TestTkRenderWindowInteractor.png"
        Testing.compareImage(self.rw, Testing.getAbsImagePath(img_file))
        Testing.interact()
        self.tkrw.destroy()

    # These are useful blackbox tests (not dummy ones!)
    def testParse(self):
        "Test if svtkTkRenderWindowInteractor is parseable"
        self._testParse(self.tkrw)

    def testGetSet(self):
        "Testing Get/Set methods of svtkTkRenderWindowInteractor"
        self._testGetSet(self.tkrw)

    def testBoolean(self):
        "Testing Boolean methods of svtkTkRenderWindowInteractor"
        self._testBoolean(self.tkrw)

if __name__ == "__main__":
    cases = [(TestTkRenderWindowInteractor, 'test')]
    del TestTkRenderWindowInteractor
    Testing.main(cases)
