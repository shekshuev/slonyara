#include "windoweffects_p.h"

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QTimer>
#include <QWindow>

namespace {
constexpr auto effectsObjectName = "slonyaraMacWindowEffects";
constexpr int titleBarHeight = 38;
constexpr int buttonAreaWidth = 78;
constexpr int titleBarMargin = 12;

class TitleBar final : public QWidget
{
public:
    explicit TitleBar(QMainWindow *window) : QWidget(window)
    {
        setFixedHeight(titleBarHeight);
        setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(titleBarMargin, 0, titleBarMargin, 0);
        layout->addSpacing(buttonAreaWidth);
        auto *label = new QLabel(window->windowTitle(), this);
        label->setAlignment(Qt::AlignCenter);
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        label->setStyleSheet("font-weight: 600; font-size: 13px;");
        QObject::connect(window, &QWidget::windowTitleChanged, label, &QLabel::setText);
        layout->addWidget(label, 1);
        layout->addSpacing(buttonAreaWidth);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragPending = true;
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_dragPending && (event->buttons() & Qt::LeftButton)
            && window()->windowHandle()) {
            m_dragPending = false;
            if (window()->windowHandle()->startSystemMove()) {
                event->accept();
                return;
            }
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        m_dragPending = false;
        QWidget::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragPending = false;
            auto *view = reinterpret_cast<NSView *>(window()->internalWinId());
            [view.window performZoom:nil];
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    bool m_dragPending = false;
};

class MacWindowEffects final : public QObject
{
public:
    explicit MacWindowEffects(QMainWindow *window) : QObject(window), m_window(window)
    {
        setObjectName(QString::fromLatin1(effectsObjectName));
        m_effect = [[NSVisualEffectView alloc] initWithFrame:NSZeroRect];
        m_effect.material = NSVisualEffectMaterialUnderWindowBackground;
        m_effect.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        m_effect.state = NSVisualEffectStateFollowsWindowActiveState;
        m_effect.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        m_observers = [[NSMutableArray alloc] init];
        auto *center = [NSNotificationCenter defaultCenter];
        for (NSNotificationName name in @[
                 NSWindowWillEnterFullScreenNotification, NSWindowDidEnterFullScreenNotification,
                 NSWindowWillExitFullScreenNotification, NSWindowDidExitFullScreenNotification,
                 NSWindowDidResizeNotification, NSWindowDidEndSheetNotification]) {
            id observer = [center addObserverForName:name object:nil queue:nil
                                         usingBlock:^(NSNotification *notification) {
                if (!m_view || notification.object != m_view.window)
                    return;
                if ([notification.name isEqualToString:NSWindowWillEnterFullScreenNotification]
                    || [notification.name isEqualToString:NSWindowWillExitFullScreenNotification]) {
                    m_transition = true;
                } else if ([notification.name isEqualToString:NSWindowDidEnterFullScreenNotification]
                           || [notification.name isEqualToString:NSWindowDidExitFullScreenNotification]) {
                    m_transition = false;
                }
                apply();
                if (!m_refreshPending) {
                    m_refreshPending = true;
                    QTimer::singleShot(0, this, [this] {
                        m_refreshPending = false;
                        apply();
                    });
                }
            }];
            [m_observers addObject:observer];
        }
    }

    ~MacWindowEffects() override
    {
        for (id observer in m_observers)
            [[NSNotificationCenter defaultCenter] removeObserver:observer];
        [m_observers release];
        [m_effect removeFromSuperview];
        [m_effect release];
        [m_view release];
    }

    bool apply()
    {
        auto *view = reinterpret_cast<NSView *>(m_window->internalWinId());
        if (!view || !view.window || !view.superview)
            return false;
        if (view != m_view) {
            [view retain];
            [m_view release];
            m_view = view;
        }
        NSWindow *nativeWindow = view.window;
        nativeWindow.opaque = NO;
        nativeWindow.backgroundColor = NSColor.clearColor;
        nativeWindow.titlebarAppearsTransparent = YES;
        nativeWindow.titleVisibility = NSWindowTitleHidden;
        view.wantsLayer = YES;
        view.layer.opaque = NO;
        view.layer.backgroundColor = NSColor.clearColor.CGColor;

        if (m_effect.superview != view.superview) {
            [m_effect removeFromSuperview];
            [view.superview addSubview:m_effect positioned:NSWindowBelow relativeTo:view];
        }
        m_effect.frame = view.frame;
        if (!m_transition && !(nativeWindow.styleMask & NSWindowStyleMaskFullScreen))
            positionButtons(nativeWindow, view);
        return true;
    }

private:
    void positionButtons(NSWindow *window, NSView *view)
    {
        NSButton *close = [window standardWindowButton:NSWindowCloseButton];
        NSButton *minimize = [window standardWindowButton:NSWindowMiniaturizeButton];
        NSButton *zoom = [window standardWindowButton:NSWindowZoomButton];
        if (!close || !minimize || !zoom || !minimize.superview)
            return;
        const CGFloat spacing = NSMinX(minimize.frame) - NSMinX(close.frame);
        const CGFloat y = view.isFlipped ? titleBarHeight / 2.0
                                        : NSHeight(view.bounds) - titleBarHeight / 2.0;
        const NSPoint center = [minimize.superview
            convertPoint:NSMakePoint(titleBarMargin + buttonAreaWidth / 2.0, y) fromView:view];
        const CGFloat x = center.x - NSWidth(minimize.frame) / 2;
        const CGFloat bottom = center.y - NSHeight(minimize.frame) / 2;
        [close setFrameOrigin:NSMakePoint(x - spacing, bottom)];
        [minimize setFrameOrigin:NSMakePoint(x, bottom)];
        [zoom setFrameOrigin:NSMakePoint(x + spacing, bottom)];
    }

    QMainWindow *m_window;
    NSVisualEffectView *m_effect = nil;
    NSView *m_view = nil;
    NSMutableArray *m_observers = nil;
    bool m_transition = false;
    bool m_refreshPending = false;
};
}

namespace WindowEffects {
void prepare(QMainWindow *window)
{
    window->setAttribute(Qt::WA_TranslucentBackground);
    window->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    window->setWindowFlags(window->windowFlags() | Qt::ExpandedClientAreaHint
                          | Qt::NoTitleBarBackgroundHint);
}

bool setup(QMainWindow *window)
{
    if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
        return false;
    window->setMenuWidget(new TitleBar(window));
    auto *effects = new MacWindowEffects(window);
    window->winId();
    return effects->apply();
}

bool refresh(QMainWindow *window)
{
    auto *effects = static_cast<MacWindowEffects *>(window->findChild<QObject *>(
        QString::fromLatin1(effectsObjectName), Qt::FindDirectChildrenOnly));
    return effects && effects->apply();
}
}
