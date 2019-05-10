#include <gui/desktop.h>

using namespace myos;
using namespace myos::common;
using namespace myos::gui;

Desktop::Desktop(int32_t w, int32_t h, uint32_t r, uint32_t g, uint32_t b)
  // so the desktop constructor will go to the base constructor
  // assuming here that there's only a top-level desktop
  //
  // not have a parent: 0
  // x, y coordinate: 0, 0. It's supposed to start in the upper left corner
  // w, h, r, g, b: you could pass other parameters here
  //    but a desktop as a child maybe for a window for example would be probably a good idea
  //    if you wanted to do something like multi document interface (MDI) form parent that you possibly know from Windows
  //    you can have a window which has child windows in them
  //    so then it would make sense to have a desktop which has a parent and
  //    something else then 0 for x and y in x
  : CompositeWidget(0, 0, 0, w, h, r, g, b),
  MouseEventHandler()
{

  // the initially mouse position at the center of the screen
  MouseX = w / 2;
  MouseY = h / 2;
}

Desktop::~Desktop() {

}

void Desktop::Draw(GraphicsContext* gc) {
  // call the parent Draw
  CompositeWidget::Draw(gc);

  // after the screen has been drawn, we put the mouse cursor in
  // make a little white cross mouse cursor
  for (int i = 0; i < 4; i++) {
    gc->PutPixel(MouseX-i, MouseY, 0xFF, 0xFF,0xFF);
    gc->PutPixel(MouseX+i, MouseY, 0xFF, 0xFF,0xFF);
    gc->PutPixel(MouseX, MouseY-i, 0xFF, 0xFF,0xFF);
    gc->PutPixel(MouseX, MouseY+i, 0xFF, 0xFF,0xFF);
  }
}

void Desktop::OnMouseDown(uint8_t button) {
  // translates button to the widget from where you get the mouse position
  CompositeWidget::OnMouseDown(MouseX, MouseY, button);
}

void Desktop::OnMouseUp(uint8_t button) {
  CompositeWidget::OnMouseUp(MouseX, MouseY, button);
}

void Desktop::OnMouseMove(int x, int y) {

  // at the beginning I will just divide the relative movement by four
  // because it's relatively fast
  x /= 4;
  y /= 4;

  // make sure that the mouse stays inside the the screen
  int32_t newMouseX = MouseX + x;
  if (newMouseX < 0) newMouseX = 0;
  if (newMouseX >= w) newMouseX = w - 1;

  int32_t newMouseY = MouseY + y;
  if (newMouseY < 0) newMouseY = 0;
  if (newMouseY >= h) newMouseY = h - 1;

  // call the MouseEventHandler
  CompositeWidget::OnMouseMove(MouseX, MouseY, newMouseX, newMouseY);

  // and then we store the new values back in the coordinates
  MouseX = newMouseX;
  MouseY = newMouseY;
}

