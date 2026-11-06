#include "introwindow.h"
#include "../delegates/connectiondelegate.h"
#include "../widgets/statuscard.h"
#include "../../utils/theme.h"

#include <QCoreApplication>
#include "../dialogs/connectiondialog.h"
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QStandardItemModel>
#include <QStackedWidget>
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

IntroWindow::IntroWindow(QWidget *parent) : AppWindow(parent)
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
    auto *outerLayout = new QVBoxLayout(central);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    auto *layout = new QHBoxLayout;
    outerLayout->addLayout(layout, 1);
#if defined(Q_OS_MACOS)
    central->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
#endif
    layout->setContentsMargins(
            WindowContentMargin, WindowTopMargin, WindowContentMargin, WindowContentMargin);

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
    m_createButton = createBtn;
    createBtn->setText(tr("New Connection"));
    connect(createBtn, &QPushButton::clicked, this, &IntroWindow::createConnection);
    auto *connectBtn = new QPushButton(tr("Connect"), this);
    m_connectButton = connectBtn;
    connectBtn->setEnabled(false);
    connect(connectBtn, &QPushButton::clicked, this, &IntroWindow::openSelectedConnection);
    auto *editBtn = new QPushButton(tr("Edit"), this);
    m_editButton = editBtn;
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->hide();
    connect(m_cancelButton, &QPushButton::clicked, this, &IntroWindow::connectionCancelled);
    editBtn->setEnabled(false);
    connect(editBtn, &QPushButton::clicked, this, &IntroWindow::editConnection);

    auto *rightPanel = new QWidget(this);
    auto *rightPanelLayout = new QVBoxLayout(rightPanel);
    rightPanelLayout->setContentsMargins(
            RightPanelLeftMargin, RightPanelTopMargin, RightPanelMargin, RightPanelMargin);
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
    m_connectionsView = connectionsView;
#if defined(Q_OS_WIN)
    connectionsView->setFrameShape(QFrame::NoFrame);
    connectionsView->setAutoFillBackground(false);
    connectionsView->viewport()->setAutoFillBackground(false);
#endif
    connectionsView->setMouseTracking(true);
    connectionsView->setModel(filterModel);
    connect(connectionsView->selectionModel(), &QItemSelectionModel::selectionChanged, editBtn,
            [this, connectionsView, editBtn, connectBtn] {
                const bool hasSelection =
                        !m_connecting && connectionsView->selectionModel()->hasSelection();
                editBtn->setEnabled(hasSelection);
                connectBtn->setEnabled(hasSelection);
            });
    connectionsView->installEventFilter(this);
    connect(connectionsView, &QListView::doubleClicked, this, &IntroWindow::openSelectedConnection);
    connectionsView->setItemDelegate(new ConnectionDelegate(connectionsView));
    connectionsView->setUniformItemSizes(true);
    connectionsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connectionsView->setSelectionMode(QAbstractItemView::SingleSelection);
    connectionsView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connectionsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connectionsView->setSpacing(ConnectionSpacing);
    connect(searchInput, &QLineEdit::textChanged, filterModel,
            &QSortFilterProxyModel::setFilterFixedString);

    auto *connectionsArea = new QStackedWidget(rightPanel);
    connectionsArea->addWidget(connectionsView);

    auto *emptyState = new QWidget(connectionsArea);
    auto *emptyLayout = new QVBoxLayout(emptyState);
    emptyLayout->setSpacing(12);

    auto *emptyLabel = new QLabel(tr("No connections yet"), emptyState);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);
    Theme::applyFor(emptyLabel, Theme::FontRole::Default, Theme::ColorRole::Muted);

    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch();
    connectionsArea->addWidget(emptyState);

    const auto updateEmptyState = [this, connectionsArea, connectionsView, emptyState] {
        connectionsArea->setCurrentWidget(
                m_connectionsModel->rowCount() == 0 ? emptyState : connectionsView);
    };
    connect(m_connectionsModel, &QAbstractItemModel::rowsInserted, connectionsArea,
            updateEmptyState);
    connect(m_connectionsModel, &QAbstractItemModel::rowsRemoved, connectionsArea,
            updateEmptyState);
    connect(m_connectionsModel, &QAbstractItemModel::modelReset, connectionsArea, updateEmptyState);
    updateEmptyState();

    rightPanelLayout->addWidget(searchInput);
    rightPanelLayout->addWidget(connectionsArea, 1);
    m_status = new StatusCard(central);
    outerLayout->addWidget(m_status);

    leftPanelLayout->addWidget(imageLbl, 0, Qt::AlignHCenter);
    leftPanelLayout->addWidget(titleLbl, 0, Qt::AlignHCenter);
    leftPanelLayout->addWidget(versionLbl, 0, Qt::AlignHCenter);
    leftPanelLayout->addStretch(1);
    leftPanelLayout->addWidget(connectBtn);
    leftPanelLayout->addWidget(m_cancelButton);
    leftPanelLayout->addWidget(editBtn);
    leftPanelLayout->addSpacing(12);
    leftPanelLayout->addWidget(createBtn);

    layout->addWidget(leftPanel);
    layout->addWidget(rightPanel, 1);

    setCentralWidget(central);
}

bool IntroWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_connectionsView && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!keyEvent->isAutoRepeat())
                openSelectedConnection();
            return true;
        }
    }
    return AppWindow::eventFilter(watched, event);
}

void IntroWindow::openSelectedConnection()
{
    if (m_connecting)
        return;
    const auto selected = m_connectionsView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return;
    const auto id = selected.first().data(ConnectionIdRole).toUuid();
    for (const auto &profile : m_connectionStore.connections()) {
        if (profile.id == id) {
            emit connectionRequested(profile);
            return;
        }
    }
}

void IntroWindow::setConnecting(bool connecting)
{
    m_connecting = connecting;
    const bool selected = m_connectionsView->selectionModel()->hasSelection();
    m_connectButton->setEnabled(!connecting && selected);
    m_connectButton->setText(connecting ? tr("Connecting…") : tr("Connect"));
    m_editButton->setEnabled(!connecting && selected);
    m_createButton->setEnabled(!connecting);
    m_connectionsView->setEnabled(!connecting);
    m_cancelButton->setVisible(connecting);
    if (connecting)
        m_status->clear();
}

void IntroWindow::showConnectionError(const QString &details)
{
    m_status->setStatus(tr("Could not connect."), Theme::ColorRole::Error, details);
}

void IntroWindow::reloadConnections()
{
    m_connectionsModel->clear();
    for (const auto &connection : m_connectionStore.connections()) {
        const QString address = QStringLiteral("%1:%2/%3")
                                        .arg(connection.host)
                                        .arg(connection.port)
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
                    m_connectionStore.removeConnectionAt(
                            m_connectionStore.connections().size() - 1);
                    dialog->completeSave(false);
                    return;
                }
                reloadConnections();
                dialog->completeSave(true);
            });
    dialog->open();
}

void IntroWindow::editConnection()
{
    const auto selected = m_connectionsView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return;
    const auto id = selected.first().data(ConnectionIdRole).toUuid();
    for (const auto &connection : m_connectionStore.connections()) {
        if (connection.id != id)
            continue;
        // The asynchronous callback below captures the original by value for rollback.
        const auto &original = connection;
        auto *dialog = new ConnectionDialog(this, &original);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &ConnectionDialog::connectionSubmitted, this,
                [this, dialog, original](const ConnectionProfile &profile) {
                    if (!m_connectionStore.updateConnection(profile)) {
                        dialog->completeSave(false);
                        return;
                    }
                    if (!m_connectionStore.save()) {
                        m_connectionStore.updateConnection(original);
                        dialog->completeSave(false);
                        return;
                    }
                    reloadConnections();
                    const auto *model = m_connectionsView->model();
                    for (int row = 0; row < model->rowCount(); ++row) {
                        const auto index = model->index(row, 0);
                        if (index.data(ConnectionIdRole).toUuid() == profile.id) {
                            m_connectionsView->setCurrentIndex(index);
                            break;
                        }
                    }
                    dialog->completeSave(true);
                });
        dialog->open();
        return;
    }
}
