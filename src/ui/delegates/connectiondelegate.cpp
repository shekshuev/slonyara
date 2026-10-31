#include "connectiondelegate.h"
#include "../../utils/theme.h"

#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QWidget>

namespace {
#if defined(Q_OS_MACOS)
constexpr int HorizontalPadding = 20;
#else
constexpr int HorizontalPadding = 10;
constexpr qreal CornerRadius = 4.0;
#endif
constexpr int VerticalPadding = 4;
constexpr int TextLineSpacing = 2;
constexpr qreal BorderWidth = 1.0;
} // namespace

QSize ConnectionDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    const int textHeight = QFontMetrics(Theme::font(Theme::FontRole::BodyBold)).height()
                           + TextLineSpacing
                           + QFontMetrics(Theme::font(Theme::FontRole::Caption)).height();
    return QSize(0, VerticalPadding + textHeight + VerticalPadding);
}

void ConnectionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    const auto *widget = option.widget;

    painter->save();
    painter->setClipRect(option.rect);
    painter->setRenderHint(QPainter::Antialiasing);
    const bool selected = option.state.testFlag(QStyle::State_Selected);
    const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
    const bool focused = option.state.testFlag(QStyle::State_HasFocus);
    const QRectF background = QRectF(option.rect).adjusted(BorderWidth / 2, BorderWidth / 2,
                                                          -BorderWidth / 2, -BorderWidth / 2);
#if defined(Q_OS_MACOS)
    const qreal radius = background.height() / 2;
#else
    const qreal radius = CornerRadius;
#endif
    if (selected || hovered || focused) {
        painter->setPen(focused ? QPen(Theme::color(Theme::ColorRole::ItemBorder, widget), BorderWidth)
                                : QPen(Qt::NoPen));
        painter->setBrush(selected ? Theme::color(Theme::ColorRole::ItemSelected, widget)
                                  : Theme::color(Theme::ColorRole::ItemHover, widget));
        painter->drawRoundedRect(background, radius, radius);
    }
    const QRect content = option.rect.adjusted(HorizontalPadding, VerticalPadding,
                                               -HorizontalPadding, -VerticalPadding);
    const QColor textColor = Theme::color(Theme::ColorRole::Primary, widget);
    const QFont titleFont = Theme::font(Theme::FontRole::BodyBold);
    const QFont detailFont = Theme::font(Theme::FontRole::Caption);
    const QFontMetrics titleMetrics(titleFont);
    const QFontMetrics detailMetrics(detailFont);
    const QRect titleRect(content.topLeft(), QSize(content.width(), titleMetrics.height()));
    const QRect detailRect(content.left(), titleRect.y() + titleRect.height() + TextLineSpacing,
                           content.width(), detailMetrics.height());
    painter->setFont(titleFont);
    painter->setPen(textColor);
    painter->drawText(titleRect, Qt::AlignVCenter,
                      titleMetrics.elidedText(index.data().toString(),
                                              Qt::ElideRight, content.width()));

    painter->setFont(detailFont);
    painter->setPen(Theme::color(Theme::ColorRole::Muted, widget));
    painter->drawText(detailRect, Qt::AlignVCenter,
                      detailMetrics.elidedText(index.data(AddressRole).toString(),
                                               Qt::ElideRight, content.width()));
    painter->restore();
}
