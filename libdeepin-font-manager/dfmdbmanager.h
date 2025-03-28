/*
 *
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

#ifndef DFMDBMANAGER_H
#define DFMDBMANAGER_H

#include "dsqliteutil.h"
#include "dfontpreviewitemdef.h"

class DFMDBManager: public QObject
{
    Q_OBJECT


public:
    static DFMDBManager *instance();
    static QList<QMap<QString, QString>> recordList;
    explicit DFMDBManager(QObject *parent = nullptr);
    ~DFMDBManager();

    QList<DFontPreviewItemData> getAllFontInfo(QList<DFontPreviewItemData> *deletedFontInfo = nullptr);
    QList<DFontPreviewItemData> getFontInfo(const int count, QList<DFontPreviewItemData> *deletedFontInfo = nullptr);
    QList<DFontPreviewItemData> getFontInfo(QList<QMap<QString, QString>> list, QList<DFontPreviewItemData> *deletedFontInfo = nullptr);
    int getRecordCount();
    int getCurrMaxFontId();
    QStringList getInstalledFontsPath();
    QString isFontInfoExist(const DFontInfo &newFileFontInfo);
    // 获取指定字体文件的fontname列表
    QStringList getSpecifiedFontName(const QString &filePath);
    bool addFontInfo(const DFontPreviewItemData &itemData);
    bool updateFontInfo(const QMap<QString, QString> &whereMap, const QMap<QString, QString> &dataMap);

    void updateSP3FamilyName(const QList<DFontInfo> &fontList, bool inFontList = false);

    // batch operation
    void commitAddFontInfo();
    void addFontInfo(const QList<DFontPreviewItemData> &fontList);
    void deleteFontInfo(const DFontPreviewItemData &itemData);
    void deleteFontInfo(const QList<DFontPreviewItemData> &fontList);
    void commitDeleteFontInfo();
    void updateFontInfo(const DFontPreviewItemData &itemData, const QString &strKey);
    void updateFontInfo(const QList<DFontPreviewItemData> &fontList, const QString &strKey);
    void commitUpdateFontInfo();
    void getAllRecords();
    void syncOldRecords();
    //去除非法记录
    void checkIfEmpty();
    //开启事务
    void beginTransaction()
    {
        m_sqlUtil->m_db.transaction();
    }
    //关闭事务
    void endTransaction()
    {
        m_sqlUtil->m_db.commit();
    }

    bool isSystemFont(const QString &filePath)
    {
        return filePath.contains("/usr/share/fonts/");
    }

    //获取数据库是否被清空重建
    bool isDBDeleted(){return m_sqlUtil->isDBDeleted();}
private:
    DFontPreviewItemData parseRecordToItemData(const QMap<QString, QString> &record);
    QMap<QString, QString> mapItemData(DFontPreviewItemData itemData);
    DFontInfo getDFontInfo(const QMap<QString, QString> &record);
    inline void appendAllKeys(QList<QString> &keyList);

    DSqliteUtil *m_sqlUtil;
    QList<DFontPreviewItemData> m_addFontList;
    QList<DFontPreviewItemData> m_delFontList;
    QList<DFontPreviewItemData> m_updateFontList;
    QString m_strKey;
};

#endif // DFMDBMANAGER_H
