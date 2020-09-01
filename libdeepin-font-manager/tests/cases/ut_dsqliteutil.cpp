#define private public
#include "dsqliteutil.h"
#undef private

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"

#include <QDir>

#include <DLog>

namespace {
class TestDSqliteUtil : public testing::Test
{
protected:
    void SetUp()
    {
        fs = new DSqliteUtil();
    }
    void TearDown()
    {
        delete  fs;
    }
    // Some expensive resource shared by all tests.
    DSqliteUtil *fs;
    QString tbname = "t_fontmanager";
    QMap<QString, QString> data;
};
}

TEST_F(TestDSqliteUtil, checkEscapeString)
{
    EXPECT_TRUE(fs->escapeString("") == "");
    EXPECT_TRUE(fs->escapeString(QString()) == "");
    EXPECT_EQ("aaa", fs->escapeString("aaa"));
}

//TEST_F(TestDSqliteUtil, checkDelRecord)
//{
//    data.insert("fontId", "1");
//    EXPECT_TRUE(fs->addRecord(data, tbname));

//}
