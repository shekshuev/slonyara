#pragma once

#include <QFont>
#include <QColor>

class QLabel;
class QLineEdit;
class QWidget;
class StatusCard;

namespace Theme {

enum class FontRole { Default, Title, Header, BodyBold, Caption };
enum class ColorRole { Primary, Muted, Accent, ItemHover, ItemSelected, ItemBorder, SidebarTint,
                       Warning, Error, Info, Success };

QFont font(FontRole role = FontRole::Default);
QColor color(ColorRole role, const QWidget *widget = nullptr);

void applyFor(QLabel *label,
              FontRole font = FontRole::Default,
              ColorRole color = ColorRole::Primary);
void applyFor(QLineEdit *input);
void applyFor(StatusCard *card, ColorRole role = ColorRole::Info);
void setTheme(QWidget *window, bool isDark);

} // namespace Theme
