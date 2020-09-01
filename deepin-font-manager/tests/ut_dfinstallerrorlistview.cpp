#define private public
#define protected public
#include "views/dfinstallerrorlistview.h"
#undef private
#undef protected

#include "globaldef.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"
#include "utils.h"

#include <QTest>
#include <QSignalSpy>
#include <QPainter>
#include <QMouseEvent>
#include <QStandardItemModel>

#include <DLog>
#include <DStyleHelper>
#include <DApplication>
#include <DApplicationHelper>
#include <DCheckBox>
#include <DFontSizeManager>


namespace {
class TestDFInstallErrorListView : public testing::Test
{

protected:
    void SetUp()
    {
        iEListview = new DFInstallErrorListView(QList<DFInstallErrorItemModel>());
    }
    void TearDown()
    {
        delete iEListview;
    }
    // Some expensive resource shared by all tests.
    DFInstallErrorListView *iEListview;
    QWidget *w = new QWidget;
    QPainter *p = new QPainter;
    QStyleOptionViewItem option;
};


}

TEST_F(TestDFInstallErrorListView, checkPaint)
{
    DFInstallErrorItemModel model1;
    model1.bIsNormalUserFont = true;
    model1.bChecked = false;
    iEListview->m_installErrorFontModelList.append(model1);

    model1.bChecked = true;
    iEListview->m_installErrorFontModelList.append(model1);

    model1.bSelectable = false;
    iEListview->m_installErrorFontModelList.append(model1);

    iEListview->initErrorListData();

    QModelIndex index ;
    index = iEListview->getErrorListSourceModel()->index(0, 0);
    iEListview->m_errorListItemDelegate->paint(p, option, index);

    index = iEListview->getErrorListSourceModel()->index(1, 0);
    iEListview->m_errorListItemDelegate->paint(p, option, index);

    index = iEListview->getErrorListSourceModel()->index(2, 0);
    iEListview->m_errorListItemDelegate->paint(p, option, index);

//    iEListview->selectAll();
    index = iEListview->getErrorListSourceModel()->index(0, 0);
    option.state = QStyle::State_Selected;
    iEListview->m_IsTabFocus = true;
    iEListview->m_errorListItemDelegate->paint(p, option, index);

    iEListview->m_IsTabFocus = false;
    iEListview->m_errorListItemDelegate->paint(p, option, index);

}

TEST_F(TestDFInstallErrorListView, checkMousePressEvent)
{

    DFInstallErrorItemModel model1;
    model1.bIsNormalUserFont = true;
    model1.bChecked = false;
    iEListview->m_installErrorFontModelList.append(model1);

    model1.bChecked = true;
    iEListview->m_installErrorFontModelList.append(model1);
    iEListview->initErrorListData();

    QTest::mousePress(iEListview->viewport(), Qt::LeftButton);
    EXPECT_TRUE(iEListview->m_isMouseClicked);
    EXPECT_TRUE(iEListview->m_bLeftMouse);


    QTest::mousePress(iEListview->viewport(), Qt::RightButton);
    EXPECT_FALSE(iEListview->m_bLeftMouse);

}

TEST_F(TestDFInstallErrorListView, checkSetSelection)
{
    QSignalSpy spy(iEListview, SIGNAL(onClickErrorListItem(QModelIndex)));

    DFInstallErrorItemModel model1;
    model1.bIsNormalUserFont = true;
    model1.bChecked = false;
    iEListview->m_installErrorFontModelList.append(model1);
    iEListview->initErrorListData();

    QRect t;
    QTest::mousePress(iEListview->viewport(), Qt::LeftButton);
    iEListview->setSelection(t, QItemSelectionModel::Select);

    EXPECT_TRUE(spy.count() == 1);
}

TEST_F(TestDFInstallErrorListView, checkKeyPressEvent)
{
    DFInstallErrorItemModel model1;
    model1.bIsNormalUserFont = true;
    model1.bChecked = false;
    iEListview->m_installErrorFontModelList.append(model1);
    iEListview->initErrorListData();

    QTest::keyPress(iEListview->viewport(), Qt::Key_Home);
    QTest::keyPress(iEListview->viewport(), Qt::Key_End);
    QTest::keyPress(iEListview->viewport(), Qt::Key_PageUp);
    QTest::keyPress(iEListview->viewport(), Qt::Key_PageDown);

    QTest::keyPress(iEListview->viewport(), Qt::Key_Up, Qt::ControlModifier);
    QTest::keyPress(iEListview->viewport(), Qt::Key_Up, Qt::ShiftModifier);

    QTest::keyPress(iEListview->viewport(), Qt::Key_Up);
    QTest::keyPress(iEListview->viewport(), Qt::Key_Down);
    QTest::keyRelease(iEListview->viewport(), Qt::Key_Down);

}

//TEST_F(TestDFInstallErrorListView, checkLengthAutoFeed)
//{
//    QPainter *p = new QPainter;
//    QString str = iEListview->m_errorListItemDelegate->lengthAutoFeed(p, "asdsadasda", 50);
//    qDebug() << str << endl;
//}




