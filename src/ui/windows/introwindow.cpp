#include "introwindow.h"
#include "../delegates/connectiondelegate.h"
#include "../../utils/theme.h"

#include <QCoreApplication>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr int SearchRole = Qt::UserRole + 2;
constexpr int WindowWidth = 640;
constexpr int WindowHeight = 480;
constexpr int WindowContentMargin = 4;
constexpr int WindowTopMargin = 0;
constexpr int LeftPanelWidth = 200;
constexpr int LogoSize = 100;
constexpr int RightPanelLeftMargin = 8;
constexpr int RightPanelMargin = 12;
constexpr int RightPanelTopMargin = 0;
constexpr int RightPanelSpacing = 12;
constexpr int ConnectionSpacing = 2;
} // namespace

IntroWindow::IntroWindow(QWidget *parent)
    : AppWindow(parent)
{
    setFixedSize(WindowWidth, WindowHeight);
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint | Qt::WindowTitleHint
                    | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
                   & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);

    buildLayout();
}

void IntroWindow::paintEvent(QPaintEvent *event)
{
    AppWindow::paintEvent(event);
    if (!m_leftPanel)
        return;

    const int panelRight = m_leftPanel->mapTo(this, QPoint(m_leftPanel->width(), 0)).x();
    const int panelSpacing = centralWidget()->layout()->spacing();
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(QRect(0, 0, panelRight + panelSpacing / 2, height()),
                     Theme::color(Theme::ColorRole::SidebarTint, this));
}

void IntroWindow::buildLayout()
{
    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
#if defined(Q_OS_MACOS)
    central->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
#endif
    layout->setContentsMargins(WindowContentMargin, WindowTopMargin,
                               WindowContentMargin, WindowContentMargin);

    auto *leftPanel = new QWidget(this);
    m_leftPanel = leftPanel;
    leftPanel->setFixedWidth(LeftPanelWidth);

    auto *leftPanelLayout = new QVBoxLayout(leftPanel);

    auto *imageLbl = new QLabel(this);
    imageLbl->setPixmap(QPixmap(":/images/logo.png"));
    imageLbl->setFixedSize(LogoSize, LogoSize);
    imageLbl->setScaledContents(true);

    auto *titleLbl = new QLabel(this);
    titleLbl->setText(tr("SLONYARA"));
    Theme::applyFor(titleLbl, Theme::FontRole::Title, Theme::ColorRole::Primary);

    auto *versionLbl = new QLabel(this);
    QString appVersion = tr("Version: %1").arg(QCoreApplication::applicationVersion());
    versionLbl->setText(appVersion);
    Theme::applyFor(versionLbl, Theme::FontRole::Caption, Theme::ColorRole::Muted);

    auto *createBtn = new QPushButton(this);
    createBtn->setText(tr("New Connection"));

    auto *rightPanel = new QWidget(this);
    auto *rightPanelLayout = new QVBoxLayout(rightPanel);
    rightPanelLayout->setContentsMargins(RightPanelLeftMargin, RightPanelTopMargin,
                                         RightPanelMargin, RightPanelMargin);
    rightPanelLayout->setSpacing(RightPanelSpacing);

    auto *searchInput = new QLineEdit(rightPanel);
    Theme::applyFor(searchInput);
    searchInput->setClearButtonEnabled(true);
    searchInput->setPlaceholderText(tr("Search connections…"));
    searchInput->setAccessibleName(tr("Search connections…"));

    auto *connectionsModel = new QStandardItemModel(rightPanel);
    struct Connection {
        const char *name;
        const char *host;
        const char *database;
    };
    const Connection connections[] = {
        {"Local PostgreSQL", "localhost", "postgres"},
        {"Development", "127.0.0.1", "slonyara_dev"},
        {"Staging", "staging.db.example.com", "slonyara"},
        {"Production", "db.example.com", "slonyara"},
        {"Analytics", "10.0.1.10", "analytics"},
        {"Reporting", "10.0.1.11", "reports"},
        {"Warehouse", "warehouse.example.com", "warehouse"},
        {"Customers", "10.0.2.10", "customers"},
        {"Billing", "10.0.2.11", "billing"},
        {"Inventory", "10.0.2.12", "inventory"},
        {"Orders", "orders.db.example.com", "orders"},
    };
    for (const auto &connection : connections) {
        const QString name = QString::fromLatin1(connection.name);
        const QString address = QString::fromLatin1(connection.host) + ":"
                                + QString::fromLatin1(connection.database);
        auto *item = new QStandardItem(name);
        item->setData(address, ConnectionDelegate::AddressRole);
        item->setData(name + " " + address, SearchRole);
        item->setToolTip(name + "\n" + address);
        item->setEditable(false);
        connectionsModel->appendRow(item);
    }

    auto *filterModel = new QSortFilterProxyModel(rightPanel);
    filterModel->setSourceModel(connectionsModel);
    filterModel->setFilterRole(SearchRole);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    auto *connectionsView = new QListView(rightPanel);
    connectionsView->setMouseTracking(true);
    connectionsView->setModel(filterModel);
    connectionsView->setItemDelegate(new ConnectionDelegate(connectionsView));
    connectionsView->setUniformItemSizes(true);
    connectionsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connectionsView->setSelectionMode(QAbstractItemView::SingleSelection);
    connectionsView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connectionsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connectionsView->setSpacing(ConnectionSpacing);
    connect(searchInput, &QLineEdit::textChanged, filterModel,
            &QSortFilterProxyModel::setFilterFixedString);

    rightPanelLayout->addWidget(searchInput);
    rightPanelLayout->addWidget(connectionsView, 1);

    leftPanelLayout->addWidget(imageLbl, 0, Qt::AlignHCenter);
    leftPanelLayout->addWidget(titleLbl, 0, Qt::AlignHCenter);
    leftPanelLayout->addWidget(versionLbl, 0, Qt::AlignHCenter);
    leftPanelLayout->addStretch(1);
    leftPanelLayout->addWidget(createBtn);

    layout->addWidget(leftPanel);
    layout->addWidget(rightPanel, 1);

    setCentralWidget(central);
}
