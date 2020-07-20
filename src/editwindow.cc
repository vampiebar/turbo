#define Uses_TScrollBar
#define Uses_MsgBox
#include <tvision/tv.h>

#include "editwindow.h"
#include "docview.h"
#include "app.h"

EditorWindow::EditorWindow(const TRect &bounds, std::string_view aFile) :
    TWindow(bounds, nullptr, wnNoNumber),
    TWindowInit(&initFrame),
    drawing(false),
    lastSize(size),
    file(aFile),
    inSavePoint(true),
    MRUhead(this),
    editorView(editorBounds())
{
    options |= ofTileable;
    setState(sfShadow, False);

    editorView.hide();

    hScrollBar = new TScrollBar(TRect( 18, size.y - 1, size.x - 2, size.y ));
    hScrollBar->hide();
    insert(hScrollBar);

    vScrollBar = new TScrollBar(TRect( size.x - 1, 1, size.x, size.y - 1 ));
    vScrollBar->hide();
    insert(vScrollBar);

    leftMargin = new TDrawSubView(TRect( 1, 1, 6, size.y - 1 ), editorView);
    leftMargin->options |= ofFramed;
    leftMargin->growMode = gfGrowHiY | gfFixed;
    insert(leftMargin);

    docView = new DocumentView( TRect( 7, 1, size.x - 1, size.y - 1 ),
                                editorView,
                                editor,
                                *this );
    insert(docView);

    tryLoadFile();
    setUpEditor();
}

EditorWindow::~EditorWindow()
{
    if (TVEditApp::app)
        TVEditApp::app->removeEditor(this);
}

TRect EditorWindow::editorBounds() const
{
    // Editor size: the window's inside minus the line numbers frame.
    TRect r = getExtent().grow(-1, -1);
    r.b.x--;
    return r;
}

void EditorWindow::setUpEditor()
{
    // Editor should take into account the size of docView.
    editor.setWindow(&editorView);
    // But should send notifications to this window.
    editor.setParent(this);

    // Colors
    uchar color = 0x1E; // Blue & Light Yellow.
    uchar colorSel = 0x71; // White & Blue.
    editorView.setFillColor(color); // Screw palettes, they are too hard to understand.
    editor.setStyleColor(STYLE_DEFAULT, color);
    editor.WndProc(SCI_STYLECLEARALL, 0U, 0U); // Must be done before setting other colors.
    editor.setSelectionColor(colorSel);

    // Line numbers
    int marginWidth = 5;
    editor.setStyleColor(STYLE_LINENUMBER, color);
    editor.WndProc(SCI_SETMARGINS, 1, 0U);
    editor.WndProc(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
    editor.WndProc(SCI_SETMARGINWIDTHN, 0, marginWidth);

    // The document view does not contain the margin.
    docView->setDelta({marginWidth, 0});

    // Dynamic horizontal scroll
    editor.WndProc(SCI_SETSCROLLWIDTHTRACKING, true, 0U);
    editor.WndProc(SCI_SETSCROLLWIDTH, 1, 0U);
    editor.WndProc(SCI_SETXCARETPOLICY, CARET_EVEN, 0);
    // Trick so that the scroll width gets computed.
    editor.WndProc(SCI_SETFIRSTVISIBLELINE, 1, 0U);
    editor.WndProc(SCI_SETFIRSTVISIBLELINE, 0, 0U);

    // If we wanted line wrapping, we would enable this:
//     WndProc(SCI_SETWRAPMODE, SC_WRAP_WORD, nil);

    redrawEditor();
}

void EditorWindow::redrawEditor()
{
    if (!drawing) {
        drawing = true;
        lock();
        editor.changeSize();
        editor.draw(editorView);
        leftMargin->drawView();
        docView->drawView();
        hScrollBar->drawView();
        vScrollBar->drawView();
        unlock();
        drawing = false;
    }
}

void EditorWindow::handleEvent(TEvent &ev) {
    if (ev.what == evBroadcast && ev.message.command == cmScrollBarChanged) {
        if (scrollBarChanged((TScrollBar *) ev.message.infoPtr)) {
            redrawEditor();
            clearEvent(ev);
        }
    }
    TWindow::handleEvent(ev);
}

void EditorWindow::changeBounds(const TRect &bounds)
{
    lock();
    lockSubViews();
    TWindow::changeBounds(bounds);
    unlockSubViews();
    if (size != lastSize) {
        editorView.changeBounds(editorBounds());
        redrawEditor();
        lastSize = size;
    }
    unlock();
}

void EditorWindow::setState(ushort aState, Boolean enable)
{
    TWindow::setState(aState, enable);
    if (state & sfExposed) {
        // Otherwise, we could be in the middle of shutdown and our
        // subviews could have already been free'd.
        switch (aState) {
            case sfActive:
                hScrollBar->setState(sfVisible, enable);
                vScrollBar->setState(sfVisible, enable);
                if (enable && TVEditApp::app)
                    TVEditApp::app->tellFocusedEditor(this);
                break;
        }
    }
}

Boolean EditorWindow::valid(ushort command)
{
    if (command == cmValid && !error.empty()) {
        messageBox(error.c_str(), mfError | mfOKButton);
        return False;
    }
    return True;
}

const char* EditorWindow::getTitle(short)
{
    if (!title.empty())
        return title.c_str();
    if (!file.empty())
        return file.c_str();
    return nullptr;
}

void EditorWindow::sizeLimits( TPoint& min, TPoint& max )
{
    TView::sizeLimits(min, max);
    min = minEditWinSize;
}

void EditorWindow::lockSubViews()
{
    for (auto *v : std::initializer_list<TView *> {docView, leftMargin, vScrollBar})
        v->setState(sfExposed, False);
}

void EditorWindow::unlockSubViews()
{
    for (auto *v : std::initializer_list<TView *> {docView, leftMargin, vScrollBar})
        v->setState(sfExposed, True);
}

void EditorWindow::scrollBarEvent(TEvent ev)
{
    hScrollBar->handleEvent(ev);
    vScrollBar->handleEvent(ev);
}

bool EditorWindow::scrollBarChanged(TScrollBar *bar)
{
    if (bar == vScrollBar) {
        editor.WndProc(SCI_SETFIRSTVISIBLELINE, bar->value, 0U);
        return true;
    } else if (bar == hScrollBar) {
        editor.WndProc(SCI_SETXOFFSET, bar->value, 0U);
        return true;
    }
    return false;
}

void EditorWindow::scrollTo(TPoint delta)
{
    editor.WndProc(SCI_SETXOFFSET, std::clamp(delta.x, 0, hScrollBar->maxVal), 0U);
    editor.WndProc(SCI_SETFIRSTVISIBLELINE, delta.y, 0U);
}

void EditorWindow::notify(SCNotification scn)
{
    switch (scn.nmhdr.code) {
        case SCN_SAVEPOINTLEFT: setSavePointLeft(); break;
        case SCN_SAVEPOINTREACHED: setSavePointReached(); break;
    }
}

void EditorWindow::setHorizontalScrollPos(int delta, int limit)
{
    int size = docView->size.x;
    hScrollBar->setParams(delta, 0, limit - size, size - 1, 1);
}

void EditorWindow::setVerticalScrollPos(int delta, int limit)
{
    int size = docView->size.y;
    vScrollBar->setParams(delta, 0, limit - size, size - 1, 1);
}

void EditorWindow::setSavePointLeft()
{
    if (inSavePoint) {
        inSavePoint = false;
        title.append("*"sv);
        frame->drawView();
    }
}

void EditorWindow::setSavePointReached()
{
    if (!inSavePoint) {
        inSavePoint = true;
        title.erase(title.size() - 1, 1);
        frame->drawView();
    }
}

/////////////////////////////////////////////////////////////////////////
// File opening and saving.

// Note: the 'error' variable set here is later checked in valid() for
// command cmValid. If there was an error, a messageBox will be shown and
// valid() will return False, thus resulting in the EditorWindow being destroyed.

#include <memory>
#include <fstream>

void EditorWindow::tryLoadFile()
{
    if (!file.empty()) {
        std::error_code ec;
        file.assign(std::filesystem::absolute(file, ec));
        if (!ec)
            loadFile();
        else
            error = fmt::format("'{}' is not a valid path.", file.native());
    }
}

void EditorWindow::loadFile()
{
    std::ifstream f(file, ios::in | ios::binary);
    if (f) {
        f.seekg(0, ios::end);
        size_t fSize = f.tellg();
        f.seekg(0);
        // Allocate 1000 extra bytes, as in SciTE.
        editor.WndProc(SCI_ALLOCATE, fSize + 1000, 0U);
        if (fSize > (1 << 20))
            // Disable word wrap on big files.
            editor.WndProc(SCI_SETWRAPMODE, SC_WRAP_NONE, 0U);
        if (fSize) {
            bool ok = true;
            constexpr size_t blockSize = 1 << 20; // Read in chunks of 1 MiB.
            size_t readSize = std::min(fSize, blockSize);
            std::unique_ptr<char[]> buffer {new char[readSize]};
            sptr_t wParam = reinterpret_cast<sptr_t>(buffer.get());
            while (fSize > 0 && (ok = bool(f.read(buffer.get(), readSize)))) {
                editor.WndProc(SCI_APPENDTEXT, readSize, wParam);
                fSize -= readSize;
                if (fSize < readSize)
                    readSize = fSize;
            };
            if (!ok)
                error = fmt::format("An error occurred while reading file '{}'.", file.native());
        }
    } else
        error = fmt::format("Unable to open file '{}'.", file.native());
}
