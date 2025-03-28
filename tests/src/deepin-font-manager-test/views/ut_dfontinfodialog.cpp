/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:
*
* Maintainer:
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "views/dfontinfodialog.h"
#include "commonheaderfile.h"

#include "dfontpreviewitemdef.h"
#include "utils.h"
#include "views/fonticontext.h"
#include "views/dfontinfoscrollarea.h"

#include <gtest/gtest.h>
#include "../third-party/stub/stub.h"

#include <QBitmap>
#include <QTest>
#include <QFileInfo>
#include <QFontMetrics>
#include <QTextBlock>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>

#include <DGuiApplicationHelper>
#include <DApplication>
#include <DApplicationHelper>
#include <DLog>
#include <DFontSizeManager>
#include <DTipLabel>

namespace {
class TestDFontInfoDialog: public testing::Test
{

protected:
    void SetUp()
    {
        w = new QWidget;
        dFontInforDialog = new DFontInfoDialog(&d, w);

    }
    void TearDown()
    {
        delete dFontInforDialog;
        delete  w;
    }
    DFontPreviewItemData d;
    QWidget *w;
    DFontInfoDialog *dFontInforDialog;

};

DGuiApplicationHelper::ColorType stub_darktype()
{
    return DGuiApplicationHelper::DarkType;
}
DGuiApplicationHelper::ColorType stub_lighttype()
{
    return DGuiApplicationHelper::LightType;
}
}

TEST_F(TestDFontInfoDialog, checkKeyPressEvent)
{
    QTest::keyClick(dFontInforDialog, Qt::Key_I, Qt::ControlModifier);

    EXPECT_FALSE(dFontInforDialog->isVisible());
}

TEST_F(TestDFontInfoDialog, checkAutoHeight)
{
    dFontInforDialog->autoHeight(200);
    EXPECT_TRUE(dFontInforDialog->m_scrollArea->viewport()->height() == static_cast<int>(200 * 1.1 + 10));
    dFontInforDialog->autoHeight(350);
    EXPECT_TRUE(dFontInforDialog->m_scrollArea->viewport()->height() == 375);
}

TEST_F(TestDFontInfoDialog, checkAutoFeed)
{
    QString str;
    str.fill('a', 100);
    qDebug() << str << endl;
    str = dFontInforDialog->AutoFeed(str);
    //进行处理之后字符串长度发生变化
    EXPECT_TRUE(str.size() != 100);


    str.fill('a', 300);
    qDebug() << str << endl;
    str = dFontInforDialog->AutoFeed(str);
    //进行处理之后字符串长度发生变化
    EXPECT_TRUE(str.size() != 300);


    QString str2;
    str2.fill('a', 20);
    str2 = dFontInforDialog->AutoFeed(str2);
    EXPECT_TRUE(str2.size() == 20) << str2.size();
}

//fontinfoArea
TEST_F(TestDFontInfoDialog, checkfontinfoAreaEventFilter)
{
    QEvent *e = new QEvent(QEvent::FontChange);

    dFontInforDialog->m_fontinfoArea->eventFilter(dFontInforDialog->m_fontinfoArea, e);
    SAFE_DELETE_ELE(e)
}

TEST_F(TestDFontInfoDialog, checkinitConnections001)
{
    Stub s;
    s.set(ADDR(DGuiApplicationHelper, themeType), stub_darktype);

    DPalette paFrame = DApplicationHelper::instance()->palette(dFontInforDialog->m_scrollArea->viewport());
    QColor colorFrame = paFrame.textLively().color();
    colorFrame.setAlphaF(0.05);
    paFrame.setColor(DPalette::Base, colorFrame);
    emit DApplicationHelper::instance()->themeTypeChanged(DApplicationHelper::DarkType);

    EXPECT_TRUE(DApplicationHelper::instance()->palette(dFontInforDialog->m_fontFileName) == paFrame);
}

TEST_F(TestDFontInfoDialog, checkinitConnections002)
{
    Stub s;
    s.set(ADDR(DGuiApplicationHelper, themeType), stub_lighttype);

    DPalette paFrame = DApplicationHelper::instance()->palette(dFontInforDialog->m_scrollArea->viewport());
    QColor colorFrame(255, 255, 255);
    colorFrame.setAlphaF(0.70);
    paFrame.setColor(DPalette::Base, colorFrame);
    emit DApplicationHelper::instance()->themeTypeChanged(DApplicationHelper::LightType);

    EXPECT_TRUE(DApplicationHelper::instance()->palette(dFontInforDialog->m_fontFileName) == paFrame);
}












