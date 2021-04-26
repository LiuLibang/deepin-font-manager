#include "dfontpreviewlistdatathread.h"
#include "dfmxmlwrapper.h"
#include "dfontpreviewlistview.h"
#include "dcopyfilesmanager.h"
#include "dcomworker.h"

#include <QFontDatabase>
#include <QApplication>

static DFontPreviewListDataThread *INSTANCE = nullptr;

QList<DFontPreviewItemData> DFontPreviewListDataThread::m_fontModelList = QList<DFontPreviewItemData>();
//系统字体路径
const QString FONTS_DIR = QDir::homePath() + "/.local/share/fonts/";

const QString FONTS_UP_DIR = QDir::homePath() + "/.local/share/";

DFontPreviewListDataThread *DFontPreviewListDataThread::instance(DFontPreviewListView *view)
{
    if (!INSTANCE) {
        INSTANCE = new DFontPreviewListDataThread(view);
    }

    return INSTANCE;
}

DFontPreviewListDataThread *DFontPreviewListDataThread::instance()
{
    return INSTANCE;
}

/*************************************************************************
 <Function>      DFontPreviewListDataThread
 <Description>    构造函数
 <Author>        null
 <Input>
    <param1>     view            Description:父指针
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
DFontPreviewListDataThread::DFontPreviewListDataThread(DFontPreviewListView *view)
    : m_view(view)
    , m_fsWatcher(nullptr)
    , m_mutex(nullptr)
{
    if (view != nullptr)
        m_mutex = view->getMutex();

    m_dbManager = DFMDBManager::instance();
    moveToThread(&mThread);
    QObject::connect(&mThread, SIGNAL(started()), this, SLOT(doWork()));
    connect(this, &DFontPreviewListDataThread::requestDeleted, this, &DFontPreviewListDataThread::onFileDeleted/*, Qt::QueuedConnection*/);
    connect(this, &DFontPreviewListDataThread::requestAdded, this, &DFontPreviewListDataThread::onFileAdded/*, Qt::QueuedConnection*/);
    connect(this, &DFontPreviewListDataThread::requestForceDeleteFiles, this, &DFontPreviewListDataThread::forceDeleteFiles);
    connect(this, &DFontPreviewListDataThread::requestRemoveFileWatchers, this, &DFontPreviewListDataThread::onRemoveFileWatchers);
    connect(this, &DFontPreviewListDataThread::requestAutoDirWatchers, this, &DFontPreviewListDataThread::onAutoDirWatchers);
    connect(this, &DFontPreviewListDataThread::requestExportFont, this, &DFontPreviewListDataThread::onExportFont);

    QTimer::singleShot(3, [this] {
        mThread.start();
    });
}

DFontPreviewListDataThread::~DFontPreviewListDataThread()
{
}

/*************************************************************************
 <Function>      doWork
 <Description>   线程函数
 <Author>        null
 <Input>
    <param1>     null            Description:null
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::doWork()
{
    QMutexLocker locker(m_mutex);
    initFileSystemWatcher();
    m_fontModelList.clear();

    qDebug() << "doWork thread id = " << QThread::currentThreadId();

    QStringList disableFontList = DFMXmlWrapper::getFontConfigDisableFontPathList();

    FontManager::getFontList(true);

    if (!m_fontModelList.isEmpty()) {
        //从fontconfig配置文件同步字体启用/禁用状态数据
        syncFontEnableDisableStatusData(disableFontList);
        refreshFontListData(true, disableFontList);

        m_view->onFinishedDataLoad();
        return;
    }

    qDebug() << "strAllFontList.size()" << m_allFontPathList.size() << endl;
    int index = 0;
    for (QString &filePath : m_allFontPathList) {
        if (filePath.length() > 0) {
            index = insertFontItemData(filePath, index, m_chineseFontPathList, m_monoSpaceFontPathList, true);
        }
    }

    for (QString &filePath : disableFontList) {
        index =  insertFontItemData(filePath, index, m_chineseFontPathList, m_monoSpaceFontPathList, true, false);
    }

    m_dbManager->commitAddFontInfo();
    m_view->onFinishedDataLoad();

    Q_EMIT m_view->multiItemsAdded(m_fontModelList, DFontSpinnerWidget::StartupLoad);
}

/*************************************************************************
 <Function>      initFileSystemWatcher
 <Description>   初始化系统文件监视器
 <Author>        null
 <Input>
    <param1>     null            Description:null
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::initFileSystemWatcher()
{
    if (m_fsWatcher == nullptr)
        m_fsWatcher = new QFileSystemWatcher(this);
    QDir dir(FONTS_DIR);
    if (!dir.exists())
        dir.mkpath(FONTS_DIR);

    m_deleteFileRecivetimer = new QTimer;

    m_fsWatcher->addPath(FONTS_DIR);
    m_fsWatcher->addPath(FONTS_UP_DIR);

    connect(m_fsWatcher, &QFileSystemWatcher::fileChanged,
    this, [ = ](const QString & path) {
        //启动定时器收集需要删除的字体
        m_deleteFileRecivetimer->start(20);
        m_waitForDeleteFiles << path;
    });

    //定时器停止表示没有需要删除的字体了,开始进行进行删除
    connect(m_deleteFileRecivetimer, &QTimer::timeout, this, [ = ] {
        qDebug() << m_waitForDeleteFiles.count();
        //删除前显示加载动画
        m_view->updateSpinner(DFontSpinnerWidget::Delete, false);
        m_view->updateChangedFile(m_waitForDeleteFiles);
        //删除后加载动画消失
        Q_EMIT m_view->requestShowSpinner(false, true, DFontSpinnerWidget::Delete);
        m_waitForDeleteFiles.clear();
    });

    connect(m_fsWatcher, &QFileSystemWatcher::directoryChanged,
    this, [ = ](const QString & path) {
        QFileInfo f(path);
        if (!f.isDir())
            return ;

        //删除前显示加载动画
        m_view->updateSpinner(DFontSpinnerWidget::Delete, false);
        updateChangedDir();

        //删除后加载动画消失
        Q_EMIT m_view->requestShowSpinner(false, true, DFontSpinnerWidget::Delete);

        if (!dir.exists()) {
            m_fsWatcher->removePath(FONTS_DIR);
        }
    });
}

/*************************************************************************
 <Function>      updateChangedFile
 <Description>   文件修改后触发函数
 <Author>        null
 <Input>
    <param1>     path            Description:路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
//void DFontPreviewListDataThread::updateChangedFile(const QString &path)
//{
//    m_view->updateChangedFile(path);
//}

/*************************************************************************
 <Function>      updateChangedDir
 <Description>   文件夹修改后触发函数
 <Author>        null
 <Input>
    <param1>     path            Description:路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::updateChangedDir()
{
    QMutexLocker locker(m_mutex);
    m_view->updateChangedDir();
}

/*************************************************************************
 <Function>      addPathWatcher
 <Description>   添加文件监视器
 <Author>        null
 <Input>
    <param1>     path            Description:需要添加的路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::addPathWatcher(const QString &path)
{
    if (m_fsWatcher == nullptr)
        return;

    if (!QFileInfo(path).exists())
        return;

    if (m_fsWatcher->addPath(path)) {
        if (!m_fsWatcher->directories().contains(FONTS_DIR))
            m_fsWatcher->addPath(FONTS_DIR);

        if (!m_fsWatcher->directories().contains(FONTS_UP_DIR))
            m_fsWatcher->addPath(FONTS_UP_DIR);
    }
}

/*************************************************************************
 <Function>      removePathWatcher
 <Description>   移除特定路径的文件监视器
 <Author>        null
 <Input>
    <param1>     path            Description:需要移除的路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::removePathWatcher(const QString &path)
{
    if (m_fsWatcher == nullptr)
        return;
    m_fsWatcher->removePath(path);
}

/*************************************************************************
 <Function>      onFileDeleted
 <Description>   文件删除响应函数
 <Author>        null
 <Input>
    <param1>     files            Description:删除的文件路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::onFileDeleted(QStringList &files)
{
    qDebug() << __FUNCTION__ << files.size();
    if (m_mutex != nullptr)
        QMutexLocker locker(m_mutex);
    m_view->deleteCurFonts(files, false);
}

/*************************************************************************
 <Function>      onFileAdded
 <Description>   增加文件响应函数
 <Author>        null
 <Input>
    <param1>     files            Description:增加的文件路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::onFileAdded(const QStringList &files)
{
    if (files.isEmpty()) {
        return;
    }

    if (m_mutex != nullptr)
        QMutexLocker locker(m_mutex);
    qDebug() << __func__ << "S" << QThread::currentThreadId() << endl;
    refreshFontListData(false, files);
}

/*************************************************************************
 <Function>      getFontModelList
 <Description>   获取当前列表所有项的数据链表
 <Author>        null
 <Input>
    <param1>     null                         Description:null
 <Return>        QList<DFontPreviewItemData>  Description:列表所有项的数据链表
 <Note>          null
*************************************************************************/
QList<DFontPreviewItemData> DFontPreviewListDataThread::getFontModelList()
{
    if (m_view->isListDataLoadFinished())
        return m_fontModelList;
    return QList<DFontPreviewItemData>();
}

//更新itemDataList的itemData状态

/*************************************************************************
 <Function>      updateItemStatus
 <Description>   更新itemDataList的itemData状态
 <Author>        null
 <Input>
    <param1>     index               Description:需要更新的位置
    <param2>     itemData            Description:需要更新的数据
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::updateItemStatus(int index, const DFontPreviewItemData &itemData)
{
    m_fontModelList.replace(index, itemData);
}

/*************************************************************************
 <Function>      getDiffFontModelList
 <Description>   获取需要新增的字体数据链表
 <Author>        null
 <Input>
    <param1>     null            Description:null
 <Return>        QList<DFontPreviewItemData>            Description:新增的字体数据链表
 <Note>          null
*************************************************************************/
QList<DFontPreviewItemData> DFontPreviewListDataThread::getDiffFontModelList() const
{
    return m_diffFontModelList;
}

/*************************************************************************
 <Function>      setMutex
 <Description>   给线程锁赋值
 <Author>        null
 <Input>
    <param1>     mutex            Description:线程锁赋的值
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::setMutex(QMutex *mutex)
{
    m_mutex = mutex;
}

/*************************************************************************
 <Function>      forceDeleteFiles
 <Description>   强制删除文件
 <Author>        null
 <Input>
    <param1>     files            Description:需要删除的文件路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::forceDeleteFiles(QStringList &files)
{
    qDebug() << __FUNCTION__ << files << m_mutex;
    if (m_mutex != nullptr)
        QMutexLocker locker(m_mutex);
    m_view->deleteFontFiles(files, true);

    Q_EMIT requestBatchReInstallContinue();
    qDebug() << __FUNCTION__ << files << " end ";
}

/*************************************************************************
 <Function>      onRemoveFileWatchers
 <Description>   批量移除文件监视器
 <Author>        null
 <Input>
    <param1>     files            Description:需要移除的路径
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::onRemoveFileWatchers(const QStringList &files)
{
    qDebug() << __FUNCTION__ << files.size() << m_fsWatcher->removePaths(files);
    Q_UNUSED(files)
    m_fsWatcher->removePath(FONTS_DIR);
    m_fsWatcher->removePath(FONTS_UP_DIR);
}

/*************************************************************************
 <Function>      onAutoDirWatchers
 <Description>   添加特定文件夹的文件监视器
 <Author>        null
 <Input>
    <param1>     null            Description:null
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::onAutoDirWatchers()
{
    m_fsWatcher->addPath(FONTS_DIR);
    m_fsWatcher->addPath(FONTS_UP_DIR);
}

/**
* @brief DFontPreviewListDataThread::onExportFont 导出文件列表槽函数
* @param fontList 要导出的文件列表
* @return void
*/
void DFontPreviewListDataThread::onExportFont(QStringList &fontList)
{
    qDebug() << __FUNCTION__ << fontList.size();

    QDir dir(FONTS_DESKTOP_DIR);
    if (!dir.exists())
        dir.mkpath(FONTS_DESKTOP_DIR);

    DCopyFilesManager::instance()->copyFiles(CopyFontThread::EXPORT, fontList);
    Q_EMIT exportFontFinished(fontList.size());
}

/*************************************************************************
 <Function>      insertFontItemData
 <Description>   将需要添加项的字体数据收集放人list中.
 <Author>        null
 <Input>
    <param1>     filepath            Description:文件路径
    <param2>     index               Description:null添加的位置
    <param3>     chineseFontPathList           Description:中文字体列表
    <param4>     monoSpaceFontPathList           Description:等宽字体列表
    <param5>     isStartup           Description:是否为第一次启动
    <param6>     isEnabled           Description:是否可以换启用

 <Return>        int                 Description:null
 <Note>          null
*************************************************************************/
int DFontPreviewListDataThread::insertFontItemData(const QString &filePath,
                                                   int index,
                                                   const QStringList &chineseFontPathList,
                                                   const QStringList &monoSpaceFontPathList,
                                                   bool isStartup, bool isEnabled)
{
    DFontInfoManager *fontInfoMgr = DFontInfoManager::instance();
    DFontPreviewItemData itemData;
    itemData.fontInfo = fontInfoMgr->getFontInfo(filePath, true);

    QString familyName;
    if (itemData.fontInfo.sp3FamilyName.isEmpty() || itemData.fontInfo.sp3FamilyName.contains(QChar('?'))) {
        int appFontId = QFontDatabase::addApplicationFont(itemData.fontInfo.filePath);

        itemData.appFontId = appFontId;
        QStringList fontFamilyList = QFontDatabase::applicationFontFamilies(appFontId);
        for (QString &family : fontFamilyList) {
            if (family.contains(QChar('?')))
                continue;
            familyName = family;
        }
        if (familyName.isEmpty()) {
            familyName = itemData.fontInfo.fullname;
        } else {
            itemData.fontInfo.sp3FamilyName = familyName;
        }
    } else if (itemData.fontInfo.isSystemFont) {
        familyName = itemData.fontInfo.sp3FamilyName;
    } else {
        familyName = itemData.fontInfo.fullname;
    }

    if (itemData.fontInfo.styleName.length() > 0 && !familyName.endsWith(itemData.fontInfo.styleName)) {
        itemData.fontData.strFontName =
            QString("%1-%2").arg(familyName).arg(itemData.fontInfo.styleName);
    } else {
        itemData.fontData.strFontName = familyName;
    }

    itemData.strFontId = QString::number(index);
    itemData.fontData.setEnabled(isEnabled);
    itemData.fontData.setCollected(false);
    itemData.fontData.setChinese(chineseFontPathList.contains(filePath) && (itemData.fontInfo.previewLang & FONT_LANG_CHINESE));
    itemData.fontData.setMonoSpace(monoSpaceFontPathList.contains(filePath));
    itemData.fontData.setFontType(itemData.fontInfo.type);
    itemData.fontData.isSystemFont = m_dbManager->isSystemFont(filePath);

    itemData.fontInfo.isInstalled = true;

    //中文字体
    if (itemData.fontInfo.isSystemFont && itemData.fontData.isChinese()) {
        /* Bug#16821 UT000591  添加字体后需要加入到Qt的字体数据库中，否则无法使用*/
//        qDebug() << "addApplicationFont s"  << endl;
        int appFontId = QFontDatabase::addApplicationFont(itemData.fontInfo.filePath);
//        qDebug() << "addApplicationFont e"  << appFontId << itemData.fontData.strFontName << endl;
        itemData.appFontId = appFontId;

        QStringList fontFamilyList = QFontDatabase::applicationFontFamilies(appFontId);
        if (fontFamilyList.size() > 1) {
            ++index;
            for (QString &fontFamily : fontFamilyList) {
                itemData.strFontId = QString::number(index);
                itemData.fontInfo.familyName = fontFamily;
                itemData.fontInfo.sp3FamilyName = fontFamily;
                if (itemData.fontInfo.styleName.length() > 0) {
                    itemData.fontData.strFontName =
                        QString("%1-%2").arg(itemData.fontInfo.familyName).arg(itemData.fontInfo.styleName);
                } else {
                    itemData.fontData.strFontName = itemData.fontInfo.familyName;
                }
                m_dbManager->addFontInfo(itemData);
                m_fontModelList.append(itemData);
            }
        } else {
            m_dbManager->addFontInfo(itemData);
            m_fontModelList.append(itemData);
        }
    } else {
        m_dbManager->addFontInfo(itemData);
        m_fontModelList.append(itemData);
    }

//    Q_EMIT m_view->itemAdded(itemData);
    addPathWatcher(filePath);

    if (!isStartup) {
        m_diffFontModelList.append(itemData);
    }
    return (index + 1);
}

/*************************************************************************
 <Function>      refreshFontListData
 <Description>    刷新字体列表的数据
 <Author>        null
 <Input>
    <param1>     isStartup              Description:是否为第一次启动
    <param2>     installFont            Description:安装的字体文件
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread:: refreshFontListData(bool isStartup, const QStringList &installFont)
{
    qDebug() << __FUNCTION__ << " begin" << isStartup << installFont.size() << QThread::currentThreadId() << m_fontModelList.size();

    if (isStartup) {
        for (const QString &filePath : installFont) {
            if (!m_allFontPathList.contains(filePath))
                m_allFontPathList << filePath;
        }
    } else {
        FontManager::getFontListInSequence(isStartup);
    }

    QSet<QString> dbFilePathSet;
    for (DFontPreviewItemData &itemData : m_fontModelList) {
        QString filePath = itemData.fontInfo.filePath.trimmed();

        dbFilePathSet.insert(filePath);
        if (isStartup)
            addPathWatcher(filePath);
    }

    if (isStartup) {
        for (DFontPreviewItemData &itemData : m_delFontInfoList) {
            //如果字体文件已经不存在，则从t_manager表中删除
            //删除字体之前启用字体，防止下次重新安装后就被禁用
            m_view->enableFont(itemData.fontInfo.filePath);
            DFMDBManager::instance()->deleteFontInfo(itemData);
        }
    }

    DFMDBManager::instance()->commitDeleteFontInfo();
    m_view->enableFonts();

    m_delFontInfoList.clear();
    m_diffFontModelList.clear();

    //根据文件路径比较出不同的字体文件
    QSet<QString> allFontListSet = m_allFontPathList.toSet();
    QSet<QString> diffSet = allFontListSet.subtract(dbFilePathSet);
    qDebug() << "diffSet count:" << diffSet.count();
    if (diffSet.count() > 0) {
        int maxFontId = m_dbManager->getCurrMaxFontId();
        QList<QString> diffFilePathList = diffSet.toList();
        int index = maxFontId + 1;
        for (QString &filePath : diffFilePathList) {
            if (m_dbManager->isSystemFont(filePath) || installFont.contains(filePath)) {
                bool isEnabled = (isStartup && installFont.contains(filePath)) ? false : true;
                index = insertFontItemData(filePath, index, m_chineseFontPathList, m_monoSpaceFontPathList, isStartup, isEnabled);
            }
        }
        m_dbManager->commitAddFontInfo();
    }
    qDebug() << __FUNCTION__ << " end " << QThread::currentThreadId() << m_diffFontModelList.size() << m_fontModelList.size();

    if (!isStartup && !installFont.isEmpty()) {
        if (!m_isAllLoaded) {
            //之前如果初始数据没有全加载时，安装字体后加载全部的数据。主要出现在文管中直接打开字体文件进行安装
            QList<DFontPreviewItemData> list = m_fontModelList.mid(50, m_fontModelList.size());
            Q_EMIT m_view->multiItemsAdded(list, DFontSpinnerWidget::NoLabel);

            // 数据完成完全加载，将标志位设置为true；
            m_isAllLoaded = true;
        } else {
            Q_EMIT m_view->multiItemsAdded(m_diffFontModelList, DFontSpinnerWidget::NoLabel);
        }
        Q_EMIT m_view->itemsSelected(installFont);
    } else if (isStartup) {
        QList<DFontPreviewItemData> list = m_fontModelList.mid(0, 50);
        Q_EMIT m_view->multiItemsAdded(list, DFontSpinnerWidget::StartupLoad);
    } else {
        Q_EMIT m_view->multiItemsAdded(m_fontModelList, DFontSpinnerWidget::Load);
    }
}

/*************************************************************************
 <Function>      syncFontEnableDisableStatusData
 <Description>   更新从配置文件中读到的禁用字体的信息
 <Author>        null
 <Input>
    <param1>     null            Description:null
    <param2>     null            Description:null
    <param3>     null            Description:null
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::syncFontEnableDisableStatusData(const QStringList &disableFontPathList)
{
    //disableFontPathList为被禁用的字体路径列表
//    if (disableFontPathList.size() == 0) {
//        return;
//    }

    QMap<QString, bool> disableFontMap;
    for (const QString &disableFontPath : disableFontPathList) {
        disableFontMap.insert(disableFontPath, true);
    }

    for (DFontPreviewItemData &fontItemData : m_fontModelList) {
        QString keyFilePath = fontItemData.fontInfo.filePath;

        if (fontItemData.fontData.isEnabled() != disableFontMap.value(keyFilePath))
            continue;

        //disableFontMap为被禁用的字体map
        if (disableFontMap.value(keyFilePath)) {
            fontItemData.fontData.setEnabled(false);
        } else {
            fontItemData.fontData.setEnabled(true);
        }

        m_dbManager->updateFontInfo(fontItemData, "isEnabled");
    }

    m_dbManager->commitUpdateFontInfo();
}


/*************************************************************************
 <Function>      updateFontId
 <Description>   更新字体信息中的fontid
 <Author>        null
 <Input>
    <param1>     null            Description:null
    <param2>     null            Description:null
    <param3>     null            Description:null
 <Return>        null            Description:null
 <Note>          null
*************************************************************************/
void DFontPreviewListDataThread::updateFontId(const DFontPreviewItemData &itemData, int id)
{
    if (id < 0)
        return;
    int index = m_fontModelList.indexOf(itemData);
    if (index > -1) {
        m_fontModelList[index].appFontId = id;
        if (m_fontModelList[index].fontInfo.sp3FamilyName != itemData.fontInfo.sp3FamilyName)
            m_fontModelList[index].fontInfo.sp3FamilyName = itemData.fontInfo.sp3FamilyName;
    }
}
