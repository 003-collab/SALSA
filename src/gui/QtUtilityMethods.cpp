/*
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
*/
#include "QtUtilityMethods.hpp"

#include <QDir>
#include <QFontDatabase>

bool recursiveCopydir(QString sourcePath, QString targetPath)
{
    QDir sourceDir(sourcePath);
    QFileInfoList fileInfoList = sourceDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    // Create the target directory if it doesn't already exist
    QDir targetDir(targetPath);

    if (!targetDir.exists())
    {
        QDir parentDir(targetPath);
        parentDir.cdUp();

        QString targetDirName = targetDir.dirName();
        parentDir.mkdir(targetDirName);
    }

    bool success = true;

    foreach (QFileInfo fileInfo, fileInfoList)
    {
        if (fileInfo.isFile())
        {
            QString currentFilename = sourcePath + "/" + fileInfo.fileName();
            QString newFilename = targetPath + "/" + fileInfo.fileName();

            QFile currentFile(currentFilename);

            // If newFilename already exists, delete it so the copy will overwrite it
            QFile newFile(newFilename);
            if (newFile.exists())
            {
                newFile.remove();
            }

            success = success && currentFile.copy(newFilename);
        }
        else if(fileInfo.isDir())
        {
            QFileInfo targetFileInfo(targetPath);

            if (fileInfo == targetFileInfo)
                continue; // avoid infinite recursion

            QString newSourcePath = sourcePath + "/" + fileInfo.fileName();
            QString newtargetPath = targetPath + "/" + fileInfo.fileName();

            QDir targetDir(targetPath);
            targetDir.mkdir(newtargetPath);
            success = success && recursiveCopydir(newSourcePath, newtargetPath);
        }
        else
        {
            // we found something that is neither a file nor a directory
            success = false;
        }
    }

    return success;
}


qint64 recursiveDirSize_bytes(QString dirPath)
{
    QDir currentDir(dirPath);

    if (!currentDir.exists()) return 0;

    qint64 sizeBytes = 0;

    QFileInfoList entityInfoList = currentDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    foreach (QFileInfo entityInfo, entityInfoList)
    {
        if (entityInfo.isFile())
        {
            qint64 fileSize = entityInfo.size();
            sizeBytes += fileSize;
        }
        else if (entityInfo.isDir())
        {
            sizeBytes += recursiveDirSize_bytes(entityInfo.absoluteFilePath());
        }
    }

    return sizeBytes;
}

int getNumFiles(QString dirPath)
{
    QDir currentDir(dirPath);

    if (!currentDir.exists()) return 0;

    QFileInfoList entityInfoList = currentDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    int num_files = entityInfoList.size();

    foreach (QFileInfo entityInfo, entityInfoList)
    {
        if (entityInfo.isDir())
            num_files += getNumFiles(entityInfo.absoluteFilePath());
    }

    return num_files;
}

bool renameFile(QString oldName, QString newName)
{
    QFile oldFile(oldName);
    if (!oldFile.exists()) return false;

    // Delete the new file if it exists
    QFile newFile(newName);
    if (newFile.exists())
        newFile.remove();

    // Rename the old file
    return oldFile.rename(newName);
}

/**
 * @brief getSystemFontSize Determines the value of the system font size. Most fonts in SALSA
 * were hard-coded to 12pt, but that paradigm led to inconsistencies between OS after the Qt5.15 update.
 * This method now dynamically determine a base font point size based on OS settings.
 * @return int system font size
 */
int getIdealFontSize()
{
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize();
}
