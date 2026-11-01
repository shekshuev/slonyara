#include "introwindow.h"
#include "../delegates/connectiondelegate.h"
#include "../../utils/theme.h"

#include <QCoreApplication>
#include "../dialogs/connectiondialog.h"
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
constexpr int ConnectionIdRole = Qt::UserRole + 3;
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
    connect(createBtn, &QPushButton::clicked, this, &IntroWindow::createConnection);

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

    m_connectionsModel = new QStandardItemModel(rightPanel);
    reloadConnections();

    auto *filterModel = new QSortFilterProxyModel(rightPanel);
    filterModel->setSourceModel(m_connectionsModel);
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

void IntroWindow::reloadConnections()
{
    m_connectionsModel->clear();
    for (const auto &connection : m_connectionStore.connections()) {
        const QString address = QStringLiteral("%1:%2/%3")
                                    .arg(connection.host).arg(connection.port)
                                    .arg(connection.database);
        auto *item = new QStandardItem(connection.name);
        item->setData(connection.id, ConnectionIdRole);
        item->setData(address, ConnectionDelegate::AddressRole);
        item->setData(connection.name + " " + address + " " + connection.user, SearchRole);
        item->setToolTip(connection.name + "\n" + address);
        item->setEditable(false);
        m_connectionsModel->appendRow(item);
    }
}

void IntroWindow::createConnection()
{
    auto *dialog = new ConnectionDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &ConnectionDialog::connectionSubmitted, this,
            [this, dialog](const ConnectionProfile &profile) {
        m_connectionStore.addConnection(profile);
        if (!m_connectionStore.save()) {
            m_connectionStore.removeConnectionAt(m_connectionStore.connections().size() - 1);
            return;
        }
        reloadConnections();
        dialog->accept();
    });
    dialog->open();
}
