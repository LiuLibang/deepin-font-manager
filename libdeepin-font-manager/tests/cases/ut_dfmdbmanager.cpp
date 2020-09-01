#define private public
#include "dfmdbmanager.h"
#undef private

#include "dfontinfomanager.h"
#include "dsqliteutil.h"

#include <QDir>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"

namespace {
class TestDFMDBManager : public testing::Test
{

protected:
    void SetUp()
    {
        fmd = DFMDBManager::instance();
    }
    void TearDown()
    {
    }
    // Some expensive resource shared by all tests.
    DFMDBManager *fmd;
};

//bool stub_findAllRecords(void* obj ,const QList<QString> &key, QList<QMap<QString, QString>> &row,
//                         const QString &table_name)
//{
//    DSqliteUtil *a = (DSqliteUtil*) obj;


//    QMap<QString, QString> mapRow_first;
//    mapRow_first.insert("fontID", "1");
//    mapRow_first.insert("fontName", "first");
//    row.append(mapRow_first);

//    QMap<QString, QString> mapRow_second;
//    mapRow_second.insert("fontID", "2");
//    mapRow_second.insert("fontName", "second");
//    row.append(mapRow_second);

//    return true;
//}

QStringList stub_getInstalledFontsPath()
{
    QStringList list;
    list << "first" << "second" << "third";
    return list;
}

//int stub_getRecordCount(const QString &table_name = "t_fontmanager")
//{
//    Q_UNUSED(table_name)
//    return 4;
//}

int stub_getMaxFontId(const QString &table_name = "t_fontmanager")
{
    Q_UNUSED(table_name)
    return 10;
}

//bool stub_delRecord(QMap<QString, QString> where, const QString &table_name = "t_fontmanager")
//{
//    Q_UNUSED(table_name)
//    if (where.isEmpty()) {
//        return false;
//    } else {
//        return true;
//    }

//}

//stub_findRecords
//bool stub_findRecords(const QList<QString> &key, const QMap<QString, QString> &where,
//                      QList<QMap<QString, QString>> *row, const QString &table_name)
//{

//    Q_UNUSED(key)
//    Q_UNUSED(table_name)

//    QMap<QString, QString> mapRow;

//    mapRow.insert("filePath", "aaa");
//    row->append(mapRow);
//    mapRow.clear();
////    for (int i = 0; i < columnLen; i++) {
////        mapRow.insert(key.at(i), m_query->value(i).toString());
////    }
////    row->append(mapRow);
////    mapRow.clear();

////    if (where.value("familyname") == "first") {
////        QMap<QString, QString> mapRow;
////        mapRow.insert("first", "1");
////        row->append(mapRow);
////        return true;
////    }
////    return false;
//    return true;
//}

}


//传入系统字体判断是否为系统字体
TEST_F(TestDFMDBManager, check_SystemFont_IsSystemFont)
{
    Stub stub;
    EXPECT_EQ(true, fmd->isSystemFont("/usr/share/fonts/truetype/liberation/LiberationMono-Italic.ttf"));
}

//传入用户字体判断是否为系统字体
TEST_F(TestDFMDBManager, check_UserFont_IsSystestub_delRecordmFont)
{
    EXPECT_EQ(false, fmd->isSystemFont(QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf"));
}

//传入用户字体判断是否为用户字体
TEST_F(TestDFMDBManager, check_SystemFont_IsUserFont)
{
    EXPECT_EQ(false, fmd->isUserFont("/usr/share/fonts/truetype/liberation/LiberationMono-Italic.ttf"));
}

//传入未安装用户字体判断是否为用户字体
TEST_F(TestDFMDBManager, check_UserFontUnInstalled_IsUserFont)
{
    EXPECT_EQ(false, fmd->isUserFont(QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf"));
}

//传入已安装的用户字体
TEST_F(TestDFMDBManager, check_UserFontInstalled_IsUserFont)
{
    EXPECT_EQ(true, fmd->isUserFont(QDir::homePath() + "/.local/share/fonts/UpcEBwrP72xTt/V200020_.TTF"));
}



////check parseRecordToItemData函数是否正常 打桩还是有问题，传引用的时候值不对，需要进行修改。
//TEST_F(TestDFMDBManager, check_parseRecordToItemData_Isnormal)
//{
//    Stub s;
//    s.set(ADDR(DSqliteUtil, findAllRecords), stub_findAllRecords);
//    EXPECT_EQ(2, fmd->getAllFontInfo().count());
//}
TEST_F(TestDFMDBManager, check_getInstalledFontsPath)
{
    Stub s;
    s.set(ADDR(DSqliteUtil, getInstalledFontsPath), stub_getInstalledFontsPath);
    EXPECT_EQ(3, fmd->getInstalledFontsPath().count());
    EXPECT_EQ(true, fmd->getInstalledFontsPath().contains("first"));
}

TEST_F(TestDFMDBManager, check_getRecordCount)
{
    Stub s;
    s.set(ADDR(DSqliteUtil, getMaxFontId), stub_getMaxFontId);
    EXPECT_EQ(10, fmd->getCurrMaxFontId());
//    EXPECT_EQ(true, fmd->getInstalledFontsPath().contains("first"));
}

TEST_F(TestDFMDBManager, check_addFontInfo)
{
    DFontPreviewItemData data;
    data.appFontId = 1;
    EXPECT_EQ(true, fmd->addFontInfo(data));
    EXPECT_EQ(1, fmd->m_addFontList.count());

    //再次添加，因为添加的是重复的所以数目还是断言为1个
    EXPECT_EQ(true, fmd->addFontInfo(data));
    EXPECT_EQ(1, fmd->m_addFontList.count());

    //函数出错，issystemfont属性为默认值为true的原因

//    EXPECT_EQ(true, fmd->getInstalledFontsPath().contains("first"));
}

//TEST_F(TestDFMDBManager, check_deleteFontInfoByFontMap)
//{
//    QMap<QString, QString> map;
//    map.insert("path", "first");
//    Stub s;
//    s.set(ADDR(DSqliteUtil, delRecord), stub_delRecord);
//    EXPECT_EQ(true, fmd->deleteFontInfoByFontMap(map));

//    map.clear();
//    EXPECT_EQ(false, fmd->deleteFontInfoByFontMap(map));
//}




//TEST_F(TestDFMDBManager, check_IsFontInfoExist)
//{
//    Stub s;
//    s.set((bool(DSqliteUtil::*)(const QList<QString> &, const QMap<QString, QString> &, QList<QMap<QString, QString>> *, const QString &))ADDR(DSqliteUtil, findRecords), stub_findRecords);
////    EXPECT_EQ(10, fmd->isFontInfoExist());
////    EXPECT_EQ(true, fmd->getInstalledFontsPath().contains("first"));
//    DFontInfo f;
////    f.familyName = "first";
//    EXPECT_EQ("aaa", fmd->isFontInfoExist(f));

//}



