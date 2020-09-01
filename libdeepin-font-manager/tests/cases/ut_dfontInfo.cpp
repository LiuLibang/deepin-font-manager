#include "dfontinfomanager.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QProcess>
#include <QDir>


namespace  {

class TestDFontInfo : public testing::Test
{
public:

    void SetUp() override
    {
        fontInfo.familyName = "familyname_one";
        fontInfo.styleName = "stylename_one";
        fontInfo.psname = "psname_one";
        fontInfo.trademark = "trademark_one";
        fontInfo.fullname = "fullname_one";

        fontinfo2.familyName = "familyname_two";
        fontinfo2.styleName = "stylename_two";
    }
    void TearDown() override
    {

    }

public:
    DFontInfo fontInfo;
    DFontInfo fontinfo2;
};

class TestDFontInfoGetFileNames : public testing::Test
{
public:
    virtual void SetUp()
    {
        dfm = DFontInfoManager::instance();
    }
    virtual void TearDown()
    {
//        if (dfm != nullptr) {
//        delete dfm;
//            dfm = nullptr;
//        }

//        returnList.clear();
    }
public:
    DFontInfoManager *dfm = nullptr;
    //任何一个存在字体的路径
    QString path = "/usr/share/fonts/truetype/lohit-devanagari";

    //任何不存在的一个路径，用以提供判断
    QString path2 = "/usr/share/abc";

    //
    QString path3 = "/usr/share/fonts/X11/Type1";

    QStringList returnList;
};


class TestgetFontType : public testing::Test
{
public:
    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {
//        delete dfm;
    }
public:
    DFontInfoManager *dfm = nullptr;
    DFontInfo fontinfo;
};

//参数化测试
class TestcheckStyleName : public::testing::TestWithParam<QString>
{
public:
    DFontInfoManager *dfm = DFontInfoManager::instance();
};

class TestgetFontInfo : public testing::Test
{
public:

    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
    DFontInfo fontinfo;
};

class TestCheckGetAllChineseFontList : public testing::Test
{
public:

    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
    DFontInfo fontinfo;
};

class TestCheckGetAllMonoFontList : public testing::Test
{
public:

    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
};

//getInstFontPath
class TestgetInstFontPath : public testing::Test
{
public:

    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
    QString originPath;
    QString familyName;
    QString sysDir = QDir::homePath() + "/.local/share/fonts";
    QString target;
};

class TestCheckFontIsInstalled :  public testing::Test
{

public:
    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
};

class TestCheckCurFonFamily : public testing::Test
{
public:
    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
    QString filepath = QString();
};

class TestCheckgetDefaultPreview : public testing::Test
{
public:
    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
};

class TestCheckgetFontFamilyStyle : public testing::Test
{
public:
    void SetUp() override
    {
        dfm = DFontInfoManager::instance();
    }
    void TearDown() override
    {

    }
public:
    DFontInfoManager *dfm = nullptr;
};
}

TEST_F(TestDFontInfo, equalsign_is_normal)
{
    EXPECT_EQ(false, fontInfo == fontinfo2);
    DFontInfo fontinfor3(fontInfo);
    EXPECT_EQ("familyname_one", fontinfor3.familyName);
    EXPECT_EQ("stylename_one", fontinfor3.styleName);
    EXPECT_EQ("psname_one", fontinfor3.psname);
    EXPECT_EQ("trademark_one", fontinfor3.trademark);
    EXPECT_EQ("fullname_one", fontinfor3.fullname);

//    EXPECT_EQ(true, fontInfo == fontinfo2);
}

TEST_F(TestDFontInfo, tostring_is_normal)
{
    QString str = "FontInfo : familyname_one, stylename_one, psname = psname_one, trademark = trademark_one, fullname = fullname_one";
    EXPECT_EQ(true, str == fontInfo.toString());
}

TEST_F(TestDFontInfoGetFileNames, getFileNames_Is_Normal)
{
    returnList = dfm->getFileNames(path2);

    //不含字体文件的路径返回的list断言为空
    EXPECT_EQ(true, 0 == returnList.count());

    returnList.clear();
    returnList = dfm->getFileNames(path);

//    std::cout << returnList.first().toStdString();
    //指定路径下只有一个字体文件 断言返回链表长度为一
    EXPECT_EQ(true, 1 == returnList.count());

    //指定路径下字体名称为Lohit-Devanagari.ttf，断言返回路径中含有该文件名
    EXPECT_EQ(true, returnList.first().contains("Lohit-Devanagari.ttf"));

    //.pfb格式的字体无法被检测到 getfilenames函数出现问题
    returnList.clear();
    returnList = dfm->getFileNames(path3);
    EXPECT_EQ(8, returnList.count());
    delete dfm;
}

TEST_F(TestDFontInfoGetFileNames, getAllFontPath_is_normal)
{
//  非第一次启动时获取所有字体路径，获取数目应该与在终端中执行fc-list ： file读出来的数目相同。 //这个方式应该不对//
    returnList = dfm->getAllFontPath(false);
    EXPECT_EQ(334, returnList.count()) << L"Call getAllFontPath when false returnList.size =  " << returnList.count();;

    //第一次启动时，获取字体的数目应该为用户字体文件夹下所有字体的数目加上系统字体文件夹下所有字体的数目,
    returnList.clear();
    returnList = dfm->getAllFontPath(true);
//    int count = dfm->getFileNames("/usr/share/fonts/").count() + dfm->getFileNames(QDir::homePath() + "/.local/share/fonts/").count();

//    QProcess process;
//    process.start("find /usr/share/fonts/ -name *.ttf");
//    process.waitForFinished(-1);

//    QString output = process.readAllStandardOutput();
//    QStringList lines = output.split(QChar('\n'));

//    process.start("find /usr/share/fonts/ -name *.TTF");
//    process.waitForFinished(-1);
//    output = process.readAllStandardOutput();
//    lines.append(output.split(QChar('\n')));

//    process.start("find /usr/share/fonts/ -name *.ttc");
//    process.waitForFinished(-1);
//    output = process.readAllStandardOutput();
//    lines.append(output.split(QChar('\n')));

//    process.start("find /usr/share/fonts/ -name *.pfb");
//    process.waitForFinished(-1);
//    output = process.readAllStandardOutput();
//    lines.append(output.split(QChar('\n')));

//    std::cout << "++++++++++++++++++++++++++" << lines.count();
//    EXPECT_EQ(lines.count() - 4 +  dfm->getFileNames(QDir::homePath() + "/.local/share/fonts/").count(), returnList.count());
}

TEST_F(TestgetFontType, getFontType_is_normal)
{
    EXPECT_EQ("TrueType", dfm->getFontType("/usr/share/fonts/truetype/liberation/LiberationMono-Italic.ttf"));

    EXPECT_EQ("TrueType", dfm->getFontType("/usr/share/fonts/fonts-cesi/CESI_XBS_GB18030.TTF"));

    EXPECT_EQ("TrueType", dfm->getFontType("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"));

    EXPECT_EQ("OpenType", dfm->getFontType(QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf"));

    EXPECT_EQ("Unknown", dfm->getFontType("/usr/share/fonts/X11/Type1/c0649bt_.pfb"));

    //异常检查
    EXPECT_EQ("Unknown", dfm->getFontType(""));

}

//checkStyleName 函数出错 20200806
TEST_P(TestcheckStyleName, checkStyleName_Is_Normal)
{
    QString n =  GetParam();
//    std::cout << n << "++++++++++++++++++" << std::endl;
    DFontInfo f;
    f.styleName = "?";
    f.psname = QString(n);
//    std::cout << f.psname.toStdString() << f.styleName.toStdString() << "++++++++++++++++++" << std::endl;
    dfm->checkStyleName(f);
//    std::cout << f.psname.toStdString() << f.styleName.toStdString() << "++++++++++++++++++" << std::endl;
    ASSERT_EQ(f.psname, f.styleName);
}

INSTANTIATE_TEST_CASE_P(HandleTrueReturn, TestcheckStyleName, testing::Values("Regular", "Bold", "Light", "Thin", "ExtraLight", "ExtraBold",
                                                                              "Medium", "DemiBold", "Black", "AnyStretch", "UltraCondensed",
                                                                              "ExtraCondensed", "Condensed", "SemiCondensed", "Unstretched",
                                                                              "SemiExpanded", "Expanded", "ExtraExpanded", "UltraExpanded"));

TEST_F(TestgetFontInfo, getErrorFontInfo_is_normal)
{
    //系统字体应该为已安装,执行函数得到的结果已安装为false,出现错误.
    fontinfo = dfm->getFontInfo(QDir::homePath() + "/Desktop/abc.ttf");
    EXPECT_EQ(true, fontinfo.isError);
}

TEST_F(TestgetFontInfo, getDefaultPreview_is_normal)
{
    qint8 Long = 1;
    QString str = dfm->getDefaultPreview(QDir::homePath() + "/Desktop/abc.ttf", Long);
    EXPECT_EQ(true, str.isNull());
}



//getFontInfo 函数出错 获取系统字体信息时,安装状态被检测为未安装 20200813
TEST_F(TestgetFontInfo, getSystemFontInfo_Is_normal)
{
    //系统字体应该为已安装,执行函数得到的结果已安装为false,出现错误.
    fontinfo = dfm->getFontInfo("/usr/share/fonts/truetype/noto/NotoSansTamil-Bold.ttf");
    EXPECT_EQ(false, fontinfo.isError);
    EXPECT_EQ("TrueType", fontinfo.type);
    EXPECT_EQ("Bold", fontinfo.styleName);
    EXPECT_EQ(true, fontinfo.isInstalled);
}

//这个测试不稳定,取决于这个字体有没有被安装过,以后需要进行修改***
TEST_F(TestgetFontInfo, getInstalledFontInfo_is_normal)
{
    //wolves.ttf是安装过的一个字体 , 检查结果没有问题
    fontinfo = dfm->getFontInfo(QDir::homePath() + "/Desktop/1048字体/wolves.ttf");
    EXPECT_EQ(false, fontinfo.isError);
    EXPECT_EQ("TrueType", fontinfo.type);
    EXPECT_EQ(true, fontinfo.isInstalled);
}


TEST_F(TestgetFontInfo, getChineseFontInfo_is_normal)
{
    fontinfo = dfm->getFontInfo("/usr/share/fonts/fonts-cesi/CESI_FS_GB2312.TTF");
    EXPECT_EQ(false, fontinfo.isError);
    EXPECT_EQ("TrueType", fontinfo.type);
    EXPECT_EQ(FONT_LANG_CHINESE, fontinfo.previewLang);
}

TEST_F(TestCheckGetAllChineseFontList, getAllChineseFontCount_is_normal)
{
    int count = dfm->getAllChineseFontPath().count();

    QProcess process;
    process.start("fc-list :lang=zh");
    process.waitForFinished(-1);

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split(QChar('\n'));

    EXPECT_EQ(lines.count() - 1, count);
}


TEST_F(TestCheckGetAllMonoFontList, getAllMonoFontCount_is_normal)
{
    int count = dfm->getAllMonoSpaceFontPath().count();

    QProcess process;
    process.start("fc-list :spacing=mono");
    process.waitForFinished(-1);

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split(QChar('\n'));

    EXPECT_EQ(lines.count() - 1, count);
}

//getInstFontPath函数正常未安装字体检测
TEST_F(TestgetInstFontPath, getInstallFontPath_normalfont_is_normal)
{
    originPath = QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf";
    QFileInfo dir(originPath);
    target = sysDir + "/asd/" + dir.fileName();
//    std::cout << target.toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;
//    std::cout << dfm->getInstFontPath(originPath, "asd").toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;

    EXPECT_EQ(target, dfm->getInstFontPath(originPath, "asd"));
}
//getInstFontPath函数系统字体字体检测
TEST_F(TestgetInstFontPath, getInstallFontPath_systemfont_is_normal)
{
    originPath = "/usr/share/fonts/truetype/noto/NotoSansLinearB-Regular.ttf";
    QFileInfo dir(originPath);
    target = sysDir + "/asd/" + dir.fileName();
//    std::cout << target.toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;
//    std::cout << dfm->getInstFontPath(originPath, "asd").toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;

    EXPECT_EQ(originPath, dfm->getInstFontPath(originPath, "asd"));
}

//getInstFontPath函数已安装字体字体字体检测
TEST_F(TestgetInstFontPath, getInstallFontPath_normalIntalledfont_is_normal)
{
    originPath =  QDir::homePath() + "/.local/share/fonts/Yikatu/yikatu.ttf";

    QFileInfo dir(originPath);
    target = sysDir + "/asd/" + dir.fileName();
//    std::cout << target.toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;
//    std::cout << dfm->getInstFontPath(originPath, "asd").toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;

    EXPECT_EQ(originPath, dfm->getInstFontPath(originPath, "asd"));
}

//getInstFontPath函数字体familyname为空检测
TEST_F(TestgetInstFontPath, getInstallFontPath_errorfam_is_normal)
{
    originPath =  QDir::homePath() + "/Desktop/1048字体/Addictype-Regular.otf";

    QFileInfo dir(originPath);
    target = sysDir + "/" + dir.baseName() + "/" + dir.fileName();
    std::cout << target.toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;
    std::cout << dfm->getInstFontPath(originPath, QString()).toStdString() << "+++++++++++++++++++++++++++++++++++++" << std::endl;

    EXPECT_EQ(target, dfm->getInstFontPath(originPath, QString()));
}

//isFontInstalled检测已安装字体是否已安装
TEST_F(TestCheckFontIsInstalled, fontIsInstalled_installedFont_isnormal)
{
//因为static成员 dataList没有其他办法访问,所以先调用这个函数
    dfm->refreshList();
    DFontInfo fontInfo = dfm->getFontInfo("/usr/share/fonts/truetype/noto/NotoSansLinearB-Regular.ttf");

    EXPECT_EQ(true, dfm->isFontInstalled(fontInfo));
}

//isFontInstalled检测未安装字体是否已安装
TEST_F(TestCheckFontIsInstalled, fontIsInstalled_notInstalledFont_isnormal)
{
//因为static成员 dataList没有其他办法访问,所以先调用这个函数
    dfm->refreshList();
    DFontInfo fontInfo = dfm->getFontInfo(QDir::homePath() + "/Desktop/1048字体/食物.ttf");

    EXPECT_EQ(false, dfm->isFontInstalled(fontInfo));
}

//isFontInstalled检测异常字体是否已安装
TEST_F(TestCheckFontIsInstalled, fontIsInstalled_errorFont_isnormal)
{
//因为static成员 dataList没有其他办法访问,所以先调用这个函数
    dfm->refreshList();
    DFontInfo fontInfo;
    EXPECT_EQ(false, dfm->isFontInstalled(fontInfo));
}

//getCurrentFontFamily检测正在使用的系统字体
TEST_F(TestCheckCurFonFamily, getFFamStyle)
{
    // 查看当前系统中设置的字体名,将之与函数返回值进行比较
    QString str = "Noto Sans CJK KR";
    EXPECT_EQ(true,  dfm->getCurrentFontFamily().contains(str));
}

//getDefaultPreview 检测中文字体的默认预览效果是否正常 在系统字体为英文的环境下出错 20200814 中文字体预览效果标志位不为1
TEST_F(TestCheckgetDefaultPreview, get_Chinese_DefaultPreview)
{
    //dfm->refreshList();
    DFontInfo fontInfo = dfm->getFontInfo("/usr/share/fonts/fonts-cesi/CESI_XBS_GB13000.TTF");
    dfm->getDefaultPreview(fontInfo);
    std::cout << fontInfo.previewLang << "+++++++++++++++++++++++++++++++++++++" << std::endl;
    //这个标志符表示中文字体,显示默认的中文内容.
    EXPECT_EQ(1,  fontInfo.previewLang);

    fontInfo = dfm->getFontInfo("/usr/share/fonts/fonts-cesi/CESI_KT_GB13000.TTF");
    dfm->getDefaultPreview(fontInfo);
    std::cout << fontInfo.previewLang << "+++++++++++++++++++++++++++++++++++++" << std::endl;
    //这个标志符表示中文字体,显示默认的中文内容.
    EXPECT_EQ(1,  fontInfo.previewLang);

}

//getDefaultPreview 检测英文字体的默认预览效果是否正常
TEST_F(TestCheckgetDefaultPreview, get_English_DefaultPreview)
{
    //dfm->refreshList();
    DFontInfo fontInfo = dfm->getFontInfo("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf");
    dfm->getDefaultPreview(fontInfo);
    std::cout << fontInfo.previewLang << "+++++++++++++++++++++++++++++++++++++" << std::endl;
    //这个标志符表示中文字体,显示默认的英文内容.
    EXPECT_EQ(2,  fontInfo.previewLang);
}

//getFontFamilyStyle 查看系统个性化中的字体设置,现实的是字体familyname,与结果进行对比
TEST_F(TestCheckgetFontFamilyStyle, get_Normal_FamilyStyle)
{
    QStringList str = dfm->getFontFamilyStyle("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf");
    EXPECT_EQ(true, str.contains("Liberation Sans")) << L"++++++++getFontFamilyStyle isnormal font familyname is " << str.contains("Liberation Sans");

    str = dfm->getFontFamilyStyle("/usr/share/fonts/truetype/msttcorefonts/Georgia.ttf");
    EXPECT_EQ(true, str.contains("Georgia")) << L"++++++++getFontFamilyStyle isnormal font familyname is " << str.contains("Liberation Sans");
}
















