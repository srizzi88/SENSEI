package svtk.rendering.awt;

import java.awt.Canvas;
import java.awt.Graphics;

import svtk.svtkRenderWindow;

public class svtkInternalAwtComponent extends Canvas {
  protected native int RenderCreate(svtkRenderWindow renderWindow);

  private static final long serialVersionUID = -7756069664577797620L;
  private svtkAwtComponent parent;

  public svtkInternalAwtComponent(svtkAwtComponent parent) {
    this.parent = parent;
    this.addMouseListener(this.parent.getInteractorForwarder());
    this.addMouseMotionListener(this.parent.getInteractorForwarder());
    this.addMouseWheelListener(this.parent.getInteractorForwarder());
    this.addKeyListener(this.parent.getInteractorForwarder());
  }

  public void addNotify() {
    super.addNotify();
    parent.isWindowCreated = false;
    parent.getRenderWindow().SetForceMakeCurrent();
    parent.updateInRenderCall(false);
  }

  public void removeNotify() {
    parent.updateInRenderCall(true);
    super.removeNotify();
  }

  public void paint(Graphics g) {
    parent.Render();
  }

  public void update(Graphics g) {
    parent.Render();
  }
}
