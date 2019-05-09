#ifndef __MYOS__GUI__WIDGET_H
#define __MYOS__GUI__WIDGET_H

#include <common/types.h>
#include <common/graphicscontext.h>

namespace myos {

  namespace gui {

    class Widget {
      protected:
        Widget* parent;

        common::int32_t x;
        common::int32_t y;
        common::int32_t w;
        common::int32_t h;

        // to prevent writing color here
        // just hard code r, g, b
        common::int32_t r;
        common::int32_t g;
        common::int32_t b;

        bool Focussable;

      public:
        Widget(Widget* parent, common::int32_t x, common::int32_t y, common::int32_t w,
            common::int32_t h, common::int32_t r, common::int32_t g, common::int32_t b);
        ~Widget();

        virtual void GetFocus(Widget* widget);
        virtual void ModelToScreen(common::int32_t& x, common::int32_t& y);

        virtual void Draw(GraphicsContext* gc);
        virtual void OnMouseDown(common::int32_t x, common::int32_t y);
        virtual void OnMouseUp(common::int32_t x, common::int32_t y);
        virtual void OnMouseMove(common::int32_t oldx, common::int32_t oldy, common::int32_t newx, common::int32_t newy);

        virtual void OnKeyDown(common::int32_t x, common::int32_t y);
        virtual void OnKeyUp(common::int32_t x, common::int32_t y);

    };

    // CompositeWidget which will overwrite most of Wiget
    class CompositeWidget : public Widget {
      private:
        Widget* children[100];
        int numChildren;

        Widget* focussedChild;

      public:

        CompositeWidget(Widget* parent, common::int32_t x, common::int32_t y, common::int32_t w,
            common::int32_t h, common::int32_t r, common::int32_t g, common::int32_t b);
        ~CompositeWidget();

        virtual void GetFocus(Widget* widget);

        virtual void Draw(GraphicsContext* gc);
        virtual void OnMouseDown(common::int32_t x, common::int32_t y);
        virtual void OnMouseUp(common::int32_t x, common::int32_t y);
        virtual void OnMouseMove(common::int32_t oldx, common::int32_t oldy, common::int32_t newx, common::int32_t newy);

        virtual void OnKeyDown(common::int32_t x, common::int32_t y);
        virtual void OnKeyUp(common::int32_t x, common::int32_t y);
    };
  }

}

#endif
