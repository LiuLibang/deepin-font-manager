#include "views/dfontmgrmainwindow.h"
#include "dfinstallnormalwindow.h"

#include "globaldef.h"
#include "interfaces/dfontmenumanager.h"
#include "utils.h"
#include "views/dfdeletedialog.h"
#include "views/dfontinfodialog.h"
#include "views/dfquickinstallwindow.h"

#include <QHBoxLayout>
#include <QShortcut>
#include <QFileSystemWatcher>
#include <QDBusConnection>

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DFileDialog>
#include <DIconButton>
#include <DLabel>
#include <DLineEdit>
#include <DLog>
#include <DMenu>
#include <DSearchEdit>
#include <DSlider>
#include <DSplitter>
#include <DTitlebar>
#include <DWidgetUtil>
#include <DDesktopServices>
#include <DMessageManager>

#include <unistd.h>

class DFontMgrMainWindowPrivate
{
public:
    DFontMgrMainWindowPrivate(DFontMgrMainWindow *q)
        : settingsQsPtr(new QSettings(QDir(Utils::getConfigPath()).filePath("config.conf"),
                                      QSettings::IniFormat))
        , q_ptr(q)
    {
    }

    //~DFontMgrMainWindowPrivate() {}
    QWidget *titleActionArea {nullptr};
    DIconButton *addFontButton {nullptr};
    DSearchEdit *searchFontEdit {nullptr};

    QWidget *fontShowArea {nullptr};

    //Shadow line of StateBar
    DHorizontalLine  *sbarShadowLine {nullptr};

    QWidget *stateBar {nullptr};
    DLineEdit *textInputEdit {nullptr};
    DSlider *fontScaleSlider {nullptr};
    DLabel *fontSizeLabel {nullptr};

    DSplitter *mainWndSpliter {nullptr};
    QWidget *leftBarHolder {nullptr};
    QWidget *rightViewHolder {nullptr};

    // Menu
    DMenu *toolBarMenu {nullptr};
    DMenu *rightKeyMenu {nullptr};

    DSplitListWidget *leftSiderBar {nullptr};

    QScopedPointer<QSettings> settingsQsPtr;
    DFontMgrMainWindow *q_ptr;

    Q_DECLARE_PUBLIC(DFontMgrMainWindow)
};

DFontMgrMainWindow::DFontMgrMainWindow(bool isQuickMode, QWidget *parent)
    : DMainWindow(parent)
    , m_isQuickMode(isQuickMode)
    , m_fontManager(DFontManager::instance())
    , m_scFullScreen(nullptr)
    , m_scZoomIn(nullptr)
    , m_scZoomOut(nullptr)
    , m_scDefaultSize(nullptr)
    , m_previewFontSize(DEFAULT_FONT_SIZE)
    , m_previewText(QString()) //用户输入的预览
    , m_quickInstallWnd(nullptr)
    , d_ptr(new DFontMgrMainWindowPrivate(this))
{
    // setWindoDSpinnerwOpacity(0.5); //Debug
    // setWindowFlags(windowFlags() | (Qt::FramelessWindowHint | Qt::WindowMaximizeButtonHint));

    initData();
    initUI();
    initConnections();
    initShortcuts();
    initFontFiles();
}

DFontMgrMainWindow::~DFontMgrMainWindow()
{
    d_func()->settingsQsPtr->setValue(FTM_MWSIZE_H_KEY, m_winHight);
    d_func()->settingsQsPtr->setValue(FTM_MWSIZE_W_KEY, m_winWidth);
    d_func()->settingsQsPtr->setValue(FTM_MWSTATUS_KEY, m_IsWindowMax);
    //ut000442 bug33870 偶现关闭窗口后,因没有取消dbus服务注册导致的应用无法启动的问题,在这里进行取消
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService("com.deepin.FontManager");
}

void DFontMgrMainWindow::initData()
{
    D_D(DFontMgrMainWindow);
    //Initialize app Theme
    QVariant theme;
    theme = d->settingsQsPtr->value(FTM_THEME_KEY);

    bool ok = false;
    int color = theme.toInt(&ok);

    DGuiApplicationHelper::ColorType colorType = DGuiApplicationHelper::ColorType::UnknownType;

    if (ok) {
        colorType = static_cast<DGuiApplicationHelper::ColorType>(color);
    }

    m_winHight = d->settingsQsPtr->value(FTM_MWSIZE_H_KEY).toInt();
    m_winWidth = d->settingsQsPtr->value(FTM_MWSIZE_W_KEY).toInt();
    m_IsWindowMax = d->settingsQsPtr->value(FTM_MWSTATUS_KEY).toBool();
    qDebug() << __FUNCTION__ << "init theme = " << colorType;

    DGuiApplicationHelper::instance()->setPaletteType(colorType);
}

void DFontMgrMainWindow::initUI()
{
    //Enable main window accept drag event
    setAcceptDrops(true);
//    m_loadingSpinner = new DSpinner(this);
//    m_loadingSpinner->setFixedSize(32, 32);
//    m_loadingSpinner->hide();
    initTileBar();
    initRightKeyMenu();
    initMainVeiws();
}

void DFontMgrMainWindow::initConnections()
{
    D_D(DFontMgrMainWindow);

    // Loading Font List Signal
    QObject::connect(m_fontPreviewListView, SIGNAL(onLoadFontsStatus(int)),
                     this, SLOT(onLoadStatus(int)));

    connect(m_fontPreviewListView, &DFontPreviewListView::rowCountChanged, this,
            &DFontMgrMainWindow::onFontListViewRowCountChanged, Qt::UniqueConnection);

    connect(m_fontPreviewListView, &DFontPreviewListView::deleteFinished, this, [ = ]() {
        setDeleteFinish();
    });

    // Add Font button event
    QObject::connect(d->addFontButton, &DIconButton::clicked, this,
                     &DFontMgrMainWindow::handleAddFontEvent);

    QObject::connect(this, &DFontMgrMainWindow::fileSelected, this,
    [this](const QStringList & files) {
        this->installFont(files);
    });

    QObject::connect(this, &DFontMgrMainWindow::fileSelectedInSys, this,
    [this](const QStringList & files) {
        this->installFontFromSys(files);
    });

    // Menu event
    QObject::connect(d->toolBarMenu, &QMenu::triggered, this, &DFontMgrMainWindow::handleMenuEvent);

    //Theme change event
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::paletteTypeChanged,
    [this](DGuiApplicationHelper::ColorType type) {
        qDebug() << "Update Theme type:" << type;
        //Save theme value
        d_func()->settingsQsPtr->setValue(FTM_THEME_KEY, type);
    });

    // Right Key menu
    QObject::connect(d->rightKeyMenu, &QMenu::triggered, this,
                     &DFontMgrMainWindow::handleMenuEvent);

    // Initialize rigth menu it state
    QObject::connect(d->rightKeyMenu, &QMenu::aboutToShow, this, [ = ]() {
        qDebug() << __FUNCTION__ << "about toshow";
        DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();
        int cnt = 0;
        int systemCnt = 0;
        int disableCnt = 0;
        int curCnt = 0;
        QModelIndexList indexList;
        m_fontPreviewListView->selectedFonts(&cnt, &systemCnt, &curCnt, &disableCnt, nullptr, nullptr, &indexList, nullptr, &currItemData);

        DFontMenuManager::getInstance()->onRightKeyMenuPopup(currItemData, (cnt > 0), (disableCnt > 0), (curCnt > 0));
        qDebug() << __FUNCTION__ << "about toshow end \n";
    });

    connect(d->rightKeyMenu, &QMenu::aboutToHide, this, [ = ] {
        qDebug() << __FUNCTION__ << "about to hide\n\n";
        m_fontPreviewListView->clearPressState();
    });

    // State bar event
    QObject::connect(d->fontScaleSlider, &DSlider::valueChanged, this, [this, d](int value) {
        m_previewFontSize = value;
        QString fontSizeText;
        fontSizeText.sprintf(FMT_FONT_SIZE, value);
//        d->fontSizeLabel->setText(fontSizeText);
        //调节右下角字体大小显示label显示内容/*UT000539*/
        autoLabelWidth(fontSizeText, d->fontSizeLabel, d->fontSizeLabel->fontMetrics());
        onFontSizeChanged(value);
    });

    // Search text changed
    QObject::connect(d->searchFontEdit, SIGNAL(textChanged(const QString &)), this,
                     SLOT(onSearchTextChanged(const QString &)));

    QObject::connect(d->textInputEdit, SIGNAL(textChanged(const QString &)), this,
                     SLOT(onPreviewTextChanged(const QString &)));
    QObject::connect(d->leftSiderBar, SIGNAL(onListWidgetItemClicked(int)), this,
                     SLOT(onLeftSiderBarItemClicked(int)));

    QObject::connect(m_fontManager, SIGNAL(uninstallFontFinished(const QStringList &)), this,
                     SIGNAL(requestDeleted(const QStringList &)));
    QObject::connect(m_fontManager, &DFontManager::uninstallFcCacheFinish, this, &DFontMgrMainWindow::onUninstallFcCacheFinish);
    QObject::connect(m_signalManager, &SignalManager::showInstallFloatingMessage, this, &DFontMgrMainWindow::onShowMessage);

    //安装结束后刷新字体列表
    connect(m_signalManager, &SignalManager::finishFontInstall, this,
            &DFontMgrMainWindow::onFontInstallFinished);

    connect(m_signalManager, &SignalManager::closeInstallDialog, this, [ = ] {
//        if (m_dfNormalInstalldlg->isVisible())
//        {
//            m_dfNormalInstalldlg->deleteLater();
//        }

//        showSpinner(DFontSpinnerWidget::Load);

    });

    connect(m_fontManager, &DFontManager::cacheFinish, this, [ = ] {
        m_cacheFinish = true;
        hideSpinner();
    });

    connect(m_signalManager, &SignalManager::requestInstallAdded, this, [ = ] {
        m_installFinish = true;
        hideSpinner();
    });

    //调节右下角字体大小显示label显示内容/*UT000539*/
    connect(qApp, &DApplication::fontChanged, this, [ = ]() {
        int size = d->fontScaleSlider->value();
        QString fontSize = QString::number(size) + "px";
        autoLabelWidth(fontSize, d->fontSizeLabel, d->fontSizeLabel->fontMetrics());
        m_fontPreviewListView->onFontChanged(qApp->font());
    });

    connect(m_signalManager, &SignalManager::popInstallErrorDialog, this, [ = ] {
        m_isPopInstallErrorDialog = true;
    });

    connect(m_signalManager, &SignalManager::hideInstallErrorDialog, this, [ = ] {
        m_isPopInstallErrorDialog = false;
    });

    connect(m_signalManager, &SignalManager::installOver, this, [ = ](int successInstallCount) {
        m_isInstallOver = true;
        m_successInstallCount = successInstallCount;

        if (m_dfNormalInstalldlg->isVisible()) {
            m_dfNormalInstalldlg->deleteLater();
        }
        if (successInstallCount > 0) {
            showSpinner(DFontSpinnerWidget::Load);
        } else {
//      成功安装的字体数目为0时,在这里将安装标志位复位
            qDebug() << __func__ << "install finish" << endl;
            m_fIsInstalling = false;
        }

    });

    connect(m_signalManager, &SignalManager::cancelInstall, this, [ = ]() {
        m_isInstallOver = true;
        m_successInstallCount = 0;
        m_fIsInstalling = false;
    });

    /*UT000539 增加slider press聚焦的判断*/
    QObject::connect(d->fontScaleSlider, &DSlider::sliderPressed, [ = ] {
        d->fontScaleSlider->setFocus(Qt::MouseFocusReason);
    });

    connect(m_signalManager, &SignalManager::fontSizeRequestToSlider, this, [ = ] {
//        m_previewFontSize = d->fontScaleSlider->value();
//        if (30 != m_previewFontSize)
//            onFontSizeChanged(m_previewFontSize);
        if (!d->searchFontEdit->text().isEmpty())
        {
            onSearchTextChanged(d->searchFontEdit->text());
        }

        //删除过程中安装字体，删除过后要继续安装
        qDebug() << "m_waitForInstall" << m_waitForInstall;
        waitForInsert();
    });
}

void DFontMgrMainWindow::initShortcuts()
{
    D_D(DFontMgrMainWindow);

    //设置字体放大快捷键
    if (!m_scZoomIn) {
        m_scZoomIn = new QShortcut(this);
        m_scZoomIn->setKey(tr("Ctrl+="));
        m_scZoomIn->setContext(Qt::ApplicationShortcut);
        m_scZoomIn->setAutoRepeat(false);

        connect(m_scZoomIn, &QShortcut::activated, this, [this, d] {
            if (m_previewFontSize < MAX_FONT_SIZE)
            {
                ++m_previewFontSize;
            }
            d->fontScaleSlider->setValue(m_previewFontSize);
        });
    }

    //设置字体缩小快捷键
    if (!m_scZoomOut) {
        m_scZoomOut = new QShortcut(this);
        m_scZoomOut->setKey(tr("Ctrl+-"));
        m_scZoomOut->setContext(Qt::ApplicationShortcut);
        m_scZoomOut->setAutoRepeat(false);

        connect(m_scZoomOut, &QShortcut::activated, this, [this, d] {
            if (m_previewFontSize > MIN_FONT_SIZE)
            {
                --m_previewFontSize;
            }
            d->fontScaleSlider->setValue(m_previewFontSize);
        });
    }

    //设置字体默认大小快捷键
    if (!m_scDefaultSize) {
        m_scDefaultSize = new QShortcut(this);
        m_scDefaultSize->setKey(tr("Ctrl+0"));
        m_scDefaultSize->setContext(Qt::ApplicationShortcut);
        m_scDefaultSize->setAutoRepeat(false);

        connect(m_scDefaultSize, &QShortcut::activated, this, [this, d] {
            m_previewFontSize = DEFAULT_FONT_SIZE;
            d->fontScaleSlider->setValue(DEFAULT_FONT_SIZE);
        });
    }

    //Show shortcut --> Ctrl+Shift+/
    if (nullptr == m_scShowAllSC) {
        m_scShowAllSC = new QShortcut(this);
        m_scShowAllSC->setKey(tr("Ctrl+Shift+/"));
        m_scShowAllSC->setContext(Qt::ApplicationShortcut);
        m_scShowAllSC->setAutoRepeat(false);

        connect(m_scShowAllSC, &QShortcut::activated, this, [this] {
            this->showAllShortcut();
        });
    }

    //Show previous page --> PageUp
    if (nullptr == m_scPageUp) {
        m_scPageUp = new QShortcut(this);
        m_scPageUp->setKey(tr("PgUp"));
        m_scPageUp->setContext(Qt::ApplicationShortcut);
        m_scPageUp->setAutoRepeat(false);

        connect(m_scPageUp, &QShortcut::activated, this, [this] {;
                                                                 //For: PageUP
                                                                 //Scrolling first visible item to bottom
                                                                 QModelIndex firstVisibleItem = this->m_fontPreviewListView->indexAt(QPoint(3, 3));

                                                                 if (firstVisibleItem.isValid())
    {
        m_fontPreviewListView->scrollTo(firstVisibleItem, QAbstractItemView::PositionAtBottom);
        }
                                                                });
    }

    //Show next page --> PageDown
    if (nullptr == m_scPageDown) {
        m_scDefaultSize = new QShortcut(this);
        m_scDefaultSize->setKey(tr("PgDown"));
        m_scDefaultSize->setContext(Qt::ApplicationShortcut);
        m_scDefaultSize->setAutoRepeat(false);

        connect(m_scDefaultSize, &QShortcut::activated, this, [this] {
            //For: PageDown
            //Scrolling last visible item to top
            QRect visibleRect = m_fontPreviewListView->geometry();

            QModelIndex lastVisibleItem = this->m_fontPreviewListView->indexAt(QPoint(3, visibleRect.height() - 3));
            if (lastVisibleItem.isValid())
            {
                m_fontPreviewListView->scrollTo(lastVisibleItem, QAbstractItemView::PositionAtTop);
            }
        });
    }

    //Resize Window --> Ctrl+Alt+F
    if (nullptr == m_scWndReize) {
        m_scWndReize = new QShortcut(this);
        m_scWndReize->setKey(tr("Ctrl+Alt+F"));
        m_scWndReize->setContext(Qt::ApplicationShortcut);
        m_scWndReize->setAutoRepeat(false);

        connect(m_scWndReize, &QShortcut::activated, this, [this] {
            if (this->windowState() & Qt::WindowMaximized)
            {
                this->showNormal();
            } else if (this->windowState() == Qt::WindowNoState)
            {
                this->showMaximized();
            }
        });
    }

    //Find font --> Ctrl+F
    if (nullptr == m_scFindFont) {
        m_scFindFont = new QShortcut(this);
        m_scFindFont->setKey(tr("Ctrl+F"));
        m_scFindFont->setContext(Qt::ApplicationShortcut);
        m_scFindFont->setAutoRepeat(false);

        connect(m_scFindFont, &QShortcut::activated, this, [d] {
            d->searchFontEdit->lineEdit()->setFocus(Qt::MouseFocusReason);
        });
    }

    //Delete font --> Delete
    if (nullptr == m_scDeleteFont) {
        m_scDeleteFont = new QShortcut(this);
        m_scDeleteFont->setKey(Qt::Key_Delete);
        m_scDeleteFont->setContext(Qt::ApplicationShortcut);
        m_scDeleteFont->setAutoRepeat(false);

        connect(m_scDeleteFont, &QShortcut::activated, this, [this] {
            //Only can't delete user font
            //first disable delete
            if (m_cacheFinish || m_installFinish)
                return;
            delCurrentFont();
        }, Qt::UniqueConnection);
    }

    //Add Font --> Ctrl+O
    if (nullptr == m_scAddNewFont) {
        m_scAddNewFont = new QShortcut(this);
        m_scAddNewFont->setKey(tr("Ctrl+O"));
        m_scAddNewFont->setContext(Qt::ApplicationShortcut);
        m_scAddNewFont->setAutoRepeat(false);

        connect(m_scAddNewFont, &QShortcut::activated, this, [d] {
            d->addFontButton->click();
        });
    }

    //Add or cancel favorite --> .
    if (nullptr == m_scAddOrCancelFavFont) {
        m_scAddOrCancelFavFont = new QShortcut(this);
        m_scAddOrCancelFavFont->setKey(/*tr(".")*/Qt::Key_Period);
        m_scAddOrCancelFavFont->setContext(Qt::ApplicationShortcut);
        m_scAddOrCancelFavFont->setAutoRepeat(false);

        connect(m_scAddOrCancelFavFont, &QShortcut::activated, this, [ = ] {
            QModelIndexList allItemIndexes;
            DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();
            m_fontPreviewListView->selectedFonts(nullptr, nullptr, nullptr, nullptr, nullptr, &allItemIndexes, nullptr, nullptr, &currItemData);
            if (!m_fontPreviewListView->isVisible() || allItemIndexes.count() == 0)
                return;

            if (m_fontPreviewListView->m_rightMenu->isVisible())
                m_fontPreviewListView->m_rightMenu->close();

            m_fontPreviewListView->onCollectBtnClicked(allItemIndexes, !currItemData.isCollected,
                                                       filterGroup == DSplitListWidget::FontGroup::CollectFont);
        });
    }

    //Font information --> CTRL+I
    if (nullptr == m_scFontInfo) {
        m_scFontInfo = new QShortcut(this);
        m_scFontInfo->setKey(tr("CTRL+I"));
        m_scFontInfo->setContext(Qt::ApplicationShortcut);
        m_scFontInfo->setAutoRepeat(false);

        connect(m_scFontInfo, &QShortcut::activated, this, [this] {
            DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();
            if (m_fontPreviewListView->selectionModel()->selectedIndexes().length() < 1)
            {
                return ;
            }
            if (!currItemData.fontInfo.filePath.isEmpty())
            {
                QAction *fontInfoAction = DFontMenuManager::getInstance()->getActionByMenuAction(
                    DFontMenuManager::M_FontInfo, DFontMenuManager::MenuType::RightKeyMenu);
                fontInfoAction->trigger();
            }
        });
    }
}

void DFontMgrMainWindow::initFontFiles()
{
    /* Bug#19657 sudo执行后，QDir::homePath路径不再是/home/user,导致新建数据库，
     * 与fc-list查出来的数据差异，导致下面的代码删除了系统字体 UT000591  */
    if (geteuid() == 0) {
        return;
    }

    m_dbManager = DFMDBManager::instance();
    DFontInfoManager *fontInfoMgr = DFontInfoManager::instance();

    QStringList allFontsList = fontInfoMgr->getAllFontPath();

    QStringList installFont = m_dbManager->getInstalledFontsPath();

    for (QString filePath : allFontsList) {
        if (!m_dbManager->isSystemFont(filePath) && !installFont.contains(filePath)) {
            QFileInfo openFile(filePath);
            if (!QFile::remove(filePath))
                continue;

            QDir fileDir(openFile.path());
            if (fileDir.isEmpty()) {
                fileDir.removeRecursively();
            }
        }
    }
}

void DFontMgrMainWindow::initTileBar()
{
    D_D(DFontMgrMainWindow);

    initTileFrame();

    d->toolBarMenu = DFontMenuManager::getInstance()->createToolBarSettingsMenu();

    bool isDXcbPlatform = true;

    if (isDXcbPlatform) {
        // d->toolbar->getSettingsButton()->hide();
        titlebar()->setMenu(d->toolBarMenu);
        titlebar()->setContentsMargins(0, 0, 0, 0);

        titlebar()->setFixedHeight(FTM_TITLE_FIXED_HEIGHT);
    }
}

void DFontMgrMainWindow::initTileFrame()
{
    D_D(DFontMgrMainWindow);

    //Add logo
    titlebar()->setIcon(QIcon::fromTheme(DEEPIN_FONT_MANAGER));

    //Action area add a extra space
    d->titleActionArea = new QWidget(this);
    d->titleActionArea->setFixedSize(QSize(58, FTM_TITLE_FIXED_HEIGHT));

    QHBoxLayout *titleActionAreaLayout = new QHBoxLayout(d->titleActionArea);
    titleActionAreaLayout->setSpacing(0);
    titleActionAreaLayout->setContentsMargins(0, 0, 0, 0);

    // Add Font
    d->addFontButton = new DIconButton(DStyle::StandardPixmap::SP_IncreaseElement, this);
    d->addFontButton->setFixedSize(QSize(38, 38));
    d->addFontButton->setFlat(false);
    d->addFontButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);

    titleActionAreaLayout->addWidget(d->addFontButton);

    // Search font
    d->searchFontEdit = new DSearchEdit(this);
    DFontSizeManager::instance()->bind(d->searchFontEdit, DFontSizeManager::T6);
    d->searchFontEdit->setFixedSize(QSize(FTM_SEARCH_BAR_W, FTM_SEARCH_BAR_H));
    d->searchFontEdit->setPlaceHolder(DApplication::translate("SearchBar", "Search"));

    titlebar()->addWidget(d->searchFontEdit, Qt::AlignCenter);
    titlebar()->addWidget(d->titleActionArea, Qt::AlignLeft | Qt::AlignVCenter);


    // Debug layout code
#ifdef FTM_DEBUG_LAYOUT_COLOR
    d->titleActionArea->setStyleSheet("background: red");
    d->addFontButton->setStyleSheet("background: silver");
    d->searchFontEdit->setStyleSheet("background: yellow");
#endif
}

void DFontMgrMainWindow::initMainVeiws()
{
    D_D(DFontMgrMainWindow);
    setWindowIcon(QIcon::fromTheme(DEEPIN_FONT_MANAGER));

    d->mainWndSpliter = new DSplitter(Qt::Horizontal, this);
    m_fontLoadingSpinner = new DFontSpinnerWidget(this);
//    m_fontDeletingSpinner = new DFontSpinnerWidget(this);
    // For Debug
    // d->mainWndSpliter->setStyleSheet("QSplitter::handle { background-color: red }");

    initLeftSideBar();
    initRightFontView();

    //Disable spliter drag & resize
    QSplitterHandle *handle = d->mainWndSpliter->handle(1);
    if (handle) {
        handle->setFixedWidth(2);
        handle->setDisabled(true);

        DPalette pa = DApplicationHelper::instance()->palette(handle);
        QBrush splitBrush = pa.brush(DPalette::ItemBackground);
        pa.setBrush(DPalette::Background, splitBrush);
        handle->setPalette(pa);
        handle->setBackgroundRole(QPalette::Background);
        handle->setAutoFillBackground(true);
    }

    setCentralWidget(d->mainWndSpliter);
}

void DFontMgrMainWindow::initLeftSideBar()
{
    D_D(DFontMgrMainWindow);

    d->leftBarHolder = new QWidget(d->mainWndSpliter);
    d->leftBarHolder->setObjectName("leftMainLayoutHolder");
    d->leftBarHolder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->leftBarHolder->setFixedWidth(FTM_LEFT_SIDE_BAR_WIDTH);
    d->leftBarHolder->setContentsMargins(0, 0, 2, 0);
    d->leftBarHolder->setBackgroundRole(DPalette::Base);
    d->leftBarHolder->setAutoFillBackground(true);
    // d->leftBarHolder->setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout *leftMainLayout = new QVBoxLayout();
    leftMainLayout->setContentsMargins(0, 0, 0, 0);
    leftMainLayout->setSpacing(0);

    // ToDo:
    //    Need use the custom QListView replace QListWidget
    d->leftSiderBar = new DSplitListWidget(this);
    // leftSiderBar->setAttribute(Qt::WA_TranslucentBackground, true);
    d->leftSiderBar->setFrameShape(DFrame::NoFrame);
    d->leftSiderBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftMainLayout->addSpacing(10);
    leftMainLayout->addWidget(d->leftSiderBar);
    d->leftBarHolder->setLayout(leftMainLayout);

    d->leftSiderBar->setFocus();

    // Debug layout code
#ifdef FTM_DEBUG_LAYOUT_COLOR
    d->leftBarHolder->setStyleSheet("background: blue");
    d->leftSiderBar->setStyleSheet("background: yellow");
#endif
}

void DFontMgrMainWindow::initRightFontView()
{
    Q_D(DFontMgrMainWindow);

    // initialize state bar
    initStateBar();

    d->rightViewHolder = new QWidget(d->mainWndSpliter);
    d->rightViewHolder->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    d->rightViewHolder->setObjectName("rightMainLayoutHolder");
    d->rightViewHolder->setBackgroundRole(DPalette::Base);
    d->rightViewHolder->setAutoFillBackground(true);

    QVBoxLayout *rightMainLayout = new QVBoxLayout();
    rightMainLayout->setContentsMargins(0, 0, 0, 0);
    rightMainLayout->setSpacing(0);

    d->fontShowArea = new QWidget(this);
    //d->fontShowArea->setFrameShape(DFrame::NoFrame);
    d->fontShowArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    initFontPreviewListView(d->fontShowArea);

    // initialize statebar shadow line
    d->sbarShadowLine = new DHorizontalLine(this);
    d->sbarShadowLine->setFixedHeight(1);
    d->sbarShadowLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    rightMainLayout->addWidget(d->fontShowArea);
    rightMainLayout->addWidget(d->sbarShadowLine);
    rightMainLayout->addWidget(d->stateBar);

    d->rightViewHolder->setLayout(rightMainLayout);

    // Debug layout code
#ifdef FTM_DEBUG_LAYOUT_COLOR
    d->fontShowArea->setStyleSheet("background: blue");
    m_fontPreviewListView->setStyleSheet("background: green");
    d->rightViewHolder->setStyleSheet("background: red");
#endif
}

//初始化字体预览ListView
void DFontMgrMainWindow::initFontPreviewListView(QWidget *parent)
{
    Q_D(DFontMgrMainWindow);

    QVBoxLayout *listViewVBoxLayout = new QVBoxLayout();
    listViewVBoxLayout->setMargin(0);
    listViewVBoxLayout->setContentsMargins(0, 0, 0, 0);
    listViewVBoxLayout->setSpacing(0);

    parent->setLayout(listViewVBoxLayout);

    m_fontPreviewListView = new DFontPreviewListView(this);
    m_fontPreviewListView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_fontPreviewListView->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    m_fontPreviewListView->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

    m_fontPreviewListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_fontPreviewListView->setRightContextMenu(d->rightKeyMenu);

    listViewVBoxLayout->addWidget(m_fontPreviewListView);

    // 加载图标
//    DLabel *onLoadingSpinner = new DLabel(m_fontLoadingSpinner);
//    onLoadingSpinner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
//    onLoadingSpinner->setFixedHeight(onLoadingSpinner->fontMetrics().height());
//    onLoadingSpinner->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

//    QVBoxLayout *lblLayoutLoad = new QVBoxLayout;
//    lblLayoutLoad->addWidget(onLoadingSpinner);

//    m_fontLoadingSpinner->setLayout(lblLayoutLoad);
    listViewVBoxLayout->addWidget(m_fontLoadingSpinner);

    m_fontLoadingSpinner->spinnerStart();
    m_fontPreviewListView->hide();
    d->stateBar->hide();
    m_fontLoadingSpinner->show();

    // 未搜索到结果view
    m_noResultListView = new DListView(this);

    DLabel *noResultLabel = new DLabel(m_noResultListView);
    noResultLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    noResultLabel->setFixedHeight(noResultLabel->fontMetrics().height() + 30);
    noResultLabel->setText(DApplication::translate("SearchBar", "No search results"));

    QFont labelFont = noResultLabel->font();
    labelFont.setWeight(QFont::ExtraLight);
    noResultLabel->setFont(labelFont);
    noResultLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    DFontSizeManager::instance()->bind(noResultLabel, DFontSizeManager::T4);

    QVBoxLayout *lblLayout = new QVBoxLayout;
    lblLayout->addWidget(noResultLabel);

    m_noResultListView->setLayout(lblLayout);
    listViewVBoxLayout->addWidget(m_noResultListView);

    m_noResultListView->hide();

    // 未安装字体view
    m_noInstallListView = new DListView(this);

    DLabel *noInstallLabel = new DLabel(m_noInstallListView);
    noInstallLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    noInstallLabel->setFixedHeight(noInstallLabel->fontMetrics().height() + 30);
    noInstallLabel->setText(DApplication::translate("SearchBar", "No fonts"));

    QFont labelFontNoInstall = noInstallLabel->font();
    labelFontNoInstall.setWeight(QFont::ExtraLight);
    noInstallLabel->setFont(labelFontNoInstall);
    noInstallLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    DFontSizeManager::instance()->bind(noInstallLabel, DFontSizeManager::T4);

    QVBoxLayout *lblLayoutNoInstall = new QVBoxLayout;
    lblLayoutNoInstall->addWidget(noInstallLabel);

    m_noInstallListView->setLayout(lblLayoutNoInstall);
    listViewVBoxLayout->addWidget(m_noInstallListView);

    m_noInstallListView->hide();
}

void DFontMgrMainWindow::initStateBar()
{
    Q_D(DFontMgrMainWindow);

    QHBoxLayout *stateBarLayout = new QHBoxLayout();
    stateBarLayout->setContentsMargins(0, 0, 0, 0);
    stateBarLayout->setSpacing(0);

    d->stateBar = new QWidget(this);
    //d->stateBar->setFrameShape(DFrame::NoFrame);
    d->stateBar->setFixedHeight(FTM_SBAR_HEIGHT);
    d->stateBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    d->textInputEdit = new DLineEdit(this);
//    QFont searchFont;
//    searchFont.setPixelSize(14);
//    d->textInputEdit->setFont(searchFont);
    //d->textInputEdit->setMinimumSize(QSize(FTM_SBAR_TXT_EDIT_W,FTM_SBAR_TXT_EDIT_H));
    DFontSizeManager::instance()->bind(d->textInputEdit, DFontSizeManager::T6);
    d->textInputEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->textInputEdit->setClearButtonEnabled(true);
    d->textInputEdit->lineEdit()->setPlaceholderText(DApplication::translate("StateBar", "Input preview text"));

    d->fontScaleSlider = new DSlider(Qt::Orientation::Horizontal, this);
    d->fontScaleSlider->setFixedSize(FTM_SBAR_SLIDER_W, FTM_SBAR_SLIDER_H);
    // d->fontScaleSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
//    d->fontScaleSlider->setTracking(true);
//    d->fontScaleSlider->setTickPosition(QSlider::NoTicks);
//    d->fontScaleSlider->setRange(MIN_FONT_SIZE, MAX_FONT_SIZE);
    d->fontScaleSlider->setMinimum(MIN_FONT_SIZE);
    d->fontScaleSlider->setMaximum(MAX_FONT_SIZE);
    //设置初始显示字体大小
    d->fontScaleSlider->setValue(DEFAULT_FONT_SIZE);

    d->fontSizeLabel = new DLabel(this);
    QFont fontScaleFont;
    fontScaleFont.setPixelSize(14);
    d->fontSizeLabel->setFont(fontScaleFont);
    d->fontSizeLabel->setFixedSize(FTM_SBAR_FSIZE_LABEL_W, FTM_SBAR_FSIZE_LABEL_H);
    d->fontSizeLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

//    QFont fontSize;
//    fontSize.setPixelSize(14);
//    d->fontSizeLabel->setFont(fontSize);
    DFontSizeManager::instance()->bind(d->fontSizeLabel, DFontSizeManager::T6);
    // d->fontSizeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // Init the default font size
    QString defaultFontSize;
    defaultFontSize.sprintf(FMT_FONT_SIZE, DEFAULT_FONT_SIZE);

    //调节右下角字体大小显示label显示内容/*UT000539*/
    autoLabelWidth(defaultFontSize, d->fontSizeLabel, d->fontSizeLabel->fontMetrics());
//    d->fontSizeLabel->setText(defaultFontSize);

    stateBarLayout->addSpacing(10);
    stateBarLayout->addWidget(d->textInputEdit, 1);
    stateBarLayout->addSpacing(20);
    stateBarLayout->addWidget(d->fontScaleSlider);
    stateBarLayout->addSpacing(10);
    stateBarLayout->addWidget(d->fontSizeLabel);
    stateBarLayout->addSpacing(20);

    d->stateBar->setLayout(stateBarLayout);

    // Debug layout code
#ifdef FTM_DEBUG_LAYOUT_COLOR
    d->stateBar->setStyleSheet("background: green");
    d->textInputEdit->setStyleSheet("background: blue");
    d->fontScaleSlider->setStyleSheet("background: yellow");
    d->fontSizeLabel->setStyleSheet("background: yellow");
#endif
}

void DFontMgrMainWindow::handleAddFontEvent()
{
    Q_D(DFontMgrMainWindow);
    DFileDialog dialog;
    dialog.setFileMode(DFileDialog::ExistingFiles);
    dialog.setNameFilter(Utils::suffixList());

    QString historyDir = d->settingsQsPtr->value("dir").toString();
    if (historyDir.isEmpty()) {
        historyDir = QDir::homePath();
    }
    if (!mhistoryDir.isEmpty()) {
        dialog.setDirectory(mhistoryDir);
    } else {
        dialog.setDirectory(historyDir);
    }

    m_fontPreviewListView->refreshFocuses();
    const int mode = dialog.exec();

    // save the directory string to config file.
    d->settingsQsPtr->setValue("dir", dialog.directoryUrl().toLocalFile());

    // if click cancel button or close button.
    if (mode != QDialog::Accepted) {
        return;
    }

    QStringList filelist = dialog.selectedFiles();
    if (filelist.count() > 0) {
        mhistoryDir.clear();
        QStringList strlist;
        strlist = filelist.at(0).split("/");
        for (int i = 0; i < strlist.count(); i++) {
            if (i == 0) {
                mhistoryDir += strlist[i];
            } else  if (i == strlist.count() - 1) {

            } else {
                mhistoryDir += "/" + strlist[i];
            }
        }
    }

    m_previewText = d->textInputEdit->text();
    Q_EMIT fileSelected(filelist);
}

void DFontMgrMainWindow::handleMenuEvent(QAction *action)
{
    if (action->data().isValid()) {
        bool ok = false;
        int type = action->data().toInt(&ok);

        if (ok) {
            DFontMenuManager::MenuAction actionId = static_cast<DFontMenuManager::MenuAction>(type);

            // Add menu handler code here
            switch (actionId) {
            case DFontMenuManager::MenuAction::M_AddFont: {
                handleAddFontEvent();
            }
            break;
            case DFontMenuManager::MenuAction::M_FontInfo: {
                DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();
                DFontInfoDialog *fontInfoDlg = new DFontInfoDialog(&currItemData, this);
                fontInfoDlg->exec();
            }
            break;
            case DFontMenuManager::MenuAction::M_DeleteFont: {
                delCurrentFont();
            }
            break;
            case DFontMenuManager::MenuAction::M_ExportFont: {
                exportFont();
            }
            break;
            case DFontMenuManager::MenuAction::M_EnableOrDisable: {
                DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();
                QModelIndexList itemIndexes;
                int systemCnt = 0;
                int currCnt = 0;
                int disableCnt = 0;
                m_fontPreviewListView->selectedFonts(nullptr, &systemCnt, &currCnt, &disableCnt, nullptr, nullptr, &itemIndexes, nullptr, &currItemData);

                m_fontPreviewListView->onEnableBtnClicked(itemIndexes, systemCnt, currCnt, !currItemData.isEnabled,
                                                          filterGroup == DSplitListWidget::FontGroup::ActiveFont);
            }
            break;
            case DFontMenuManager::MenuAction::M_Faverator: {
                DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();
                QModelIndexList itemIndexes;
                m_fontPreviewListView->selectedFonts(nullptr, nullptr, nullptr, nullptr, nullptr, &itemIndexes, nullptr, nullptr, &currItemData);

                m_fontPreviewListView->onCollectBtnClicked(itemIndexes, !currItemData.isCollected,
                                                           filterGroup == DSplitListWidget::FontGroup::CollectFont);
            }
            break;
            case DFontMenuManager::MenuAction::M_ShowFontPostion:
                showFontFilePostion();
                break;
            default:
                qDebug() << "handleMenuEvent->(id=" << actionId << ")";
                break;
            }
        }
    }
}

// return value: true: install success, false: install fail
bool DFontMgrMainWindow::installFont(const QStringList &files)
{
    QStringList m_installFiles = checkFilesSpace(files);
    m_abandonFilesCount = files.count() - m_installFiles.count();
    if (m_installFiles.count() == 0) {
        emit m_signalManager->showInstallFloatingMessage(0);
        return false;
    }

    if (m_fIsInstalling) {
        qDebug() << "Already exist a installtion flow";
        return false;
    }

    m_fontPreviewListView->clearSelection();
    qDebug() << "installFont new DFInstallNormalWindow " << endl;
    m_dfNormalInstalldlg = new DFInstallNormalWindow(m_installFiles, this);
    emit m_signalManager->setSpliteWidgetScrollEnable(true);//开始安装
    if (m_isQuickMode) {
        m_dfNormalInstalldlg->setSkipException(true);
    }

    //Set installtion flag
    /*
     * Add font from + ,menu, drag file to main view
     * to task bar can start a installtion flow, so must
     * to set flag avoid
     */
    m_fIsInstalling = true;

    Dtk::Widget::moveToCenter(m_dfNormalInstalldlg);
    m_dfNormalInstalldlg->exec();
//    m_dfNormalInstalldlg->setModal(true);

    //Clear installtion flag when NormalInstalltion window is closed
    return true;
}

void DFontMgrMainWindow::installFontFromSys(const QStringList &files)
{
    this->m_isFromSys = true;

    QStringList reduceSameFiles;
    foreach (auto it, files) {
        if (!reduceSameFiles.contains(it)) {
            reduceSameFiles.append(it);
        }
    }

    if (!m_fontPreviewListView->isListDataLoadFinished()) {
        qDebug() << "Is loading ,quit";
        m_waitForInstall = reduceSameFiles;
        return;
    } else if (m_fIsDeleting) {
        qDebug() << "Is deleting ,quit";
        m_waitForInstall = reduceSameFiles;
        return;
    } else if (m_isPopInstallErrorDialog) {
        emit m_signalManager->installDuringPopErrorDialog(reduceSameFiles);
    } else {
        installFont(reduceSameFiles);
    }
}

void DFontMgrMainWindow::initRightKeyMenu()
{
    Q_D(DFontMgrMainWindow);

    d->rightKeyMenu = DFontMenuManager::getInstance()->createRightKeyMenu();
}

void DFontMgrMainWindow::setQuickInstallMode(bool isQuick)
{
#ifdef QT_QML_DEBUG
    qDebug() << __FUNCTION__ << " isQuickMode=" << isQuick;
#endif
    m_isQuickMode = isQuick;
}

void DFontMgrMainWindow::hideQucikInstallWindow()
{
    if (m_quickInstallWnd.get() != nullptr) {
        m_quickInstallWnd->setVisible(false);
    }
}

void DFontMgrMainWindow::InitQuickWindowIfNeeded()
{
    if (m_quickInstallWnd.get() == nullptr) {
        m_quickInstallWnd.reset(new DFQuickInstallWindow());

        // Quick install mode handle
        QObject::connect(this, &DFontMgrMainWindow::quickModeInstall, this,
        [this](const QStringList & files) {
            connect(m_quickInstallWnd.get(), &DFQuickInstallWindow::quickInstall, this,
            [this, files]() {
                this->installFont(files);
            });
            m_quickInstallWnd.get()->setWindowModality(Qt::WindowModal);
            m_quickInstallWnd->onFileSelected(files);
            m_quickInstallWnd->show();
            m_quickInstallWnd->raise();       //Reative the window
            m_quickInstallWnd->activateWindow();

            Dtk::Widget::moveToCenter(m_quickInstallWnd.get());
        });
    }
}

void DFontMgrMainWindow::forceNoramlInstalltionQuitIfNeeded()
{
    if (m_fIsInstalling) {
        qDebug() << "In normal installtion flow, force quit!";
        m_dfNormalInstalldlg->breakInstalltion();
    }
}

void DFontMgrMainWindow::setDeleteFinish()
{
    m_fIsDeleting &= ~Delete_Deleting;
    qDebug() << __FUNCTION__ << m_fIsDeleting;
}

void DFontMgrMainWindow::cancelDelete()
{
    m_fIsDeleting = UnDeleting;
}

void DFontMgrMainWindow::onSearchTextChanged(const QString &currStr)
{
    if (!m_fontPreviewListView->isListDataLoadFinished()) {
        return;
    }

//    QString strSearchFontName = currStr;
    const QString strSearchFontName = currStr;
    qDebug() << "SearchFontName:" << strSearchFontName << endl;

    m_searchTextStatusIsEmpty = strSearchFontName.isEmpty();

    DFontPreviewProxyModel *filterModel = m_fontPreviewListView->getFontPreviewProxyModel();

    //根据搜索框内容实时过滤列表
    filterModel->setFilterKeyColumn(0);
    filterModel->setFilterFontNamePattern(strSearchFontName);
//    filterModel->setEditStatus(m_searchTextStatusIsEmpty);

    qDebug() << __FUNCTION__ << "filter Count:" << filterModel->rowCount() << endl;

    onFontListViewRowCountChanged();
    onPreviewTextChanged();
    m_fontPreviewListView->scrollToTop();
}

void DFontMgrMainWindow::onPreviewTextChanged(const QString &text)
{
    m_previewText = text;

    onPreviewTextChanged();
}

void DFontMgrMainWindow::onFontSizeChanged(int fontSize)
{
//    Q_EMIT m_signalManager->refreshCurRect();
    if (!m_fontPreviewListView->isListDataLoadFinished()) {
        return;
    }

    DFontPreviewProxyModel *filterModel = m_fontPreviewListView->getFontPreviewProxyModel();
    qDebug() << __FUNCTION__ << "filter Count:" << filterModel->rowCount() << endl;

    for (int rowIndex = 0; rowIndex < filterModel->rowCount(); rowIndex++) {
        QModelIndex modelIndex = filterModel->index(rowIndex, 0);
        filterModel->setData(modelIndex, QVariant(fontSize), Dtk::UserRole + 2);
//        filterModel->setEditStatus(m_searchTextStatusIsEmpty);
    }
//    Q_EMIT m_signalManager->prevFontChanged();
}

void DFontMgrMainWindow::showFontFilePostion()
{
    DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();

    if (-1 != currItemData.strFontId && currItemData.fontInfo.filePath.length() > 0) {
        DDesktopServices::showFileItem(currItemData.fontInfo.filePath);
    }
}

void DFontMgrMainWindow::onLeftSiderBarItemClicked(int index)
{
    if (!m_fontPreviewListView->isListDataLoadFinished()) {
        //save index to update
        m_leftIndex = index;
        return;
    }


    m_leftIndex = 0;

    qDebug() << __FUNCTION__ << index << endl;
    filterGroup = qvariant_cast<DSplitListWidget::FontGroup>(index);
    emit m_signalManager->currentFontGroup(filterGroup);

    qDebug() << "filterGroup" << filterGroup << endl;

    DFontPreviewProxyModel *filterModel = m_fontPreviewListView->getFontPreviewProxyModel();
    filterModel->setFilterKeyColumn(0);
    filterModel->setFilterGroup(filterGroup);
//    filterModel->setEditStatus(m_searchTextStatusIsEmpty);

    onFontListViewRowCountChanged();
    onPreviewTextChanged();
    m_fontPreviewListView->clearSelection();
}

void DFontMgrMainWindow::onFontInstallFinished(const QStringList &fileList)
{
    Q_D(DFontMgrMainWindow);

    Q_EMIT m_fontPreviewListView->requestAdded(fileList);
    d->textInputEdit->textChanged(d->textInputEdit->text());
    if (!fileList.isEmpty()) {
        showInstalledFiles();
    }
}

void DFontMgrMainWindow::onUninstallFcCacheFinish()
{
    m_fIsDeleting &= ~Delete_Cacheing;
    qDebug() << __FUNCTION__ << m_fIsDeleting;
}

/* 判断FontListView的结果并显示对应状态
 * dShow = 0 :查找到信息，显示正常
 * dShow = 1 :未查到信息，显示“无搜索结果”
 * dShow = 2 :未安装字体，显示“暂无字体”
 * default   :默认有信息，显示正常
 */
void DFontMgrMainWindow::onFontListViewRowCountChanged()
{
    Q_D(DFontMgrMainWindow);

    unsigned int bShow = 0;
    DFontPreviewProxyModel *filterModel = m_fontPreviewListView->getFontPreviewProxyModel();
    if (filterModel == nullptr)
        return;
    if (0 == filterModel->rowCount()) {
        if (m_searchTextStatusIsEmpty) {
            bShow = 2;
        } else {
            bShow = 1; //未找到字体
        }
    }
    bool isSpinnerHidden = m_fontLoadingSpinner->isHidden();
    switch (bShow) {
    case 0:
        if (isSpinnerHidden) {
            m_fontPreviewListView->show();
            m_noResultListView->hide();
            m_isNoResultViewShow = false;
            d->stateBar->show();
            if (m_noInstallListView->isVisible()) {
                m_noInstallListView->hide();
            }
        }
        return;
    case 1:
        if (!m_isNoResultViewShow) {
            if (!m_fIsInstalling) {
                m_fontPreviewListView->hide();
//                QTimer::singleShot(5, [ = ]() {
                m_noResultListView->show();
                m_isNoResultViewShow = true;
//                });
                d->stateBar->hide();
                if (m_noInstallListView->isVisible()) {
                    m_noInstallListView->hide();
                }
            }
            return;
        } else {
            return;
        }

    case 2:
        if (isSpinnerHidden) {
            m_fontPreviewListView->hide();
            d->stateBar->hide();
            if (m_noResultListView->isVisible()) {
                m_noResultListView->hide();
                m_isNoResultViewShow = false;
            }
            d->leftSiderBar->setFocus();
            m_noInstallListView->show();
        }
        return;
    default:
        m_fontPreviewListView->show();
        m_noResultListView->hide();
        m_isNoResultViewShow = false;
        d->stateBar->show();
        break;
    }
}

// 0 正在加载
// 1 完成加载
void DFontMgrMainWindow::onLoadStatus(int type)
{
    D_D(DFontMgrMainWindow);
    if (0 == type || 1 == type) {
        switch (type) {
        case 0:
            m_fontPreviewListView->hide();
            if (m_noResultListView->isVisible()) {
                m_noResultListView->hide();
            }
            m_fontLoadingSpinner->spinnerStop();
            m_fontLoadingSpinner->spinnerStart();
            m_fontLoadingSpinner->show();
            break;
        case 1:
            if (m_fontPreviewListView->isListDataLoadFinished()) {
                m_fontLoadingSpinner->hide();
                m_fontLoadingSpinner->spinnerStop();
            }
            if (m_leftIndex >= 0) {
                onLeftSiderBarItemClicked(m_leftIndex);
            }
            if (!m_noInstallListView->isVisible())//弹出之前判断是否已有无结果view 539 31107
                m_fontPreviewListView->show();
            //不是删除过程造成的安装等待，安装时也需要对相关标志为进行设置
            waitForInsert();
            //第一次打开软件，正在加载数据时，搜索框的内容不为空，为做此操作 ut000794
            if (m_openfirst) {
                if (!d->searchFontEdit->text().isEmpty()) {
                    emit d->searchFontEdit->textChanged(d->searchFontEdit->text());
                }
                m_openfirst = false;
            }
            m_fontPreviewListView->onFontChanged(qApp->font());
            break;
        default:
            break;
        }
    }
//    if (type == 1 && !m_fileList.isEmpty()) {
//        showInstalledFiles(m_fileList);
//    }
}

void DFontMgrMainWindow::onShowMessage(int successCount)
{
    QString messageA;
    QString messageB;

//    if (successCount == 1 && systemFontCount != 0) {
//        messageA = DApplication::translate("Font", "%1 font installed").arg(successCount);
//        DMessageManager::instance()->sendMessage(this, QIcon(":/images/ok.svg"), messageA);
//        messageB = DApplication::translate("Font", "The other %2 system fonts have already been installed").arg(systemFontCount);
//        DMessageManager::instance()->sendMessage(this, QIcon(":/images/exception-logo.svg"), messageB);
//    }
//    if (successCount != 1) {
//        messageA = DApplication::translate("Font", "%1 fonts installed").arg(successCount);
//        DMessageManager::instance()->sendMessage(this, QIcon(":/images/ok.svg"), messageA);
//        if (systemFontCount > 0) {
//            messageB = DApplication::translate("Font", "The other %2 system fonts have already been installed").arg(systemFontCount);
//            DMessageManager::instance()->sendMessage(this, QIcon(":/images/exception-logo.svg"), messageB);
//        }
//    }

    if (successCount == 1) {
        messageA = DApplication::translate("DFontMgrMainWindow", "%1 font installed").arg(successCount);
        DMessageManager::instance()->sendMessage(this, QIcon("://ok.svg"), messageA);

    } else if (successCount > 1) {
        messageA = DApplication::translate("DFontMgrMainWindow", "%1 fonts installed").arg(successCount);
        DMessageManager::instance()->sendMessage(this, QIcon("://ok.svg"), messageA);
    }
}

void DFontMgrMainWindow::onShowSpinner(bool bShow, bool force, DFontSpinnerWidget::SpinnerStyles style)
{
    qDebug() << __FUNCTION__ << bShow << "begin";
    if (bShow) {
        showSpinner(/*DFontSpinnerWidget::Delete*/style, force);
    } else {
        m_fontLoadingSpinner->spinnerStop();
        m_fontLoadingSpinner->hide();

        m_isNoResultViewShow = false;
        onFontListViewRowCountChanged();
        onPreviewTextChanged();
    }
    qDebug() << __FUNCTION__ << bShow << "end";
}

void DFontMgrMainWindow::delCurrentFont()
{
    qDebug() << __FUNCTION__ << m_fIsDeleting;
    if (m_fIsDeleting > UnDeleting)
        return;
    m_fIsDeleting = Deleting;
    int deleteCnt = 0;
    int systemCnt = 0;
    int curCnt = 0;
    QStringList uninstallFonts;
    m_fontPreviewListView->selectedFonts(&deleteCnt, &systemCnt, &curCnt, nullptr, &uninstallFonts);
    if (deleteCnt < 1) {
        m_fIsDeleting = UnDeleting;
        return;
    }

    DFDeleteDialog *confirmDelDlg = new DFDeleteDialog(this, deleteCnt, systemCnt, curCnt > 0, this);

    connect(confirmDelDlg, &DFDeleteDialog::accepted, this, [ = ]() {

        m_fontPreviewListView->markPositionBeforeRemoved(true, QModelIndexList()); //记录移除前位置
        DFontPreviewItemData currItemData = m_fontPreviewListView->currModelData();
        qDebug() << "Confirm delete:" << currItemData.fontInfo.filePath
                 << " is system font:" << currItemData.fontInfo.isSystemFont;
        //force delete all fonts
        //disable file system watcher
        onShowSpinner(true, false, DFontSpinnerWidget::Delete);
        Q_EMIT DFontPreviewListDataThread::instance(m_fontPreviewListView)->requestRemoveFileWatchers(uninstallFonts);
        DFontManager::instance()->setType(DFontManager::UnInstall);
        DFontManager::instance()->setUnInstallFile(uninstallFonts);
        DFontManager::instance()->start();
    });

//    confirmDelDlg->move((this->width() - confirmDelDlg->width() - 230 + mapToGlobal(QPoint(0, 0)).x()), (mapToGlobal(QPoint(0, 0)).y() + 180));
    confirmDelDlg->move(this->geometry().center() - confirmDelDlg->rect().center());
    confirmDelDlg->exec();
}

void DFontMgrMainWindow::exportFont()
{
    int cnt = 0;
    int systemCnt = 0;
    int curCnt = 0;

    QStringList files;
    m_fontPreviewListView->selectedFonts(&cnt, &systemCnt, &curCnt, nullptr, nullptr, nullptr, nullptr, &files);

//    qint64 m_currentDiskSpace = getDiskSpace(false);
//    if (m_currentDiskSpace == 0)
//        return;

    if (curCnt > 0 && !files.contains(m_fontPreviewListView->getCurFontData().fontInfo.filePath))
        files << m_fontPreviewListView->getCurFontData().fontInfo.filePath;

    QStringList m_exportFiles = checkFilesSpace(files, false);
    if (m_exportFiles.count() == 0) {
        showExportFontMessage(0, files.count());
        return;
    }
    if (files.isEmpty())
        return;
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + tr("Fonts") + "/";
    QDir dir(desktopPath);
    if (!dir.exists())
        dir.mkpath(desktopPath);
    for (QString &file : m_exportFiles) {
        QFile::copy(file, desktopPath + QFileInfo(file).fileName());
    }
    showExportFontMessage(m_exportFiles.count(), files.count() - m_exportFiles.count());
}

void DFontMgrMainWindow::showExportFontMessage(int successCount, int abandonFilesCount)
{
    QString message;
    if (abandonFilesCount == 0) {
        if (successCount == 1) {
            message = DApplication::translate("Main", "The font exported to your desktop");
            DMessageManager::instance()->sendMessage(this, QIcon("://ok.svg"), message);
        } else {
            message = DApplication::translate("Main", "%1 fonts exported to your desktop").arg(successCount);
            DMessageManager::instance()->sendMessage(this, QIcon("://ok.svg"), message);
        }
    } else if (abandonFilesCount == 1) {
        message = DApplication::translate("Main", "Failed to export 1 font. There is not enough disk space.");
        DMessageManager::instance()->sendMessage(this, QIcon("://exception-logo.svg"), message);
    } else if (abandonFilesCount > 1) {
        message = DApplication::translate("Main", "Failed to export %1 fonts. There is not enough disk space.").arg(abandonFilesCount);
        DMessageManager::instance()->sendMessage(this, QIcon("://exception-logo.svg"), message);
    }

}

void DFontMgrMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    //Q_D(DFontMgrMainWindow);

    if (event->mimeData()->hasUrls()) {
        QList<QUrl> dragFiles = event->mimeData()->urls();

        if (dragFiles.size() == 1) {
            //For one-drag check MIME,ignore non-font file
            if (Utils::isFontMimeType(dragFiles[0].path())) {
                event->accept();
                return;
            }
        } else {
            //Multi-drag just accept all file at start
            //will filter non-font files in drapEvent
            event->accept();
            return;
        }
    }
    qDebug() << __FUNCTION__ << "ignore";
    event->ignore();
}

void DFontMgrMainWindow::dropEvent(QDropEvent *event)
{

    if (event->mimeData()->hasUrls()) {

        QStringList installFileList;

        QList<QUrl> dragFiles = event->mimeData()->urls();

        if (dragFiles.size() > 1) {
            foreach (auto it, event->mimeData()->urls()) {
                if (Utils::isFontMimeType(it.path())) {
                    installFileList.append(it.path());
                }
            }
        } else {
            if (Utils::isFontMimeType(dragFiles[0].path())) {
                installFileList.append(dragFiles[0].path());
            }
        }

        //Check if need to trigger installtion
        if (installFileList.size() > 0) {
            event->accept();
            Q_EMIT fileSelected(installFileList);
        } else {
            event->ignore();
        }
    } else {
        event->ignore();
    }
}

void DFontMgrMainWindow::resizeEvent(QResizeEvent *event)
{
//    Q_UNUSED(event)

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect =  screen->availableVirtualGeometry();

    if (this->rect() == screenRect) {
        setWindowState(Qt::WindowMaximized);
    }

    if (Qt::WindowMaximized != int(QWidget::windowState())) {
        m_winHight = geometry().height();
        m_winWidth = geometry().width();
        m_IsWindowMax = false;
    } else {
        m_IsWindowMax = true;
    }

    DMainWindow::resizeEvent(event);
}

QString DFontMgrMainWindow::getPreviewTextWithSize(int *fontSize)
{
    if (fontSize != nullptr)
        *fontSize = m_previewFontSize;

    return m_previewText;
}

void DFontMgrMainWindow::showAllShortcut()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    QMap<QString, QString> shortcutKeymap = {
        {DApplication::translate("Shortcut", "Help"), "F1"},
//        {"Zoom in",            "Ctrl+-"},
//        {"Zoom out",           "Ctrl++"},
//        {"Reset font",         "Ctrl+0"},
//        {"Close window",       "Alt+F4"},
        {DApplication::translate("Shortcut", "Display shortcuts"),  "Ctrl+Shift+?"},
        {DApplication::translate("Shortcut", "Page up"), "PageUp"},
        {DApplication::translate("Shortcut", "Page down"), "PageDown"},
//        {"Resize window",      "Ctrl+Alt+F"},
//        {"Find",               "Ctrl+F"},
        {DApplication::translate("Shortcut", "Delete"), "Delete"},
        {DApplication::translate("Shortcut", "Add font"), "Ctrl+O"},
        {DApplication::translate("Shortcut", "Favorite"), "."},
        {DApplication::translate("Shortcut", "Unfavorite"), "."},
        {DApplication::translate("Shortcut", "Font info"), "Ctrl+I"},
    };

    QJsonObject fontMgrJsonGroup;
    fontMgrJsonGroup.insert("groupName", DApplication::translate("Main", "Font Manager"));
    QJsonArray fontJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutKeymap.begin();
            it != shortcutKeymap.end(); it++) {
        QJsonObject jsonItem;
        jsonItem.insert("name", DApplication::translate("Shortcuts", it.key().toUtf8()));
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        fontJsonItems.append(jsonItem);
    }

    fontMgrJsonGroup.insert("groupItems", fontJsonItems);
    jsonGroups.append(fontMgrJsonGroup);

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess *shortcutViewProcess = new QProcess();
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

void DFontMgrMainWindow::showInstalledFiles()
{
    D_D(DFontMgrMainWindow);

    d->leftSiderBar->setCurrentIndex(d->leftSiderBar->model()->index(DSplitListWidget::UserFont, 0));
    onLeftSiderBarItemClicked(DSplitListWidget::UserFont);
}


//通过styles来决定标签显示内容
void DFontMgrMainWindow::showSpinner(DFontSpinnerWidget::SpinnerStyles styles, bool force)
{
    D_D(DFontMgrMainWindow);
    m_noInstallListView->hide();
    m_fontPreviewListView->hide();
    m_noResultListView->hide();
    d->stateBar->hide();

    m_fontLoadingSpinner->setStyles(styles);

    if (force) {
        m_fontLoadingSpinner->spinnerStart();
        m_fontLoadingSpinner->repaint();
        return;
    }
    m_fontLoadingSpinner->show();
    m_fontLoadingSpinner->spinnerStart();
}

void DFontMgrMainWindow::hideSpinner()
{
    if (!m_cacheFinish || !m_installFinish) {
        return;
    }
    //        ut000442 安装少量字体时,会出现闪屏现象,通过加短暂延迟解决.
    QTimer::singleShot(50, this, [ = ]() {
        m_fontLoadingSpinner->spinnerStop();
        m_fontLoadingSpinner->hide();
        m_isNoResultViewShow = false;
        if (m_isInstallOver) {
            emit m_signalManager->showInstallFloatingMessage(m_successInstallCount);
            m_isInstallOver = false;
        }

        emit m_signalManager->setSpliteWidgetScrollEnable(false);//安装刷新完成后启用菜单滚动功能
        m_cacheFinish = false;
        m_installFinish = false;
        qDebug() << __func__ << "install finish" << endl;
        m_fIsInstalling = false;

        onFontListViewRowCountChanged();
        onPreviewTextChanged();
        //安装加载之后之后设置高亮状态以及listview的滚动
        QStringList fontList;
        m_fontPreviewListView->selectedFonts(nullptr, nullptr, nullptr, nullptr, &fontList);
        int count = fontList.count();
        if (1 == count) {
            m_fontPreviewListView->setCurrentSelected(m_fontPreviewListView->selectionModel()->selectedIndexes().first().row());
            //            scrollTo(currentIndex());
            m_fontPreviewListView->scrollTo(m_fontPreviewListView->selectionModel()->selectedIndexes().first());
        } else if (count > 1) {
            if (m_fontPreviewListView->selectionModel()->selectedIndexes().count() > 0) {
                m_fontPreviewListView->setCurrentSelected(m_fontPreviewListView->selectionModel()->selectedIndexes().first().row());
            }

            if (m_fontPreviewListView->selectionModel()->selectedIndexes().count() > 1) {
                m_fontPreviewListView->scrollTo(m_fontPreviewListView->selectionModel()->selectedIndexes().first());
            }
        }

        m_fontPreviewListView->refreshFocuses();
    });
}

void DFontMgrMainWindow::waitForInsert()
{
    if (m_waitForInstall.isEmpty())
        return;

    if (installFont(m_waitForInstall))
        m_waitForInstall.clear();
}

void DFontMgrMainWindow::onPreviewTextChanged()
{
    if (!m_fontPreviewListView->isListDataLoadFinished()) {
        return;
    }

    DFontPreviewProxyModel *filterModel = m_fontPreviewListView->getFontPreviewProxyModel();
    int total = filterModel->rowCount();
    qDebug() << __FUNCTION__ << "filter Count:" << filterModel->rowCount() << endl;

    for (int rowIndex = 0; rowIndex < total; rowIndex++) {
        QModelIndex modelIndex = filterModel->index(rowIndex, 0);
        QString itemPreviewTxt = filterModel->data(modelIndex, Dtk::UserRole + 1).toString();
        if (m_previewText != itemPreviewTxt)
            filterModel->setData(modelIndex, QVariant(m_previewText), Dtk::UserRole + 1);
        if (m_previewFontSize != filterModel->data(modelIndex, Dtk::UserRole + 2).toInt())
            filterModel->setData(modelIndex, QVariant(m_previewFontSize), Dtk::UserRole + 2);
//        filterModel->setEditStatus(m_searchTextStatusIsEmpty);
    }
//    emit m_signalManager->freshListView();
}

qint64 DFontMgrMainWindow::getDiskSpace(bool m_bInstall)
{
    //    QStorageInfo storage = QStorageInfo::root();

    QStorageInfo storage;
    if (m_bInstall) {
        storage = QStorageInfo(QDir::homePath());
        qDebug() << __FUNCTION__ << "storage.bytesAvailable:" << storage.bytesAvailable();
    } else {
        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        storage = QStorageInfo(desktopPath);
    }

    qint64 m_remainSpace = storage.bytesAvailable()/*/1000/1000*/;//不用转换直接用bytes更加准确
    return m_remainSpace;
}

QStringList DFontMgrMainWindow::checkFilesSpace(const QStringList &files, bool m_bInstall)
{
    QStringList m_installFiles;
    qint64 m_totalSelectSpace = 0;
    qint64 m_currentDiskSpace = getDiskSpace(m_bInstall);
    qDebug() << m_currentDiskSpace << endl;
    foreach (auto it, files) {
        QFile file(it);
        if (file.open(QIODevice::ReadWrite) || file.open(QIODevice::ReadOnly)) {
            m_totalSelectSpace = m_totalSelectSpace + file.size();
            if (m_totalSelectSpace >= m_currentDiskSpace) {
                break;
            } else {
                m_installFiles.append(it);
            }
        }
    }

    return m_installFiles;
}

//调节右下角字体大小显示label显示内容/*UT000539*/
void DFontMgrMainWindow::autoLabelWidth(QString text, DLabel *lab, QFontMetrics fm)
{
    QString str = text;
    if (fm.width(text) <= 65) {
        lab->setFixedWidth(65);
    } else if (fm.width(text) > 45) {
        lab->setFixedWidth(80);
        for (int i = 0; i < text.size(); i++) {
            str = str.left(str.length() - 1);
            if (str.length() == 1) {
                break;
            }
            if (fm.width(str) < 80) {
                break;
            }
        }
    }
    lab->setText(str);
}

void DFontMgrMainWindow::keyPressEvent(QKeyEvent *event)
{
    /*UT000539 判断slider是否聚焦，调整预览字体大小*/
    D_D(DFontMgrMainWindow);
    if (Qt::Key_Left == event->key() || Qt::Key_Down == event->key()) {
        if (d->fontScaleSlider->hasFocus()) {
            d->fontScaleSlider->setValue(d->fontScaleSlider->value() - 1);
        } else if (Qt::Key_Left == event->key() && (m_fontPreviewListView->hasFocus() || m_noInstallListView->hasFocus())) {
            d->leftSiderBar->setFocus(Qt::MouseFocusReason);
        }
    }
    if (Qt::Key_Right == event->key() || Qt::Key_Up == event->key()) {
        if (d->fontScaleSlider->hasFocus()) {
            d->fontScaleSlider->setValue(d->fontScaleSlider->value() + 1);
            /*焦点在leftSliderBar同时右侧listview有字体显示时,实现右键切换焦点至previewlistview,切换后选中首个 UT000539*/
        } else if (Qt::Key_Right == event->key() && d->leftSiderBar->hasFocus() && m_fontPreviewListView->isVisible()) {
            m_fontPreviewListView->setFocus(Qt::MouseFocusReason);
            if (0 == m_fontPreviewListView->selectionModel()->selectedIndexes().count()) {
                DFontPreviewProxyModel *filterModel = m_fontPreviewListView->getFontPreviewProxyModel();
                QModelIndex modelIndex = filterModel->index(0, 0);
                if (modelIndex.isValid())
                    m_fontPreviewListView->setCurrentIndex(modelIndex);
            }
        }
    }

    DWidget::keyPressEvent(event);
}
