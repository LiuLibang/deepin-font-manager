#define private public
#define protected public
#include "views/dfinstallnormalwindow.h"
#undef private
#undef protected

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"

#include "dfontmanager.h"
#include "globaldef.h"
#include "utils.h"
#include "dfmdbmanager.h"
#include "dfontpreviewlistdatathread.h"
#include "views/dfinstallerrordialog.h"
#include <QResizeEvent>
#include <QVBoxLayout>

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DLog>

namespace {
class TestDFInstallNormalWindow : public testing::Test
{

protected:
    void SetUp()
    {
        iw = new DFInstallNormalWindow();
    }
    void TearDown()
    {
        delete iw;
    }
    // Some expensive resource shared by all tests.
    DFInstallNormalWindow *iw;
    QWidget *w = new QWidget;
};

QList<DFontPreviewItemData> stub_getFontModelList()
{
    QList<DFontPreviewItemData> list;

    DFontPreviewItemData data;
    data.fontInfo.filePath = "/usr/share/";
    data.fontInfo.familyName = "first";
    data.fontInfo.styleName = "second";

    list << data;
    return list;
}


}

TEST_F(TestDFInstallNormalWindow, checkGetAllSysfiles)
{
    Stub s;
    s.set(ADDR(DFontPreviewListDataThread, getFontModelList), stub_getFontModelList);

    iw->getAllSysfiles();
    EXPECT_TRUE(iw->m_AllSysFilesfamilyName.contains("firstsecond"));
}
