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

}



































