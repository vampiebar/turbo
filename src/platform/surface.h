#ifndef TURBO_SURFACE_H
#define TURBO_SURFACE_H

#define Uses_TPoint
#define Uses_TRect
#include <tvision/tv.h>

#include <ScintillaHeaders.h>

class TDrawSurface;
struct TScreenCell;
struct TCellAttribs;

namespace Scintilla {

    struct TPRect : public TRect {

        using TRect::TRect;
        TPRect(PRectangle rc);

    };

    inline TPRect::TPRect(PRectangle rc) :
        TRect({(int) rc.left, (int) rc.top, (int) rc.right, (int) rc.bottom})
    {
    }

    struct TScintillaSurface : public Surface {

        TDrawSurface *view {0};
        TPRect clip {0, 0, 0, 0};

        TPRect clipRect(TPRect r);

        static TScreenCell makeCell(uchar ch, ColourDesired fore, ColourDesired back);
        static TCellAttribs convertColor(ColourDesired c);
        static TCellAttribs convertColorPair(ColourDesired fore, ColourDesired back);
        static ColourDesired convertColor(TCellAttribs c);

        void Init(WindowID wid) override;
        void Init(SurfaceID sid, WindowID wid) override;
        void InitPixMap(int width, int height, Surface *surface_, WindowID wid) override;

        void Release() override;
        bool Initialised() override;
        void PenColour(ColourDesired fore) override;
        int LogPixelsY() override;
        int DeviceHeightFont(int points) override;
        void MoveTo(int x_, int y_) override;
        void LineTo(int x_, int y_) override;
        void Polygon(Point *pts, size_t npts, ColourDesired fore, ColourDesired back) override;
        void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) override;
        void FillRectangle(PRectangle rc, ColourDesired back) override;
        void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
        void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) override;
        void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
                ColourDesired outline, int alphaOutline, int flags) override;
        void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) override;
        void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) override;
        void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) override;
        void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

        std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;

        void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) override;
        void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) override;
        void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore) override;
        void MeasureWidths(Font &font_, std::string_view text, XYPOSITION *positions) override;
        XYPOSITION WidthText(Font &font_, std::string_view text) override;
        XYPOSITION Ascent(Font &font_) override;
        XYPOSITION Descent(Font &font_) override;
        XYPOSITION InternalLeading(Font &font_) override;
        XYPOSITION Height(Font &font_) override;
        XYPOSITION AverageCharWidth(Font &font_) override;

        void SetClip(PRectangle rc) override;
        void FlushCachedState() override;

        void SetUnicodeMode(bool unicodeMode_) override;
        void SetDBCSMode(int codePage) override;
        void SetBidiR2L(bool bidiR2L_) override;

    };

}

namespace Scintilla {

    inline TPRect TScintillaSurface::clipRect(TPRect r) {
        // The 'clip' member is already intersected with the view's extent.
        // See SetClip().
        r.intersect(clip);
        return r;
    }

    inline TScreenCell TScintillaSurface::makeCell(uchar ch, ColourDesired fore, ColourDesired back)
    {
        TScreenCell c {0};
        c.Char = ch;
        c.Attr = convertColorPair(fore, back);
        return c;
    }

    inline TCellAttribs TScintillaSurface::convertColor(ColourDesired c)
    {
        TCellAttribs attr {0};
        attr = attr | (c.GetGreen() << 8) | c.GetRed();
        return attr;
    }

    inline TCellAttribs TScintillaSurface::convertColorPair(ColourDesired fore, ColourDesired back)
    {
        TCellAttribs attr {0};
        attr.fgSet(fore.GetRed());
        attr.bgSet(back.GetRed());
        attr = attr | (fore.GetGreen() << 8);
        return attr;
    }

    inline ColourDesired TScintillaSurface::convertColor(TCellAttribs c)
    {
        return ColourDesired(c & 0xFF, (c & 0xFF00) >> 8, 0);
    }

}

#endif
