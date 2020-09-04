#define private public
#define protected public
#include "views/dfontmgrmainwindow.h"
#undef private
#undef protected


#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"
#include "views/dfinstallnormalwindow.h"

#include "globaldef.h"
#include "interfaces/dfontmenumanager.h"
#include "utils.h"
#include "views/dfdeletedialog.h"
#include "views/dfontinfodialog.h"
#include "views/dfquickinstallwindow.h"

#include <QTest>
#include <QSignalSpy>
#include <QHBoxLayout>
#include <QShortcut>
#include <QFileSystemWatcher>
#include <QDBusConnection>

#include "dobject.h"
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

namespace {
class TestDFontMgrMainWindow : public testing::Test
{

protected:
    void SetUp()
    {
        fm = new DFontMgrMainWindow();
    }
    void TearDown()
    {
        delete fm;
    }

    DFontMgrMainWindow *fm;
};

bool stub_isvisible()
{
    return true;
}

bool stub_hasFocus()
{
    return true;
}

qint64 stub_getDiskSpace()
{

    return 1000;
}

qint64 stub_getSizeL()
{
    return 1500;
}

qint64 stub_getSizeS()
{
    return 500;
}

qint64 stub_bytesAvailable()
{
    return 1500;
}

bool stub_installFont()
{
    return true;
}

void stub_show()
{
    return;
}

void  stub_onLeftSiderBarItemClicked()
{
    return ;
}

bool stub_hasUrls()
{
    return true;
}

QList<QUrl> stub_urls()
{
    QList<QUrl> list;
    list << QUrl() << QUrl();

    return list;
}

bool stub_isFontMimeType()
{
    return true;
}


}

TEST_F(TestDFontMgrMainWindow, checkSetNextTabFocus)
{
    fm->setNextTabFocus(fm->m_ptr->addFontButton);
//    代码setfocus成功，但是检测焦点状态有问题
//    EXPECT_TRUE(fm->m_ptr->searchFontEdit->lineEdit()->hasFocus());
    fm->setNextTabFocus(fm->m_ptr->searchFontEdit->lineEdit());
    fm->setNextTabFocus(fm->m_fontPreviewListView);
    fm->setNextTabFocus(fm->m_ptr->leftSiderBar);

    Stub s;
    s.set(ADDR(QWidget, isVisible), stub_isvisible);
    fm->setNextTabFocus(fm->m_ptr->leftSiderBar);


    fm->setNextTabFocus(fm->m_ptr->fontScaleSlider);
    fm->setNextTabFocus(fm->m_ptr->fontShowArea);
}

TEST_F(TestDFontMgrMainWindow, checkKeyPressEvent)
{
//
    Stub s;
    s.set(ADDR(QWidget, hasFocus), stub_hasFocus);

    QSignalSpy spy(fm->m_signalManager, SIGNAL(setLostFocusState(bool)));

    fm->m_ptr->fontScaleSlider->setValue(10);
    QTest::keyPress(fm, Qt::Key_Down);
    EXPECT_TRUE(fm->m_ptr->fontScaleSlider->value() == 9);

    QTest::keyPress(fm, Qt::Key_Up);
    EXPECT_TRUE(fm->m_ptr->fontScaleSlider->value() == 10);
}

TEST_F(TestDFontMgrMainWindow, checkEventFilterKeypress)
{
    //没有可检测的数据
    QTest::keyPress(fm->m_ptr->searchFontEdit->lineEdit(), Qt::Key_Tab);
    QTest::keyPress(fm->m_ptr->textInputEdit->lineEdit(), Qt::Key_Tab);
    QTest::keyPress(fm->m_ptr->leftSiderBar, Qt::Key_Tab);

    QTest::keyPress(fm->m_ptr->addFontButton, Qt::Key_Right);

    QTest::keyPress(fm->m_ptr->searchFontEdit->lineEdit(), Qt::Key_Escape);
    EXPECT_TRUE(fm->m_ptr->searchFontEdit->lineEdit()->text().isEmpty());

    QTest::keyPress(fm->m_ptr->textInputEdit->lineEdit(), Qt::Key_Escape);
    EXPECT_TRUE(fm->m_ptr->textInputEdit->lineEdit()->text().isEmpty());
}

TEST_F(TestDFontMgrMainWindow, checkEventFilterFocusOut)
{
    QFocusEvent *e = new QFocusEvent(QEvent::FocusOut, Qt::ActiveWindowFocusReason);
    fm->eventFilter(fm->m_ptr->leftSiderBar, e);
    EXPECT_FALSE(fm->m_leftListViewTabFocus);

    Stub s;
    s.set(ADDR(DSplitListWidget, IsTabFocus), stub_hasFocus);

    fm->eventFilter(fm->m_ptr->leftSiderBar, e);
    EXPECT_TRUE(fm->m_currentStatus.m_IsFirstFocus);

    fm->eventFilter(fm->m_fontPreviewListView, e);
    EXPECT_FALSE(fm->m_previewListViewTabFocus);

    fm->eventFilter(fm->m_ptr->fontScaleSlider, e);
    EXPECT_TRUE(fm->m_fontPreviewListView->m_isGetFocusFromSlider);

}

TEST_F(TestDFontMgrMainWindow, checkEventFilterFocusIn)
{
    QSignalSpy spy(fm->m_signalManager, SIGNAL(setLostFocusState(bool)));


    QFocusEvent *e = new QFocusEvent(QEvent::FocusIn, Qt::ActiveWindowFocusReason);
    fm->m_leftListViewTabFocus = true;
    fm->eventFilter(fm->m_ptr->leftSiderBar, e);
    EXPECT_FALSE(fm->m_leftListViewTabFocus);

    fm->m_leftListViewTabFocus = false;
    fm->eventFilter(fm->m_ptr->leftSiderBar, e);
    EXPECT_TRUE(fm->m_ptr->leftSiderBar->m_IsHalfWayFocus);

    fm->eventFilter(fm->m_fontPreviewListView, e);
    EXPECT_TRUE(spy.count() == 1);
    EXPECT_FALSE(fm->m_fontPreviewListView->m_IsTabFocus);

    fm->eventFilter(fm->m_ptr->searchFontEdit->lineEdit(), e);
    EXPECT_FALSE(fm->m_isSearchLineEditMenuPoped);

    fm->eventFilter(fm->m_ptr->textInputEdit->lineEdit(), e);
    EXPECT_FALSE(fm->m_isSearchLineEditMenuPoped);

}

TEST_F(TestDFontMgrMainWindow, checkEventFilterMouse)
{
    QSignalSpy spy(fm->m_signalManager, SIGNAL(setLostFocusState(bool)));

    QTest::mousePress(fm->m_ptr->searchFontEdit->lineEdit(), Qt::RightButton);
    EXPECT_TRUE(fm->m_isSearchLineEditMenuPoped);

    QTest::mousePress(fm->m_ptr->textInputEdit->lineEdit(), Qt::RightButton);
    EXPECT_TRUE(fm->m_isSearchLineEditMenuPoped);

    QTest::mousePress(fm->m_ptr->addFontButton, Qt::RightButton);

}

TEST_F(TestDFontMgrMainWindow, checkCheckFilesSpace)
{
    Stub s;
    s.set(ADDR(DFontMgrMainWindow, getDiskSpace), stub_getDiskSpace);

    Stub s1;
    s1.set(ADDR(QFileInfo, size), stub_getSizeL);

    QStringList list;
    list << "first";

    EXPECT_TRUE(fm->checkFilesSpace(list).isEmpty());

    s1.set(ADDR(QFileInfo, size), stub_getSizeS);
    EXPECT_TRUE(fm->checkFilesSpace(list).contains("first"));

}

TEST_F(TestDFontMgrMainWindow, checkGetDiskSpace)
{
    Stub s;
    s.set(ADDR(QStorageInfo, bytesAvailable), stub_bytesAvailable);

    EXPECT_TRUE(1500 == fm->getDiskSpace());
    EXPECT_TRUE(1500 == fm->getDiskSpace(false));

}

TEST_F(TestDFontMgrMainWindow, checkOnPreviewTextChanged)
{
    fm->m_fontPreviewListView->m_bLoadDataFinish = true;
//    fm->onPreviewTextChanged();

    fm->m_fontPreviewListView->getFontPreviewProxyModel()->insertRows(0, 5);
    fm->onPreviewTextChanged();

    fm->onPreviewTextChanged("first");
    fm->onPreviewTextChanged();
}

TEST_F(TestDFontMgrMainWindow, checkWaitForInsert)
{
    fm->m_waitForInstall.clear();

    fm->waitForInsert();

    fm->m_waitForInstall.append("first");

    Stub s;
    s.set(ADDR(DFontMgrMainWindow, installFont), stub_installFont);
    fm->waitForInsert();

    EXPECT_TRUE(fm->m_waitForInstall.isEmpty());
}

TEST_F(TestDFontMgrMainWindow, checkHideSpinner)
{
    fm->m_cacheFinish = true;
    fm->m_installFinish = true;

    fm->hideSpinner();
    //定时器触发lambda函数，无法测试到
}

TEST_F(TestDFontMgrMainWindow, checkShowSpinner)
{
    Stub s;
    s.set(ADDR(QWidget, show), stub_show);

    fm->showSpinner(DFontSpinnerWidget::Load);

    fm->showSpinner(DFontSpinnerWidget::Load, true);
}

TEST_F(TestDFontMgrMainWindow, checkShowInstalledFiles)
{
    Stub s;
    s.set(ADDR(DFontMgrMainWindow, onLeftSiderBarItemClicked), stub_onLeftSiderBarItemClicked);

    fm->showInstalledFiles();
}

TEST_F(TestDFontMgrMainWindow, checkShowAllShortcut)
{
    //startDetached函数打桩失败，导致会有界面弹出。之后再研究这里如何打桩
    fm->showAllShortcut();
}

TEST_F(TestDFontMgrMainWindow, checkResizeEvent)
{

    QResizeEvent *e = new QResizeEvent(QSize(), QSize());

    fm->resizeEvent(e);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect =  screen->availableVirtualGeometry();
    fm->resize(screenRect.size());
    fm->resizeEvent(e);
}

TEST_F(TestDFontMgrMainWindow, checkDropEvent)
{
    QPointF p(300, 300);
    QMimeData data;

    QSignalSpy spy(fm, SIGNAL(fileSelected(const QStringList &)));

    QDropEvent *e = new QDropEvent(p, Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier);
    fm->dropEvent(e);

    Stub s;
    s.set(ADDR(QMimeData, hasUrls), stub_hasUrls);

    Stub s1;
    s1.set(ADDR(QMimeData, urls), stub_urls);

    Stub s2;
    s2.set(ADDR(Utils, isFontMimeType), stub_isFontMimeType);

    Stub s3;
    s3.set(ADDR(DFontMgrMainWindow, installFont), stub_installFont);


    fm->dropEvent(e);
    EXPECT_TRUE(spy.count() == 1);


//    fm->dropEvent(e);

}











