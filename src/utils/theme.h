#pragma once

#include <QFont>
#include <QColor>

class QLabel;
class QLineEdit;
class QWidget;

namespace Theme {

enum class FontRole { Default, Title, Header, BodyBold, Caption };
enum class ColorRole { Primary, Muted, Accent, ItemHover, ItemSelected, ItemBorder, SidebarTint };

QFont font(FontRole role = FontRole::Default);
QColor color(ColorRole role, const QWidget *widget = nullptr);

void applyFor(QLabel *label,
              FontRole font = FontRole::Default,
              ColorRole color = ColorRole::Primary);
void applyFor(QLineEdit *input);
void setTheme(QWidget *window, bool isDark);

} // namespace Theme
