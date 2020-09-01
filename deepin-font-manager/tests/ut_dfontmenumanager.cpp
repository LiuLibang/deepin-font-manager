#define private public
#define protected public
#include "interfaces/dfontmenumanager.h"
#undef private
#undef protected

#include <DApplicationHelper>
#include <DApplication>
#include <DMenu>
#include <DLog>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"

namespace {
class TestDFontMenuManager : public testing::Test
{

protected:
    void SetUp()
    {
        fm = DFontMenuManager::getInstance();
    }
    void TearDown()
    {
//        delete fm;
    }
    // Some expensive resource shared by all tests.
    QWidget *w = new QWidget;
    DFontMenuManager *fm;
};

}

TEST_F(TestDFontMenuManager, checkCreateToolBarSettingsMenu)
{
    fm->initMenuData();
    DMenu *menu = fm->createToolBarSettingsMenu();

    EXPECT_FALSE(menu->isEmpty());
    //有一行分割线
    EXPECT_TRUE(fm->m_fontToolsBarMenus.count() == 1);
//    delete fm;

    DFontMenuManager::FMenuItem *item = new DFontMenuManager::FMenuItem(DApplication::translate("Menu", "Add font"),
                                                                        DFontMenuManager::MenuAction::M_AddFont, true, false);

    item->subMenulist.append(new DFontMenuManager::FMenuItem(DApplication::translate("Menu", "Add font"),
                                                             DFontMenuManager::MenuAction::M_AddFont, false, false));
    fm->m_fontToolBarMenuData.append(item);
    menu = fm->createToolBarSettingsMenu();
}

TEST_F(TestDFontMenuManager, checkCreateRightKeyMenu)
{
//    fm->initMenuData();
    DMenu *menu = fm->createRightKeyMenu();

    EXPECT_FALSE(menu->isEmpty());
    //有七项
    EXPECT_TRUE(fm->m_fontRightKeyMenus.count() == 7);

    DFontMenuManager::FMenuItem *item = new DFontMenuManager::FMenuItem(DApplication::translate("Menu", "Add font"),
                                                                        DFontMenuManager::MenuAction::M_AddFont, true, false);

    item->subMenulist.append(new DFontMenuManager::FMenuItem(DApplication::translate("Menu", "Add font"),
                                                             DFontMenuManager::MenuAction::M_AddFont, false, false));
    fm->m_fontRightMenuData.append(item);
    menu = fm->createRightKeyMenu();
}

TEST_F(TestDFontMenuManager, checkOnRightKeyMenuPopup)
{
    DFontPreviewItemData data;
    data.fontData.setCollected(true);
    fm->onRightKeyMenuPopup(data, true, true, true);
    EXPECT_TRUE(fm->getActionByMenuAction(DFontMenuManager::M_DeleteFont,
                                          DFontMenuManager::MenuType::RightKeyMenu)->isEnabled());
    EXPECT_TRUE(fm->getActionByMenuAction(DFontMenuManager::M_ExportFont,
                                          DFontMenuManager::MenuType::RightKeyMenu)->isEnabled());
    data.fontData.setCollected(false);
    data.fontData.setEnabled(true);
    fm->onRightKeyMenuPopup(data, false, true, false);
    EXPECT_FALSE(fm->getActionByMenuAction(DFontMenuManager::M_DeleteFont,
                                           DFontMenuManager::MenuType::RightKeyMenu)->isEnabled());
    EXPECT_FALSE(fm->getActionByMenuAction(DFontMenuManager::M_ExportFont,
                                           DFontMenuManager::MenuType::RightKeyMenu)->isEnabled());

    fm->onRightKeyMenuPopup(data, false, false, false);
}

TEST_F(TestDFontMenuManager, checkGetActionByMenuAction)
{
    QAction *addAction = DFontMenuManager::getInstance()->getActionByMenuAction(
                             DFontMenuManager::M_AddFont, DFontMenuManager::MenuType::ToolBarMenu);

    EXPECT_TRUE(addAction->text() == "Add font");
}

