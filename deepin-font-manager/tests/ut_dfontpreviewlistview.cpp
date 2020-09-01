#define private public
#define protected public
#include "dfontpreviewlistview.h"
#undef private
#undef protected

#include "dfontinfomanager.h"
#include "dfontpreviewitemdelegate.h"
#include "globaldef.h"
#include "dfmxmlwrapper.h"
#include "views/dfontmgrmainwindow.h"

#include <DLog>
#include <DMenu>
#include <DGuiApplicationHelper>
#include <DApplication>
#include <DMessageManager>

#include <QSignalSpy>
#include <QFontDatabase>
#include <QSet>
#include <QScroller>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"

namespace {
class TestDFontPreviewListView : public testing::Test
{

protected:
    void SetUp()
    {
        listview = new DFontPreviewListView(mw);
    }
    void TearDown()
    {

    }
    QString filepath = "/home/zhaogongqiang/Desktop/1048字体/ArkanaScriptRough.otf";
    DFontMgrMainWindow *mw = new DFontMgrMainWindow;
    DFontPreviewListView *listview;
};

//参数化测试
class TestUpdateCurrentFontGroup : public::testing::TestWithParam<int>
{
public:
    DFontMgrMainWindow *mw = new DFontMgrMainWindow;
    DFontPreviewListView *listview = new DFontPreviewListView(mw);
};

QWidget *stub_viewport()
{
    QWidget *widget = new QWidget;
    widget->setFixedHeight(120);
    return widget;
}

}

TEST_F(TestDFontPreviewListView, checkInitFontListData)
{
    QSignalSpy spy(listview, SIGNAL(onLoadFontsStatus(int)));
    listview->initFontListData();
    EXPECT_TRUE(spy.count() == 1) << spy.count() << "checkInitFontListData++";
}

TEST_F(TestDFontPreviewListView, checkOnItemRemoved)
{
    DFontPreviewItemData data;
    data.fontInfo.filePath = filepath;
    data.appFontId = 1;
    listview->onItemRemoved(data);
}

TEST_F(TestDFontPreviewListView, checkOnItemRemovedSys)
{
    DFontPreviewItemData data;
    data.fontInfo.filePath = filepath;
    data.appFontId = 1;
    listview->onItemRemovedFromSys(data);
}

TEST_P(TestUpdateCurrentFontGroup, checkUpadteCurrentFontGroup)
{
    int n =  GetParam();

    listview->updateCurrentFontGroup(n);

    EXPECT_TRUE(n == listview->m_currentFontGroup) << n << "+++++++++++++++";

}

INSTANTIATE_TEST_CASE_P(HandleTrueReturn, TestUpdateCurrentFontGroup, testing::Values(0, 1, 2, 3, 4, 5, 6));


TEST_F(TestDFontPreviewListView, checkCount)
{
    qDebug() << listview->model()->insertRow(0);

    listview->count();
    EXPECT_TRUE(1 == listview->count());
}

TEST_F(TestDFontPreviewListView, checkCancelDel)
{
    listview->cancelDel();
    EXPECT_TRUE(listview->m_selectAfterDel == -1);
}

TEST_F(TestDFontPreviewListView, checkViewChanged)
{
    listview->viewChanged();

    EXPECT_TRUE(listview->m_selectAfterDel == -1);
}

//markPositionBeforeRemoved
TEST_F(TestDFontPreviewListView, checkMarkPositionBeforeRemoved_isdelete)
{
    listview->model()->insertRow(0);
    QModelIndex index = listview->model()->index(0, 0);
    listview->selectionModel()->select(index, QItemSelectionModel::Select);
    listview->markPositionBeforeRemoved(true, QModelIndexList());
    EXPECT_TRUE(listview->m_selectAfterDel == 0);
}

TEST_F(TestDFontPreviewListView, checkMarkPositionBeforeRemoved_notdelete)
{
    listview->model()->insertRow(0);
    QModelIndex index = listview->model()->index(0, 0);
    QModelIndexList list;
    list << index;
    listview->markPositionBeforeRemoved(false, list);
    EXPECT_TRUE(listview->m_selectAfterDel == 0);
    qDebug() << listview->isVisible() << endl;

}

TEST_F(TestDFontPreviewListView, checkGetOnePageCount)
{
    int count = listview->getOnePageCount();
    EXPECT_TRUE(count == 12);

    listview->model()->insertRow(0);

//    QAbstractScrollArea
    Stub s;
    s.set(ADDR(QAbstractScrollArea, viewport), stub_viewport);


    count = listview->getOnePageCount();
    EXPECT_TRUE(count == 1);
}

TEST_F(TestDFontPreviewListView, checkAppendFilePath)
{
    QStringList list;
    list << "aaa" << "bbb";
    listview->appendFilePath(&list, "ccc");
    EXPECT_TRUE(list.count() == 3);

    listview->appendFilePath(&list, "aaa");
    EXPECT_TRUE(list.count() == 3);
}

TEST_F(TestDFontPreviewListView, checkGetCurFontStrName)
{
    DFontPreviewItemData d;
    d.fontData.strFontName = "aaa";

    listview->m_curFontData = d;

    EXPECT_TRUE(listview->getCurFontData().fontData.strFontName == "aaa");
    EXPECT_TRUE(listview->getCurFontStrName() == "aaa");
}

TEST_F(TestDFontPreviewListView, checkGetFontData)
{
    FontData data;
    data.strFontName = "A";
    EXPECT_TRUE(listview->getFontData(data).appFontId == -1);

}

TEST_F(TestDFontPreviewListView, checkSetRecoveryTabFocusState)
{
    listview->setRecoveryTabFocusState(false);
    EXPECT_FALSE(listview->m_recoveryTabFocusState);
}

TEST_F(TestDFontPreviewListView, checkSetIsTabFocus)
{
    listview->setIsTabFocus(false);
    EXPECT_FALSE(listview->getIsTabFocus());
}

TEST_F(TestDFontPreviewListView, checkUpdateSpinner)
{
    QSignalSpy s(listview, SIGNAL(requestShowSpinner(bool, bool, DFontSpinnerWidget::SpinnerStyles)));
    listview->m_curTm = QDateTime::currentMSecsSinceEpoch();
    usleep(360000);
    DFontSpinnerWidget::SpinnerStyles style = DFontSpinnerWidget::SpinnerStyles::StartupLoad;
    listview->updateSpinner(style, true);
    EXPECT_TRUE(s.count() == 1) << s.count();
}

TEST_F(TestDFontPreviewListView, checkUpdateModel)
{

    QSignalSpy s1(listview, SIGNAL(requestShowSpinner(bool, bool, DFontSpinnerWidget::SpinnerStyles)));
    QSignalSpy s2(listview, SIGNAL(rowCountChanged()));
    QSignalSpy s3(listview, SIGNAL(deleteFinished()));
    QSignalSpy s4(listview->m_signalManager, SIGNAL(fontSizeRequestToSlider()));

    listview->updateModel(false);

    EXPECT_TRUE(s1.count() == 1) << s1.count();
    EXPECT_TRUE(s2.count() == 1) << s2.count();
    EXPECT_TRUE(s3.count() == 1) << s3.count();
    EXPECT_TRUE(s4.count() == 1) << s4.count();
}

//TEST_F(TestDFontPreviewListView, checkGetCollectionIconRect)
//{
//    QPoint p(0, 0);
//    QPoint q(60, 80);
//    QRect c(p, q);
////    qDebug() << c.right() << "++++++++++" << c.top() << endl;
////    QRect r = listview->getCollectionIconRect(c);
////    EXPECT_TRUE(r.height() == 32);
//}

//TEST_F(TestDFontPreviewListView, checkGetCheckboxRect)
//{
//    QPoint p(0, 0);
//    QPoint q(60, 80);
//    QRect c(p, q);
////    qDebug() << c.right() << "++++++++++" << c.top() << endl;
////    QRect r = listview->getCheckboxRect(c);
////    EXPECT_TRUE(r.height() == 30) << r.height();
//}

TEST_F(TestDFontPreviewListView, checkOnUpdateCurrentFont)
{
    listview->m_fontChanged = true;
    listview->onUpdateCurrentFont();
    EXPECT_FALSE(listview->m_fontChangeTimer->isActive());

}
// sortModelIndexList
TEST_F(TestDFontPreviewListView, checkSortModelIndexList)
{
    QStandardItemModel *m_fontPreviewItemModel = new QStandardItemModel;
    m_fontPreviewItemModel->setColumnCount(1);
    DFontPreviewProxyModel *fpm = new DFontPreviewProxyModel;
    fpm->setSourceModel(m_fontPreviewItemModel);
    DFontPreviewListView *listview = new DFontPreviewListView;
    listview->setModel(fpm);
    m_fontPreviewItemModel->insertRows(0, 5);

    QModelIndexList list;

    QModelIndex index = m_fontPreviewItemModel->index(0, 0);
    QModelIndex index1 = m_fontPreviewItemModel->index(1, 0);

    list << index << index1;

    listview->sortModelIndexList(list);
    EXPECT_TRUE(list.first().row() == 1);
}

//deleteFontModelIndex
TEST_F(TestDFontPreviewListView, checkDeleteFontModelIndex)
{
//    QStandardItemModel *m_fontPreviewItemModel = new QStandardItemModel;
//    m_fontPreviewItemModel->setColumnCount(1);
//    DFontPreviewProxyModel *fpm = new DFontPreviewProxyModel;
//    fpm->setSourceModel(m_fontPreviewItemModel);
//    DFontPreviewListView *listview = new DFontPreviewListView;
//    listview->setModel(fpm);
    listview->m_fontPreviewItemModel->insertRows(1, 5);
    listview->m_fontPreviewProxyModel->insertRow(1);
    qDebug() << listview->m_fontPreviewItemModel->rowCount();

    listview->deleteFontModelIndex("", true);
}


//isDeleting
TEST_F(TestDFontPreviewListView, checkIsDeleting)
{
//    QMouseEvent *event = new QMouseEvent();
//    QTest::mouseClick(listview, Qt::LeftButton);
}








