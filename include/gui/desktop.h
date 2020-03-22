#ifndef __MYOS__GUI__DESKTOP_H
#define __MYOS__GUI__DESKTOP_H

#include <gui/widget.h>
#include <drivers/mouse.h>

namespace myos {

  namespace gui {

    // the Desktop will basically just work like the CompositeWidget
    // but we will have to overwrite the methods from the MouseEventHandler to transate the mouse movements 
    // because from the mouse we only ge relative movements
    // and so the desktop will turn these relative movements into absolute positions of the mouse.
    class Desktop : public CompositeWidget, public drivers::MouseEventHandler {
      protected:
        // the Desktop will get mouse coordinates
        // but it's apparently not a good idea to hard code a mouse into the Desktop
        // for example because of tablets doesn't have a mouse cursor
        // but anyways I will do this way now
        common::uint32_t MouseX;
        common::uint32_t MouseY;

      public:
        Desktop(common::int32_t w, common::int32_t h,
            common::uint8_t r, common::uint8_t g, common::uint8_t b);
        ~Desktop();

        // The Desktop will be responsible for drawing the mouse
        // so we have to overwrite the drawing of them desktop
        void Draw(common::GraphicsContext* gc);

        // MouseEventHandler
        void OnMouseDown(common::uint8_t button);
        void OnMouseUp(common::uint8_t button);
        void OnMouseMove(int x, int y);
    };

  }

}


#endif
