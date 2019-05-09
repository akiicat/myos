#include <gui/widget.h>

using namespace myos::gui;

Widget::Widget(Widget* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t g, int32_t b) {
  this->parent = parent;
  this->x = x;
  this->y = y;
  this->w = w;
  this->h = h;
  this->r = r;
  this->g = g;
  this->b = b;
  this->Focussable = Focussable;
}

Widget::~Widget() {

}

void Widget::GetFocus(Widget* widget) {
  // so the get focus in the default inplementation is
  // just passes us on to the parent widget unless there isn't any

  // so if the parent is not zero then we pass the call to the parent
  if (parent != 0) {
    parent->GetFocus(widget);
  }
}

void Widget::ModelToScreen(int32_t& x, int32_t& y) {
  if (parent != 0) {
    parent->ModelToScreen(x, y);
  }

  // to get the absolute coordinates,
  // we just get the offset from the parent and then add our own sffset to this
  x += this->x;
  y += this->y;
}

void Widget::Draw(GraphicsContext* gc) {

  // we need the absolute coordinate at this point
  //
  // actually we set this to zero because the ModelToScreen also add our X and Y to this
  int X = 0;
  int Y = 0;
  ModelToScreen(X, Y);

  // then we proceed with the rectangel with X and Y width and height and our colors
  gc->FillRectangle(X, Y, w, h, r, g, b);
}

void Widget::OnMouseDown(int32_t x, int32_t y) {
  // is the widget focusable? if it isn't then we don't call this
  if (Focussable) {
    GetFocus(this);
  }
}

void Widget::OnMouseUp(int32_t x, int32_t y) {
}

void Widget::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy) {
}

void Widget::OnKeyDown(char* str) {
}

void Widget::OnKeyUp(char* str) {
}

CompositeWidget::CompositeWidget(Widget* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t g, int32_t b) {
  focussedChild = 0;
  numChildren = 0;
}


CompositeWidget::~CompositeWidget()

void CompositeWidget::GetFocus(Widget* widget) {
  // now the focussedChild is the widget
  this->focussedChild = widget;

  if (parent != 0) {
    parent->GetFocus(this);
  }
}

void CompositeWidget::Draw(GraphicsContext* gc) {
  // first draw its own background
  Widget::Draw(gc);

  // here we will iterate through the children
  // this draws the children
  for (int i = numChildren-1; i >= 0; --i) {
    children->Draw(gc);
  }
}

void CompositeWidget::OnMouseDown(int32_t x, int32_t y) {
  // here we actually need to go from zero to numChildren
  // because the children with smaller index are in front of the other ones
  for (int i = 0; i < numChildren; i++) {
    // pass the event to the child that contains the coordinate
    if (children[i]->ContainsCoordinate(x - this->x, y -  this->y)) {
      children->OnMouseDown(x - this->x, y -  this->y);

      // If you have two children and one is in front of the other
      // then the front child is encountered first and should stop the iteration
      // because the other child is hidden behind the front child
      // only the front child is supposed to get this event
      break;
    }
  }

}

void CompositeWidget::OnMouseUp(int32_t x, int32_t y) {
  for (int i = 0; i < numChildren; i++) {
    if (children[i]->ContainsCoordinate(x - this->x, y -  this->y)) {
      children->OnMouseUp(x - this->x, y -  this->y);
      break;
    }
  }
}

void CompositeWidget::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy) {


  int firstchild = -1

  // with the OnMouseMove, we will do this twice
  //
  // once for the object that contains the old coordinate and
  // once for the object that contains the new coordinator
  for (int i = 0; i < numChildren; i++) {
    if (children[i]->ContainsCoordinate(oldx - this->x, oldy -  this->y)) {
      // so I'm always subtracting the this.x and this.y,
      // turn the coordinates into relative coordinates
      children->OnMouseMove(oldx - this->x, oldy -  this->y, newx - this->x, newy - this->);
      
      firstchild = i;

      break;
    }
  }

  for (int i = 0; i < numChildren; i++) {
    if (children[i]->ContainsCoordinate(newx - this->x, newy -  this->y)) {

      // so if the MouseMove leaves the one element,
      // then that element gets only the first OnMouseMove end
      // If the MouseMove enters a child and that child gets another OnMouseMove
      // and if you move the mouse within one child it gets the same that is called twice
      //
      // So If we move the mouse within the same widget,
      // it doesn't get the same event twice like this
      if (firsthild != i) 
        children->OnMouseMove(oldx - this->x, oldy -  this->y, newx - this->x, newy - this->);
      break;
    }
  }
}

void CompositeWidget::OnKeyDown(char* str) {
  if (focussedChild != 0) {
    focussedChild->OnKeyDown(str);
  }
}

void CompositeWidget::OnKeyUp(char* str) {
  if (focussedChild != 0) {
    focussedChild->OnKeyUp(str);
  }
}