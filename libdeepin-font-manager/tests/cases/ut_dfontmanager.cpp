#define private public
#include "dfontmanager.h"
#undef private

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../third-party/stub/stub.h"

#include "signalmanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSignalSpy>

const QString sysDir = QDir::homePath() + "/.local/share/fonts";

namespace {
class TestDfontmanager : public testing::Test
{

protected:
    void SetUp()
    {
        fm = DFontManager::instance();
    }
    void TearDown()
    {
    }
    // Some expensive resource shared by all tests.
    DFontManager *fm;
};

void stub_Handle()
{

}

}

TEST_F(TestDfontmanager, checkSetType)
{
    fm->setType(DFontManager::Install);
    EXPECT_EQ(fm->m_type, DFontManager::Install);
//    delete fm;
}

TEST_F(TestDfontmanager, checkSetInstallFileList)
{
    QStringList list;
    list << "first" << "second" << "third" << "fouth";
    fm->setInstallFileList(list);
    EXPECT_EQ(fm->m_instFileList.count(), 4);
    EXPECT_EQ(true, fm->m_instFileList.contains("first"));

    list.clear();
    list << "fifth";
    fm->setInstallFileList(list);
    EXPECT_EQ(fm->m_instFileList.count(), 1);
    EXPECT_EQ(true, fm->m_instFileList.contains("fifth"));
//    delete fm;
}

TEST_F(TestDfontmanager, checkSetReInstallFile)
{
    QString reinstFile = "reinstFile";
    QString sysFile = "sysFile";

    fm->setReInstallFile(reinstFile, sysFile);
    EXPECT_EQ(true, fm->m_reinstFile == "reinstFile");
    EXPECT_EQ(true, fm->m_sysFile == "sysFile");
//    delete fm;
}

TEST_F(TestDfontmanager, checkSetUnInstallFile)
{
    QStringList list;
    list << "first" << "second" << "third" << "fouth";
    fm->setUnInstallFile(list);
    EXPECT_EQ(fm->m_uninstFile.count(), 4);
    EXPECT_EQ(true, fm->m_uninstFile.contains("first"));
//    delete fm;
}

TEST_F(TestDfontmanager, checkSetSystemFontCount)
{
    fm->setSystemFontCount(6);
    EXPECT_EQ(6, fm->m_systemFontCount);
//    delete fm;
}

TEST_F(TestDfontmanager, checkDoCache)
{
    QSignalSpy spy(fm, SIGNAL(cacheFinish()));
    fm->doCache();
    EXPECT_EQ(1, spy.count());
}

TEST_F(TestDfontmanager, checkSetCacheStatus)
{
    fm->setCacheStatus(DFontManager::CacheNow);
    EXPECT_EQ(fm->m_CacheStatus, DFontManager::Install);
}

TEST_F(TestDfontmanager, checkStop)
{
    fm->stop();
    EXPECT_EQ(true, fm->m_IsNeedStop);
}

TEST_F(TestDfontmanager, checkDoInstallNoStop)
{
    QSignalSpy spy(fm, SIGNAL(batchInstall(QString, double)));

    QStringList list;

    QString str = "/home/zhaogongqiang/Desktop/1048字体/Addictype-Regular.otf|Addictype";
    list << str;

    fm->doInstall(list, false);
    EXPECT_EQ(1, spy.count());


    QList<QVariant> arguments = spy.takeFirst();

    EXPECT_EQ("Addictype", arguments.at(0).toString());


    QDir d(sysDir + "/" + str.split("|").last());
    EXPECT_EQ(true, d.exists());

    d.removeRecursively();
}

TEST_F(TestDfontmanager, checkDoUnstallNoStop)
{
    QSignalSpy spy(fm, SIGNAL(uninstallFontFinished(QStringList)));
    QSignalSpy spys(fm, SIGNAL(uninstallFcCacheFinish()));

    QStringList list;

    QString str = QDir::homePath() + "/.local/share/fonts/Addictype";
    QString filePathOrig = QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf";
//    QFile::copy(filePathOrig, str);
//    qDebug() << str << endl;
    QDir d;
    //新建文件夹并确认创建成功
    d.mkdir(str);
    EXPECT_EQ(true, d.exists());
    QFile::copy(filePathOrig, str + "/Addictype-Regular.otf");

    list << str + "/Addictype-Regular.otf";
    fm->doUninstall(list);
    QDir dd(str);
    EXPECT_EQ(false, dd.exists());

}

TEST_F(TestDfontmanager, checkHandleReInstall)
{
    QSignalSpy spys(fm, SIGNAL(reInstallFinished(int, QStringList)));
    QStringList list ;
    QString filePathOrig = QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf" + "|Addictype";
    list << filePathOrig;
    fm->setInstallFileList(list);
    fm->setType(DFontManager::ReInstall);
    fm->handleReInstall();

    EXPECT_EQ(1, spys.count());
    QList<QVariant> arguments = spys.takeFirst();
    EXPECT_EQ(true, arguments.first().toInt() == 0);
    EXPECT_EQ(true, fm->m_instFileList.isEmpty());
}

TEST_F(TestDfontmanager, checkHandleUnInstall)
{
    Stub s;
    s.set(ADDR(DFontManager, doCmd), stub_Handle);
    QStringList list;
    list << "first" << "endl";

    fm->setUnInstallFile(list);
    fm->handleUnInstall();
    EXPECT_EQ(true, fm->m_instFileList.isEmpty());

}

TEST_F(TestDfontmanager, checkHandleInstall)
{
    QSignalSpy spys(fm, SIGNAL(installFinished(int, QStringList)));

//    Stub s;
//    s.set(ADDR(DFontManager, doCmd), stub_Docmd);

    QStringList list;
    QString filePathOrig = QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf" + "|Addictype";
    list << filePathOrig;

    fm->setInstallFileList(list);
    fm->setCacheStatus(DFontManager::CacheNow);
    fm->setType(DFontManager::Install);
    fm->handleInstall(false);
    QList<QVariant> arguments = spys.takeFirst();

//    EXPECT_EQ(true, spys.count() == 1);
//    EXPECT_EQ(DFontManager::InstallSuccess, arguments.first().toInt());

    EXPECT_EQ(true, fm->m_instFileList.isEmpty());
}

TEST_F(TestDfontmanager, checkDoCmd)
{
    QSignalSpy spys(fm, SIGNAL(installFinished(int, QStringList)));

//    Stub s;
//    s.set(ADDR(DFontManager, doCmd), stub_Docmd);

    QSignalSpy spy(fm, SIGNAL(batchInstall(QString, double)));
    QStringList list;
    QString filePathOrig = QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf" + "|Addictype";
    list << filePathOrig;

    fm->setInstallFileList(list);
    fm->setCacheStatus(DFontManager::CacheNow);
    fm->setType(DFontManager::HalfwayInstall);
    fm->doCmd(list);
    EXPECT_EQ(1, spy.count());


    QSignalSpy spy1(fm, SIGNAL(uninstallFontFinished(QStringList)));
    QSignalSpy spys1(fm, SIGNAL(uninstallFcCacheFinish()));

//    QStringList list;

    QString str = QDir::homePath() + "/.local/share/fonts/Addictype";
    filePathOrig = QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf";
//    QFile::copy(filePathOrig, str);
//    qDebug() << str << endl;
    QDir d;
    //新建文件夹并确认创建成功
    d.mkdir(str);
    EXPECT_EQ(true, d.exists());
    QFile::copy(filePathOrig, str + "/Addictype-Regular.otf");

    list << str + "/Addictype-Regular.otf";
    fm->doUninstall(list);
    QDir dd(str);
    EXPECT_EQ(false, dd.exists());
}

////需要修改
//TEST_F(TestDfontmanager, checkRunInstall)
//{
////    Stub s;
////    s.set(ADDR(DFontManager, handleInstall), stub_Handle);
////    s.set(ADDR(DFontManager, handleReInstall), stub_Handle);
////    s.set(ADDR(DFontManager, handleUnInstall), stub_Handle);
//////       s.set(ADDR(DFontManager, handleInstall), stub_Handle);

////    fm->start();

//    fm->setType(DFontManager::Install);
//    fm->start();

////    fm->setType(DFontManager::HalfwayInstall);
////    fm->start();

////    fm->setType(DFontManager::ReInstall);
////    fm->start();

////    fm->setType(DFontManager::UnInstall);
////    fm->start();
//}

//TEST_F(TestDfontmanager, checkRunHalfwayInstall)
//{
////    Stub s;
////    s.set(ADDR(DFontManager, handleInstall), stub_Handle);
//////    s.set(ADDR(DFontManager, handleReInstall), stub_Handle);
//////    s.set(

//    fm->setType(DFontManager::HalfwayInstall);
//    fm->start();
//}

//TEST_F(TestDfontmanager, checkRunUnInstall)
//{
////    Stub s;
//////    s.set(ADDR(DFontManager, handleInstall), stub_Handle);
//////    s.set(ADDR(DFontManager, handleReInstall), stub_Handle);
////    s.set(ADDR(DFontManager, handleUnInstall), stub_Handle);
////////       s.set(ADDR(DFontManager, handleInstall), stub_Handle);

////    fm->start();

//    fm->setType(DFontManager::UnInstall);
//    fm->start();
//}

//TEST_F(TestDfontmanager, checkRunReInstall)
//{
////    Stub s;

////    s.set(ADDR(DFontManager, handleReInstall), stub_Handle);

//    fm->setType(DFontManager::ReInstall);
//    fm->start();
//}







