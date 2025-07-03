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
// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <QString>
#include <QtTest>
#include <QTest>
#include <QCoreApplication>
#include <QMessageBox>
#include <QSignalSpy>
#include <QTextStream>

#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include <ui_AdjustedPositions.h>
#include <ui_StationDataDialog.h>
#include <ui_ConfigDialog.h>
#include <ui_ConverterDialog.h>
#include <ui_FindWidget.h>
#include <ui_histogramdialog.h>
#include <ui_mainwindow.h>
#include <ui_mapping.h>
#include <ui_RecordEditor.h>
#include <ui_PreferencesDialog.h>

#include <c_LSADist.hpp>
#include <FindWidget.hpp>
#include <StationDataDialog.hpp>
#include <GuiModel.hpp>
#include <GuiModelItem.hpp>
#include <LSASupportedToVersion.hpp>
#include <MainWindow.hpp>
#include <QtUtilityMethods.hpp>

#include <lsah5.hpp>

using namespace QTest;
using namespace std;

class TestLSAGui : public QObject
{
    Q_OBJECT

public:
    TestLSAGui();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // START OF TEST NAMES

    // Path Utilities
    void testCanonicalPath();

    // converter tests
    void converter_LeicaSetsOfAngles_204();
    void converter_LeicaSetsOfAngles_204HeightUncertainty();
    void converter_LeicaSetsOfAngles_222();
    void converter_LeicaSetsOfAngles_224();
    void converter_LeicaSetsOfAngles_450();
    void converter_LeicaSetsOfAngles_500();
    void converter_LeicaSetsOfAngles_505();
    void converter_LeicaSetsOfAngles_561();
    void converter_LeicaSetsOfAngles_570();
    void converter_LeicaSetsOfAngles_802();
    void converter_LeicaSetsOfAngles_803();
    void converter_LeicaSetsOfAngles_Bug1480();
    void converter_LeicaSetsOfAngles_Bug1533();
    void converter_LeicaSetsOfAngles_Bug1541();
    void converter_LeicaSetsOfAngles_Req114();
    void converter_TrimbleDiNi_B412();
    void converter_TrimbleDiNi_B412HeightUncertainty();
    void converter_TrimbleDiNi_Run1();
    void converter_TrimbleDiNi_SIXTWO();
    void converter_TrimbleDiNi_B412_disabled();
    void converter_TrimbleRounds_200226_SX10();
    void converter_TrimbleRounds_200226_SX10HeightUncertainty();
    void converter_TrimbleRounds_200226_SX10_FEET_INST();
    void converter_TrimbleRounds_200226_SX10_INST();
    void converter_TrimbleRounds_200311_RBF();
    void converter_TrimbleRounds_200505_HAIR();
    void converter_TrimbleRounds_200512_Cypress();
    void converter_TrimbleRounds_fillLoop();

    // Trimble Rounds converter tests
    void trimbleRoundsConverter1();
    void trimbleRoundsConverter1HeightUncertainty();
    void trimbleRoundsConverter1_instSigma();
    void trimbleRoundsConverter1_unitsFeet();
    void trimbleRoundsConverter2();
    void trimbleRoundsConverter3();
    void trimbleRoundsConverter4();
    void trimbleRoundsConverter5();
    void trimbleRoundsConverter6();
    void trimbleRoundsConverterTopoF1AngOnly();
    void trimbleRoundsConverterTopoF1F2AngOnly();
    void trimbleRoundsConverterTopoF1AngDist();
    void trimbleRoundsConverterTopoF1F2AngDist();
    void trimbleRoundsConverterTopoF1AngDistInst();
    void trimbleRoundsConverterTopoF1F2AngDistInst();

    //converter warning file test
    void converterWarningFile();

    // integation tests
    void example01_Integration();

    // export tests
    void export_UTM();
    void export_NCAT();
    void export_SPC();
    void export_SPC_south();
    void export_RawResidualsENU();

    void addAutogenToProject(); // Test to check adding adding autogen points to project and menu status

    //Record menu tests
    void record_Cut();

    // Text Notes tests
    void disableRecordsWithTextNotes();
    // END OF TEST NAMES

private:

    // Paths
    QString rootLSAPath;
    QString binPath;
    QString cgeoidPath;
    QString lsaInversePath;
    QString lsaPostPath;
    QString scriptsTestPath;
    QString reportingPath;
    QString convertingPath;
    QString testPath;
    QString testGuiPath;
    QString testoutputPath;
    QString testoutputGuiPath;
    QString pythonExe;

    std::string setupMessage;

    void setupPathVars();
    bool setupTestDir(QString dirName);
    void getPostprocessorLoop(MainWindow *mainWindow, QEventLoop *eventLoop);
    bool diff_files(QString expected_fname, QString output_fname, QString &errMsg);
    bool setupLeicaSetsOfAnglesConverterTest(QString testVersion, QString &failure_message);
    bool setupTrimbleTDEFConverterTest(QString testType, QString testVersion, QString &failure_message);
    void setupLsaIntegrationTest(MainWindow &mainWindow, QString testName);
    bool setupConverterTestDir(QString converterType, QString specTest);
    bool setupTrimbleRoundsConverterTest(QString specTest, QString &failure_message);
    void asciiDiffFiles(QString expected, QString actual);
};

TestLSAGui::TestLSAGui()
{

}


void TestLSAGui::initTestCase()
{
    QCoreApplication::setOrganizationName("ARLUT");
    QCoreApplication::setOrganizationDomain("arlut.utexas.edu");
    QCoreApplication::setApplicationName("SALSAPublicGuiTest");
    qputenv("salsaAppName","SALSAPublicGuiTest");

    // Specify the ini file so it will be saved in the application directory
    // rather than at the user or system level.
    // This allows multiple pipelines to run on gitlab CI without
    // conflicts with QSettings.
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
}

void TestLSAGui::cleanupTestCase()
{

}

void TestLSAGui::setupPathVars()
{

    rootLSAPath.clear();
    rootLSAPath       = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../");

    binPath.clear();
    binPath           = QDir::cleanPath(QCoreApplication::applicationDirPath());

    scriptsTestPath.clear();
    scriptsTestPath   = QDir::cleanPath(rootLSAPath + "/scripts/test/");

    reportingPath.clear();
    reportingPath     = QDir::cleanPath(rootLSAPath + "/scripts/reporting/");

    convertingPath.clear();
    convertingPath    = QDir::cleanPath(rootLSAPath + "/scripts/converters/");

    testPath.clear();
    testPath          = QDir::cleanPath(rootLSAPath + "/publicTest/");

    testGuiPath.clear();
    testGuiPath       = QDir::cleanPath(testPath + "/gui/");

    testoutputPath.clear();
    testoutputPath    = QDir::cleanPath(rootLSAPath + "/publicTestOutput/");

    testoutputGuiPath.clear();
    testoutputGuiPath = QDir::cleanPath(testoutputPath + "/gui/");

#ifdef _WIN32
    cgeoidPath = QDir::cleanPath(binPath + "/cgeoid.exe");
    lsaInversePath = QDir::cleanPath(binPath + "/lsainverse.exe");
    lsaPostPath = QDir::cleanPath(binPath + "/lsapost.exe");
#else
    cgeoidPath = QDir::cleanPath(binPath + "/cgeoid");
    lsaInversePath = QDir::cleanPath(binPath + "/lsainverse");
    lsaPostPath = QDir::cleanPath(binPath + "/lsapost");
#endif


    pythonExe.clear();
#ifdef _WIN32
    pythonExe = QDir::cleanPath(rootLSAPath +  "/python/python.exe");
#else
    pythonExe = QDir::cleanPath(qgetenv("SALSA_PYTHON_EXE"));
#endif

    setupMessage = "Test directory failed to initialize correctly.";
}

void TestLSAGui::asciiDiffFiles(QString expected, QString actual)
{
    QDir::setCurrent(scriptsTestPath);
    QProcess diffProcess;
    QString exe(pythonExe);
    QStringList diffArgs;
    diffArgs << scriptsTestPath + "/lsaAsciiDiff.py"
             << expected
             << actual;

    diffProcess.start(exe,diffArgs);
    diffProcess.waitForFinished();

    QString diffError = diffProcess.readAllStandardError();
    QString diffOutput = diffProcess.readAllStandardOutput();

    QVERIFY2(diffError.isEmpty(), diffError.toStdString().c_str() );
    QVERIFY2(diffOutput == QString("0"), diffOutput.toStdString().c_str() );
}

bool TestLSAGui::setupTestDir(QString dirName)
{
    setupPathVars();

    QString copyFromPath = QDir::cleanPath(testGuiPath + "/" + dirName);
    QString copyToPath = QDir::cleanPath(testoutputGuiPath + "/" + dirName);

    // delete the testoutput directory
    QDir copyToDir(copyToPath);
    bool success = copyToDir.removeRecursively();

    // re-create lsa/testoutput/gui
    QDir testoutputGuiDir(testoutputGuiPath);
    success = testoutputGuiDir.mkpath(copyToPath);

    // copy the contents of test to testoutput
    success = recursiveCopydir(copyFromPath, copyToPath);

    return success;
}

bool TestLSAGui::setupLeicaSetsOfAnglesConverterTest(QString testVersion, QString &failure_message)
{
    //define failure messages and test type
    QString testType = "LeicaTotalStation";
    QString extension = "log";
    QString setupMessage = "Converter test " + testType + testVersion + " FAILED!";
    QString failureMessage1 = "setupTestDir failure.";
    QString failureMessage2 = "Failure running converter script.";
    QString failureMessage3 = "Failure in diff of expected vs generated parent.";
    QString failureMessage4 = "Failure in diff of expected vs generated child include.";

    if(!setupTestDir("converters/" + testType + "/" + testVersion))
    {
        failure_message = setupMessage + " " + failureMessage1 + "\n";
        return false;
    }

    //define paths
    QString converterScript = convertingPath + "/converterlauncher.py";
    QString converterDiffScript = scriptsTestPath + "/converterDiff.py";
    QString converterTestoutputGuiPath = testoutputGuiPath + "/converters/" + testType + "/" + testVersion;
    QString inputFilePath = converterTestoutputGuiPath + "/" + testType + testVersion + "." + extension;
    QString warnFilePath = converterTestoutputGuiPath + "/" + testType + testVersion + ".wrn";
    QString truthFilePath = converterTestoutputGuiPath + "/" + testType + testVersion + ".expected.lsa";
    QString outputFilePath = converterTestoutputGuiPath + "/" + testType + testVersion + ".lsa";
    QString cfgFilePath = converterTestoutputGuiPath + "/converter.cfg";

    //run the converter
    QDir::setCurrent(scriptsTestPath);
    QProcess converterProcess;
    QString exe(pythonExe);
    QStringList converterArgs;
    converterArgs << converterScript
             << "--in" << inputFilePath
             << "--out" << outputFilePath
             << "--cfgfile" << cfgFilePath
             << "--wrnfile" << warnFilePath;
    converterProcess.start(exe, converterArgs);
    converterProcess.waitForFinished();

    // Return the results (for debugging)
    QString converterError = converterProcess.readAllStandardError();
    QString converterOutput = converterProcess.readAllStandardOutput();
    int converterReturnCode = converterProcess.exitCode();
    QProcess::ExitStatus converterExitStatus = converterProcess.exitStatus();

    if(((converterExitStatus == QProcess::NormalExit) && (converterReturnCode != 0)) ||
       (converterExitStatus != QProcess::NormalExit) ||
       (converterError.size() > 0))
    {
        failure_message = setupMessage + " " + failureMessage2 + "\n" + converterOutput + "\n" + converterError;
        return false;
    }

    //run the difference tool
    QProcess diffProcess;
    QStringList diffArgs;
    diffArgs << converterDiffScript << truthFilePath << outputFilePath;
    diffProcess.start(exe, diffArgs);
    diffProcess.waitForFinished();

    //return the results
    QString diffError = diffProcess.readAllStandardError();
    QString diffOutput = diffProcess.readAllStandardOutput();
    int diffReturnCode = diffProcess.exitCode();
    QProcess::ExitStatus diffExitStatus = diffProcess.exitStatus();

    if(((diffExitStatus == QProcess::NormalExit) && (diffReturnCode != 0)) ||
       (diffExitStatus != QProcess::NormalExit) ||
       (diffError.size() > 0))
    {
        failure_message = setupMessage + " " + failureMessage3 + "\n" + diffOutput + "\n" + diffError;
        return false;
    }

    //difference each of the child includes of the main output file
    QFile infile(outputFilePath);
    QString line;
    QList<QString> childIncludes;
    QString match_str="--include";
    if(infile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&infile);
        while(!in.atEnd())
        {
            line = in.readLine();
            if(line.startsWith("--include"))
            {
                QString include = QString::fromStdString(line.toStdString().substr(10));
                childIncludes.append(include);
            }
        }
        infile.close();
    }
    foreach(QString include, childIncludes)
    {
        QString include_name = include.remove(QChar('"'));
        include_name = include_name.remove(QChar('\n'));
        std::string include_name_str = include_name.toStdString();
        unsigned long begin = include_name_str.find('_')+1;
        unsigned long end = include_name_str.rfind('.');
        QString station = QString::fromStdString(include_name_str.substr(begin,end-begin));
        std::string truth_file_str = truthFilePath.toStdString();
        end = truth_file_str.find(".expected");
        QString expected_file = QString::fromStdString(truth_file_str.substr(0,end)) + "_" + station + ".expected.lsa";
        if(expected_file.contains(QChar(' ')))
        {
            expected_file = "\"" + expected_file + "\"";
        }
        QFileInfo output_file(outputFilePath);
        QString test_file = output_file.absolutePath() + "/" + include_name;
        if(test_file.contains(QChar(' ')))
        {
            test_file = "\"" + test_file + "\"";
        }
        diffArgs.clear();
        diffArgs << converterDiffScript << expected_file << test_file;
        diffProcess.start(exe, diffArgs);
        diffProcess.waitForFinished();

        diffError = diffProcess.readAllStandardError();
        diffOutput = diffProcess.readAllStandardOutput();
        diffReturnCode = diffProcess.exitCode();
        diffExitStatus = diffProcess.exitStatus();

        if(((diffExitStatus == QProcess::NormalExit) && (diffReturnCode != 0)) ||
           (diffExitStatus != QProcess::NormalExit) ||
           (diffError.size() > 0))
        {
            failure_message = setupMessage + " " + failureMessage4 + "\n" + diffOutput + "\n" + diffError;
            return false;
        }
    }

    return true;
}

bool TestLSAGui::setupTrimbleTDEFConverterTest(QString testType, QString testVersion, QString &failure_message)
{
    //define failure messages and test type
    QString extension = "asc";
    QString setupMessage = "Converter test " + testType + testVersion + " FAILED!";
    QString failureMessage1 = "setupTestDir failure.";
    QString failureMessage2 = "Failure running converter script.";
    QString failureMessage3 = "Failure in diff of expected vs generated parent.";
    QString failureMessage4 = "Failure in diff of expected vs generated child include.";

    if(!setupTestDir("converters/" + testType + "/" + testVersion))
    {
        failure_message = setupMessage + " " + failureMessage1 + "\n";
        return false;
    }

    //define paths
    QString converterScript = convertingPath + "/converterlauncher.py";
    QString converterDiffScript = scriptsTestPath + "/converterDiff.py";
    QString converterTestoutputGuiPath = testoutputGuiPath + "/converters/" + testType + "/" + testVersion;
    QString inputFilePath = converterTestoutputGuiPath + "/" + testVersion + "." + extension;
    QString warnFilePath = converterTestoutputGuiPath + "/" + testVersion + ".wrn";
    QString truthFilePath = converterTestoutputGuiPath + "/" + testVersion + ".expected.lsa";
    QString outputFilePath = converterTestoutputGuiPath + "/" + testVersion + ".lsa";
    QString cfgFilePath = converterTestoutputGuiPath + "/converter.cfg";

    //run the converter
    QDir::setCurrent(scriptsTestPath);
    QProcess converterProcess;
    QString exe(pythonExe);
    QStringList converterArgs;
    converterArgs << converterScript
             << "--in" << inputFilePath
             << "--out" << outputFilePath
             << "--type" << "TRIMBLE"
             << "--cfgfile" << cfgFilePath
             << "--wrnfile" << warnFilePath;
    converterProcess.start(exe, converterArgs);
    converterProcess.waitForFinished();

    // Return the results (for debugging)
    QString converterError = converterProcess.readAllStandardError();
    QString converterOutput = converterProcess.readAllStandardOutput();
    int converterReturnCode = converterProcess.exitCode();
    QProcess::ExitStatus converterExitStatus = converterProcess.exitStatus();

    if(((converterExitStatus == QProcess::NormalExit) && (converterReturnCode != 0)) ||
       (converterExitStatus != QProcess::NormalExit) ||
       (converterError.size() > 0))
    {
        failure_message = setupMessage + " " + failureMessage2 + "\n" + converterOutput + "\n" + converterError;
        return false;
    }

    //run the difference tool
    QProcess diffProcess;
    QStringList diffArgs;
    diffArgs << converterDiffScript << truthFilePath << outputFilePath;
    diffProcess.start(exe, diffArgs);
    diffProcess.waitForFinished();

    //return the results
    QString diffError = diffProcess.readAllStandardError();
    QString diffOutput = diffProcess.readAllStandardOutput();
    int diffReturnCode = diffProcess.exitCode();
    QProcess::ExitStatus diffExitStatus = diffProcess.exitStatus();

    if(((diffExitStatus == QProcess::NormalExit) && (diffReturnCode != 0)) ||
       (diffExitStatus != QProcess::NormalExit) ||
       (diffError.size() > 0))
    {
        failure_message = setupMessage + " " + failureMessage3 + "\n" + diffOutput + "\n" + diffError;
        return false;
    }

    //difference each of the child includes of the main output file
    QFile infile(outputFilePath);
    QString line;
    QList<QString> childIncludes;
    QString match_str="--include";
    if(infile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&infile);
        while(!in.atEnd())
        {
            line = in.readLine();
            if(line.startsWith("--include"))
            {
                QString include = QString::fromStdString(line.toStdString().substr(10));
                childIncludes.append(include);
            }
            else if(line.startsWith("#--include"))
            {
                QString include = QString::fromStdString(line.toStdString().substr(11));
                childIncludes.append(include);
            }
        }
        infile.close();
    }
    foreach(QString include, childIncludes)
    {
        QString include_name = include.remove(QChar('"'));
        include_name = include_name.remove(QChar('\n'));
        std::string include_name_str = include_name.toStdString();
        unsigned long point=0;
        if(include_name.contains(".pos.lsa"))
        {
            point = include_name_str.find(".pos.lsa");
        }
        else if(testType == QString("TrimbleDiNi"))
        {
            point = include_name_str.find(".lvl.lsa");
        }
        else if (testType == QString("TrimbleRoundsTDEF"))
        {
            point = include_name_str.find(".rnd.lsa");
        }
        QString expected_file = QString::fromStdString(include_name_str.substr(0,point)) + ".expected" + QString::fromStdString(include_name_str.substr(point,include_name_str.length()-point));
        if(expected_file.contains(QChar(' ')))
        {
            expected_file = "\"" + expected_file + "\"";
        }
        QFileInfo output_file(outputFilePath);
        QString test_file = output_file.absolutePath() + "/" + include_name;
        expected_file = output_file.absolutePath() + "/" + expected_file;
        if(test_file.contains(QChar(' ')))
        {
            test_file = "\"" + test_file + "\"";
        }
        diffArgs.clear();
        diffArgs << converterDiffScript << expected_file << test_file;
        diffProcess.start(exe, diffArgs);
        diffProcess.waitForFinished();

        diffError = diffProcess.readAllStandardError();
        diffOutput = diffProcess.readAllStandardOutput();
        diffReturnCode = diffProcess.exitCode();
        diffExitStatus = diffProcess.exitStatus();

        if(((diffExitStatus == QProcess::NormalExit) && (diffReturnCode != 0)) ||
           (diffExitStatus != QProcess::NormalExit) ||
           (diffError.size() > 0))
        {
            failure_message = setupMessage + " " + failureMessage4 + "\n" + diffOutput + "\n" + diffError;
            return false;
        }
    }

    QFileInfo wrnFile(warnFilePath);
    if(wrnFile.exists())
    {
        std::string warn_name_str = warnFilePath.toStdString();
        unsigned long point = warn_name_str.find(".wrn");
        QString expected_file = QString::fromStdString(warn_name_str.substr(0, point)) + ".expected" + QString::fromStdString(warn_name_str.substr(point, warn_name_str.length()-point));
        if(expected_file.contains(QChar(' ')))
        {
            expected_file = "\"" + expected_file + "\"";
        }
        //expected_file = wrnFile.absolutePath() + "/" + expected_file;
        diffArgs.clear();
        diffArgs << converterDiffScript << expected_file << warnFilePath;
        diffProcess.start(exe, diffArgs);
        diffProcess.waitForFinished();

        diffError = diffProcess.readAllStandardError();
        diffOutput = diffProcess.readAllStandardOutput();
        diffReturnCode = diffProcess.exitCode();
        diffExitStatus = diffProcess.exitStatus();

        if(((diffExitStatus == QProcess::NormalExit) && (diffReturnCode != 0)) ||
           (diffExitStatus != QProcess::NormalExit) ||
           (diffError.size() > 0))
        {
            failure_message = setupMessage + " " + failureMessage4 + "\n" + diffOutput + "\n" + diffError;
            return false;
        }
    }

    return true;
}

void TestLSAGui::getPostprocessorLoop(MainWindow *mainWindow, QEventLoop *eventLoop)
{
    connect(mainWindow,SIGNAL(postprocessorFinished()),eventLoop,SLOT(quit()));
}

bool TestLSAGui::diff_files(QString expected_fname, QString output_fname, QString &errMsg)
{
    QFile expected_file(expected_fname);
    if (!expected_file.open(QIODevice::ReadOnly | QIODevice::Text)){
        errMsg = "Unable to open " + expected_fname + "!";
        return false;
    }

    QFile output_file(output_fname);
    if (!output_file.open(QIODevice::ReadOnly | QIODevice::Text)){
        errMsg = "Unable to open " + output_fname + "!";
        expected_file.close();
        return false;
    }

    QTextStream in1(&expected_file), in2(&output_file);

    while ( !in1.atEnd() && !in2.atEnd() ) {
        QString line1 = in1.readLine();
        QString line2 = in2.readLine();
        if (QString::compare(line1, line2) != 0)
        {
            errMsg = "Expected: " + line1 + "\n   Found: " + line2;
            expected_file.close();
            output_file.close();
            return false;
        }
    }

    expected_file.close();
    output_file.close();
    return true;
}

void TestLSAGui::setupLsaIntegrationTest(MainWindow &mainWindow, QString testName)
{
    QVERIFY2(setupTestDir("integration/" + testName), setupMessage.c_str());

    QString fileName(testoutputPath + "/gui/integration/" + testName + "/" + testName + ".proj");
    QEventLoop postprocessorLoop;

    getPostprocessorLoop(&mainWindow,&postprocessorLoop);
    mainWindow.loadLsaFile(fileName);
    mainWindow.calculateAdjustment();
    postprocessorLoop.exec();

    return;
}


void TestLSAGui::testCanonicalPath()
{
    setupPathVars();

    QString inputString;
    QString testString;

    QString truthString = testGuiPath + "/paths/fileA.txt";

    inputString = rootLSAPath + "/build/../publicTest/gui/paths/fileA.txt";
    testString = QString::fromStdString(getCanonicalPath(inputString.toStdString()));
    QCOMPARE(testString, truthString);

    inputString = rootLSAPath + "/publicTest/./gui/paths/fileA.txt";
    testString = QString::fromStdString(getCanonicalPath(inputString.toStdString()));
    QCOMPARE(testString, truthString);

    inputString = rootLSAPath + "/publicTest/gui/paths/fileA.txt";
    testString = QString::fromStdString(getCanonicalPath(inputString.toStdString()));
    QCOMPARE(testString, truthString);

}

void TestLSAGui::converter_LeicaSetsOfAngles_204()
{
    QString testVersion = "204";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_204HeightUncertainty()
{
    QString testVersion = "204HeightUncertainty";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_222()
{
    QString testVersion = "222";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_224()
{
    QString testVersion = "224";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_450()
{
    QString testVersion = "450";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_500()
{
    QString testVersion = "500";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_505()
{
    QString testVersion = "505";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_561()
{
    QString testVersion = "561";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_570()
{
    QString testVersion = "570";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_802()
{
    QString testVersion = "802";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_803()
{
    QString testVersion = "803";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_Bug1480()
{
    QString testVersion = "Bug1480";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_Bug1533()
{
    QString testVersion = "Bug1533";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_Bug1541()
{
    QString testVersion = "Bug1541";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_LeicaSetsOfAngles_Req114()
{
    QString testVersion = "Req114";
    QString failure_message;

    bool success = setupLeicaSetsOfAnglesConverterTest(testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleDiNi_B412()
{
    QString testVersion = "B412";
    QString failure_message;
    QString testType = "TrimbleDiNi";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleDiNi_B412HeightUncertainty()
{
    QString testVersion = "B412HeightUncertainty";
    QString failure_message;
    QString testType = "TrimbleDiNi";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleDiNi_Run1()
{
    QString testVersion = "Run1";
    QString failure_message;
    QString testType = "TrimbleDiNi";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleDiNi_SIXTWO()
{
    QString testVersion = "SIXTWO";
    QString failure_message;
    QString testType = "TrimbleDiNi";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleDiNi_B412_disabled()
{
    QString testVersion = "B412_disabled";
    QString failure_message;
    QString testType = "TrimbleDiNi";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_200226_SX10()
{
    QString testVersion = "200226_SX10";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_200226_SX10HeightUncertainty()
{
    QString testVersion = "200226_SX10HeightUncertainty";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_200226_SX10_FEET_INST()
{
    QString testVersion = "200226_SX10_FEET_INST";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_200226_SX10_INST()
{
    QString testVersion = "200226_SX10_INST";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_200311_RBF()
{
    QString testVersion = "200311_RBF";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_200505_HAIR()
{
    QString testVersion = "200505_HAIR";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_200512_Cypress()
{
    QString testVersion = "200512_Cypress";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::converter_TrimbleRounds_fillLoop()
{
    QString testVersion = "fillLoop";
    QString failure_message;
    QString testType = "TrimbleRoundsTDEF";

    bool success = setupTrimbleTDEFConverterTest(testType, testVersion, failure_message);

    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter1()
{
    QString testName = "TEST1";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter1HeightUncertainty()
{
    QString testName = "TEST1HeightUncertainty";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter1_instSigma()
{
    QString testName = "TEST1INST";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter1_unitsFeet()
{
    QString testName = "TEST1FEET";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter2()
{
    QString testName = "TEST2";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter3()
{
    QString testName = "TEST3";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter4()
{
    QString testName = "TEST4";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter5()
{
    QString testName = "TEST5";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverter6()
{
    QString testName = "TEST6";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverterTopoF1AngOnly()
{
    QString testName = "TOPOF1ANGONLY";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverterTopoF1F2AngOnly()
{
    QString testName = "TOPOF1F2ANGONLY";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverterTopoF1AngDist()
{
    QString testName = "TOPOF1ANGDIST";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverterTopoF1F2AngDist()
{
    QString testName = "TOPOF1F2ANGDIST";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverterTopoF1AngDistInst()
{
    QString testName = "TOPOF1ANGDISTINST";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::trimbleRoundsConverterTopoF1F2AngDistInst()
{
    QString testName = "TOPOF1F2ANGDISTINST";
    QString failure_message;

    bool success = setupTrimbleRoundsConverterTest(testName, failure_message);
    QVERIFY2(success, failure_message.toStdString().c_str());
}

void TestLSAGui::example01_Integration()
{
    QString testName = "example01";
    QString errMsg;
    MainWindow mainWindow;
    setupLsaIntegrationTest(mainWindow, testName);

    double roundedAPV = roundDoubleToPrecision(mainWindow.ui->valueAPV->text().toDouble(), 3);
    QCOMPARE(roundedAPV, 1.083);

    QString expected(testoutputPath + "/gui/integration/" + testName + "/" + testName + ".expected.csv");
    QString output(testoutputPath + "/gui/integration/" + testName + "/" + testName + ".csv");
    bool success = diff_files(expected, output, errMsg);
    QVERIFY2(success, errMsg.toStdString().c_str());
}

void TestLSAGui::export_UTM()
{
    QVERIFY2(setupTestDir("exports/UTM"), setupMessage.c_str());

    QString inputFile(testoutputPath + "/gui/exports/UTM/UTM.proj");
    MainWindow mainWindow;
    QEventLoop postprocessorLoop;

    //load the project and calculate adjustment
    getPostprocessorLoop(&mainWindow,&postprocessorLoop);
    mainWindow.loadLsaFile(inputFile);
    mainWindow.calculateAdjustment();
    postprocessorLoop.exec();

    //export points in UTM format with default file name
    mainWindow.exportPointsUTM();

    //diff the expected output vs the generated output
    QString expected(testoutputPath + "/gui/exports/UTM/UTM_UTM.expected.csv");
    QString output(testoutputPath + "/gui/exports/UTM/UTM_UTM.csv");
    QString errMsg;
    bool success = diff_files(expected, output, errMsg);
    QVERIFY2(success, errMsg.toStdString().c_str());
}

void TestLSAGui::export_NCAT()
{
    QVERIFY2(setupTestDir("exports/NCAT"), setupMessage.c_str());

    QString inputFile(testoutputPath + "/gui/exports/NCAT/NCAT.proj");
    MainWindow mainWindow;
    QEventLoop postprocessorLoop;

    //load the project and calculate adjustment
    getPostprocessorLoop(&mainWindow,&postprocessorLoop);
    mainWindow.loadLsaFile(inputFile);
    mainWindow.calculateAdjustment();
    postprocessorLoop.exec();

    //export points in UTM format with default file name
    mainWindow.exportPointsNCAT();

    //diff the expected output vs the generated output
    QString expected(testoutputPath + "/gui/exports/NCAT/NCAT_NCAT.expected.csv");
    QString output(testoutputPath + "/gui/exports/NCAT/NCAT_NCAT.csv");
    QString errMsg;
    bool success = diff_files(expected, output, errMsg);
    QVERIFY2(success, errMsg.toStdString().c_str());
}

void TestLSAGui::export_SPC()
{
    QVERIFY2(setupTestDir("exports/SPC"), setupMessage.c_str());

    QString inputFile(testoutputPath + "/gui/exports/SPC/levelExample.proj");
    MainWindow mainWindow;
    QEventLoop postprocessorLoop;
    QDir::setCurrent(scriptsTestPath);
    QProcess diffProcess;
    QString exe(pythonExe);
    QStringList diffArgs;

    //load the project and calculate adjustment
    getPostprocessorLoop(&mainWindow,&postprocessorLoop);
    mainWindow.loadLsaFile(inputFile);
    mainWindow.calculateAdjustment();
    postprocessorLoop.exec();

    mainWindow.exportPointsSPC(QString("2846"));

    QString statusTxt = mainWindow.ui->statusWindow->toPlainText();
    QVERIFY2(!statusTxt.contains("Warning") && !statusTxt.contains("Error"), statusTxt.toStdString().c_str());

    //First test
    QString expected(testoutputPath + "/gui/exports/SPC/levelExample_ENH_SPCS83_Texas_Central_zone_(meter).expected.csv");
    QString output(testoutputPath + "/gui/exports/SPC/levelExample_ENH_SPCS83_Texas_Central_zone_(meter).csv");
    QString errMsg;
    bool success = diff_files(expected, output, errMsg);
    QVERIFY2(success, errMsg.toStdString().c_str());

    // Fix to failing test
    expected = testoutputPath + "/gui/exports/SPC/levelExample_projection_detailed.expected.csv";
    output = testoutputPath + "/gui/exports/SPC/levelExample_projection_detailed.csv";

    diffArgs << scriptsTestPath + "/asciiDiffwTol.py"
              << expected << output;
    diffProcess.start(exe, diffArgs);
    diffProcess.waitForFinished();

    QString diffError = diffProcess.readAllStandardError();
    QString diffOutput = diffProcess.readAllStandardOutput();
    QVERIFY2(diffError.isEmpty(), diffError.toStdString().c_str());
    QVERIFY2(diffOutput == QString("0"), diffOutput.toStdString().c_str());
}

void TestLSAGui::export_SPC_south()
{
    QVERIFY2(setupTestDir("exports/SPC_south"), setupMessage.c_str());

    QString inputFile(testoutputPath + "/gui/exports/SPC_south/southSPCcheck.proj");
    MainWindow mainWindow;
    QEventLoop postprocessorLoop;

    //load the project and calculate adjustment
    getPostprocessorLoop(&mainWindow,&postprocessorLoop);
    mainWindow.loadLsaFile(inputFile);
    mainWindow.calculateAdjustment();
    postprocessorLoop.exec();

    mainWindow.exportPointsSPC(QString("32718"));

    QString expected(testoutputPath + "/gui/exports/SPC_south/southSPCcheck_ENH_UTM_zone_18S.expected.csv");
    QString output(testoutputPath + "/gui/exports/SPC_south/southSPCcheck_ENH_UTM_zone_18S.csv");
    QString errMsg;
    bool success = diff_files(expected, output, errMsg);
    QVERIFY2(success, errMsg.toStdString().c_str());

    expected = testoutputPath + "/gui/exports/SPC_south/southSPCcheck_projection_detailed.expected.csv";
    output = testoutputPath + "/gui/exports/SPC_south/southSPCcheck_projection_detailed.csv";
    success = diff_files(expected, output, errMsg);
    QVERIFY2(success, errMsg.toStdString().c_str());
}

void TestLSAGui::export_RawResidualsENU()
{
    QVERIFY2(setupTestDir("exports/RawResidualsENU"), setupMessage.c_str());

    // Run the test
    MainWindow mainWindow;
    QString inputFile(testoutputPath + "/gui/exports/RawResidualsENU/ECEFtoENU.proj");
    mainWindow.loadLsaFile(inputFile);

    mainWindow.exportMeasResidAsENU();

    // Diff the expected.lsa versus actual .csv files
    QDir::setCurrent(scriptsTestPath);
    QProcess diffProcess;
    QString exe(pythonExe);
    QStringList diffArgs;
    diffArgs << scriptsTestPath + "/lsaAsciiDiff.py"
             << testoutputPath + "/gui/exports/RawResidualsENU/ECEFtoENU.expected.enuresid.csv"
             << testoutputPath + "/gui/exports/RawResidualsENU/ECEFtoENU.enuresid.csv"
             << "-binPath" << binPath;

    diffProcess.start(exe,diffArgs);
    diffProcess.waitForFinished();

    // Return the results
    QString diffError = diffProcess.readAllStandardError();
    QString diffOutput = diffProcess.readAllStandardOutput();

    if ( !diffError.isEmpty() )
        diffOutput = diffError;
    else if ( diffOutput.isEmpty() )
        diffOutput = QString("No output returned from diff script.");
    QVERIFY2( diffOutput == QString("0"), diffOutput.toStdString().c_str() );
}


void TestLSAGui::addAutogenToProject()
{
    QVERIFY2(setupTestDir("addAutogen"), setupMessage.c_str());

    MainWindow mainWindow;
    QString fileName(testoutputPath + QString("/gui/addAutogen/example01.proj"));
    QEventLoop postprocessorLoop;

    //load the project and calculate adjustment
    getPostprocessorLoop(&mainWindow,&postprocessorLoop);
    mainWindow.loadLsaFile(fileName);
    mainWindow.calculateAdjustment();
    postprocessorLoop.exec();

    QModelIndex rootIndex = mainWindow.guiModel->getNextIndex();

    QModelIndex commentRecordIndex = rootIndex.child(2, 0);
    QModelIndex posgNonAutoIndex = rootIndex.child(1, 0);

    LSARecord* commentRec = mainWindow.guiModel->getLSARecord(commentRecordIndex);
    LSARecord* posgNonAutoRec = mainWindow.guiModel->getLSARecord(posgNonAutoIndex);
    QVERIFY2(commentRec->getRecType() == LSAType::COMMENT, std::string("comment rec is " + commentRec->getSingleLSAString()).c_str());
    QVERIFY2(posgNonAutoRec->getRecType() == LSAType::POSG, std::string("posgNonAuto rec is " + posgNonAutoRec->getSingleLSAString()).c_str());

    mainWindow.ui->treeView->selectionModel()->select(commentRecordIndex,
                                                      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QVERIFY(!mainWindow.ui->actionAdd_Autogen->isVisible());

    QPersistentModelIndex autogenIndex = mainWindow.guiModel->autogenIndex;
    QVERIFY(autogenIndex.isValid());

    QModelIndex autogenIndex1 = mainWindow.guiModel->autogenIndex.child(3, 0);
    QModelIndex autogenIndex2 = mainWindow.guiModel->autogenIndex.child(4, 0);

    mainWindow.ui->treeView->selectionModel()->select(autogenIndex1,
                                                      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    mainWindow.ui->treeView->selectionModel()->select(posgNonAutoIndex,
                                                      QItemSelectionModel::Select | QItemSelectionModel::Rows);

    QVERIFY(!mainWindow.ui->actionAdd_Autogen->isVisible());

    mainWindow.ui->treeView->selectionModel()->select(autogenIndex1,
                                                      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    mainWindow.ui->treeView->selectionModel()->select(autogenIndex2,
                                                      QItemSelectionModel::Select | QItemSelectionModel::Rows);

    QVERIFY(mainWindow.ui->actionAdd_Autogen->isVisible());

    mainWindow.ui->actionAdd_Autogen->trigger();
    mainWindow.saveProject();

    QDir::setCurrent(scriptsTestPath);
    QProcess diffProcess;
    QString exe(pythonExe);
    QStringList diffArgs;
    QString expectedFile = testoutputPath + QString("/gui/addAutogen/example01.expected.proj");
    QString testFile = fileName;
    diffArgs << scriptsTestPath + "/lsaAsciiDiff.py"
              << expectedFile << testFile;
    diffProcess.start(exe, diffArgs);
    diffProcess.waitForFinished();

    QString diffError = diffProcess.readAllStandardError();
    QString diffOutput = diffProcess.readAllStandardOutput();
    QVERIFY2(diffError.isEmpty(), diffError.toStdString().c_str());
    QVERIFY2(diffOutput == QString("0"), diffOutput.toStdString().c_str());

    // Compare expected and output initial_coordinates.lsa file
    expectedFile = testoutputPath + QString("/gui/addAutogen/initial_coordinates.expected.lsa");
    testFile = testoutputPath + QString("/gui/addAutogen/initial_coordinates.lsa");

    diffArgs.clear();
    diffArgs << scriptsTestPath + "/lsaAsciiDiff.py"
             << expectedFile << testFile << "-n" << "2";
    diffProcess.start(exe, diffArgs);
    diffProcess.waitForFinished();

    diffError = diffProcess.readAllStandardError();
    diffOutput = diffProcess.readAllStandardOutput();
    QVERIFY2(diffError.isEmpty(), diffError.toStdString().c_str());
    QVERIFY2(diffOutput == QString("0"), diffOutput.toStdString().c_str());
}

void TestLSAGui::record_Cut()
{
    QVERIFY2(setupTestDir("record/cut"), setupMessage.c_str());

    // Load the input file
    MainWindow mainWindow;
    QString fileName(testoutputPath + "/gui/record/cut/cut.proj");
    mainWindow.loadLsaFile(fileName);
    QModelIndex rootIndex = mainWindow.guiModel->getNextIndex();

    //TEST 1: verify multi-select cut-n-paste
    QModelIndex firstChildIndex = rootIndex.child(1,0);
    QModelIndex thirdChildIndex = rootIndex.child(3,0);
    QModelIndex fifthChildIndex = rootIndex.child(5,0);
    QPersistentModelIndex sixthChildIndex = QPersistentModelIndex(rootIndex.child(6,0));
    LSARecord *firstChildRecord = mainWindow.guiModel->getLSARecord(firstChildIndex);
    LSARecord *thirdChildRecord = mainWindow.guiModel->getLSARecord(thirdChildIndex);
    LSARecord *fifthChildRecord = mainWindow.guiModel->getLSARecord(fifthChildIndex);
    QString firstChildLabel = QString::fromStdString(firstChildRecord->getLabel());
    QString thirdChildLabel = QString::fromStdString(thirdChildRecord->getLabel());
    QString fifthChildLabel = QString::fromStdString(fifthChildRecord->getLabel());

    // Select 1st, 3rd, and 5th records
    QModelIndexList selectedIndexes;
    selectedIndexes << firstChildIndex << thirdChildIndex << fifthChildIndex;
    mainWindow.guiModel->select(selectedIndexes);
    //cut the record
    mainWindow.handleCutRecord();
    //select the (previously) 6th child record
    mainWindow.guiModel->select(sixthChildIndex);
    //paste the cut records below
    mainWindow.handlePasteRecord();
    //get labels for the last 3 records
    QModelIndex fourthChildIndex = rootIndex.child(4,0);
    QModelIndex newFifthChildIndex = rootIndex.child(5,0);
    QModelIndex newSixthChildIndex = rootIndex.child(6,0);
    LSARecord *fourthChildRecord = mainWindow.guiModel->getLSARecord(fourthChildIndex);
    LSARecord *newFifthChildRecord = mainWindow.guiModel->getLSARecord(newFifthChildIndex);
    LSARecord *newSixthChildRecord = mainWindow.guiModel->getLSARecord(newSixthChildIndex);
    QString fourthChildLabel = QString::fromStdString(fourthChildRecord->getLabel());
    QString newFifthChildLabel = QString::fromStdString(newFifthChildRecord->getLabel());
    QString newSixthChildLabel = QString::fromStdString(newSixthChildRecord->getLabel());
    //verify that the same records have been cut-n-pasted
    QVERIFY2(firstChildLabel == fourthChildLabel, "record labels do not match");
    QVERIFY2(thirdChildLabel == newFifthChildLabel, "record labels do not match");
    QVERIFY2(fifthChildLabel == newSixthChildLabel, "record labels do not match");

    //TEST 2: VERIFY ABORTED CUT-N-PASTE RECORDS ARE UNHIDDEN
    firstChildIndex = rootIndex.child(1,0);
    mainWindow.guiModel->select(firstChildIndex);
    mainWindow.handleCutRecord();
    mainWindow.undo();

    //TEST 3: VERIFY MULTI-CUT-N-PASTE ONLY PASTES THE FINAL CUT
    QModelIndex secondChildIndex = rootIndex.child(2,0);
    LSARecord *secondChildRecord = mainWindow.guiModel->getLSARecord(secondChildIndex);
    QString secondChildLabel = QString::fromStdString(secondChildRecord->getLabel());
    QPersistentModelIndex persistenThirdChildIndex = rootIndex.child(3,0);
    //first cut
    mainWindow.guiModel->select(rootIndex.child(1,0));
    mainWindow.handleCutRecord();
    //second cut
    mainWindow.guiModel->select(rootIndex.child(1,0));
    mainWindow.handleCutRecord();
    mainWindow.guiModel->select(persistenThirdChildIndex);
    mainWindow.handlePasteRecord();
    thirdChildIndex = rootIndex.child(2,0);
    LSARecord *newThirdChildRecord = mainWindow.guiModel->getLSARecord(thirdChildIndex);
    QString newThirdChildLabel = QString::fromStdString(newThirdChildRecord->getLabel());
    QVERIFY2(secondChildLabel == newThirdChildLabel, "record is still gone");
}

void TestLSAGui::disableRecordsWithTextNotes()
{
    QVERIFY2(setupTestDir("textNotes/disable"), setupMessage.c_str());

    // Load the input file
    MainWindow mainWindow;
    QString fileName(testoutputPath + "/gui/textNotes/disable/RecordsWithTextNotes.proj");
    mainWindow.loadLsaFile(fileName);

    // show comments
    mainWindow.ui->actionShow_Comments->activate(QAction::Trigger);

    // Get the root index and its number of children
    QModelIndex rootIndex = mainWindow.guiModel->index(0,0,QModelIndex());
    GuiModelItem *rootItem = mainWindow.guiModel->getItem(rootIndex);
    int numChildren = rootItem->childCount();

    // Select all records except the root record
    QModelIndex firstChild = rootIndex.child(0,0);
    QModelIndex lastChild = rootIndex.child(numChildren-1,0);
    QItemSelection itemSelection(firstChild, lastChild );
    mainWindow.ui->treeView->selectionModel()->select(itemSelection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    // Disable all of the selected records
    mainWindow.ui->actionDisable->activate(QAction::Trigger);

    // Save the project
    mainWindow.saveProject();

    // Diff the output vs expected output
    QString expected = testoutputPath + "/gui/textNotes/disable/DisabledRecordsWithTextNotes.expected.proj";
    QString actual = testoutputPath + "/gui/textNotes/disable/RecordsWithTextNotes.proj";
    asciiDiffFiles(expected, actual);

    // Enable all of the selected records
    mainWindow.ui->actionEnable->activate(QAction::Trigger);

    // Save the project
    mainWindow.saveProject();

    // Diff the output vs expected output
    expected = testoutputPath + "/gui/textNotes/disable/EnabledRecordsWithTextNotes.expected.proj";
    actual = testoutputPath + "/gui/textNotes/disable/RecordsWithTextNotes.proj";
    asciiDiffFiles(expected, actual);
}

bool TestLSAGui::setupConverterTestDir(QString converterType, QString specTest)
{
    setupPathVars();

    QString copyFromPath = QDir::cleanPath(testGuiPath + "/converters/" + converterType + "/" + specTest);
    QString copyToPath = QDir::cleanPath(testoutputGuiPath + "/converters/" + converterType + "/" + specTest);

    // delete the testoutput directory
    QDir copyToDir(copyToPath);
    bool success = copyToDir.removeRecursively();

    QDir testoutputGuiDir(testoutputGuiPath);
    success = testoutputGuiDir.mkpath(copyToPath);

    // copy the contents of test to testoutput
    success = recursiveCopydir(copyFromPath, copyToPath);

    return success;
}

bool TestLSAGui::setupTrimbleRoundsConverterTest(QString specTest, QString &failure_message)
{
    QString converterType = "TrimbleRounds";
    QString extension = ".jxl";
    QString setupMessage = "Converter test " + converterType + " FAILED!";
    if(!setupConverterTestDir(converterType, specTest))
    {
        failure_message = setupMessage + " setupConverterTestDir failure\n";
        return false;
    }

    QString converterScript = convertingPath + "/converterlauncher.py";
    QString converterDiffScript = scriptsTestPath + "/converterDiff.py";
    QString converterTestOutputPath = QDir::cleanPath(testoutputGuiPath + "/converters/" + converterType + "/" + specTest);
    QString inputFile = converterTestOutputPath + "/" + specTest + extension;
    QString truthFile = converterTestOutputPath + "/" + specTest + ".expected.lsa";
    QString outputFile = converterTestOutputPath + "/" + specTest + ".lsa";
    QString warnFilePath = converterTestOutputPath + "/" + specTest + ".wrn";
    QString cfgFilePath = converterTestOutputPath + "/converter.cfg";

    // run the converter
    QDir::setCurrent(scriptsTestPath);
    QProcess converterProcess;
    QString exe(pythonExe);
    QStringList converterArgs;
    converterArgs << converterScript
                  << "--in" << inputFile
                  << "--out" << outputFile
                  << "--cfgfile" << cfgFilePath
                  << "--wrnfile" << warnFilePath;

    converterProcess.start(exe, converterArgs);
    converterProcess.waitForFinished();

    QString converterError = converterProcess.readAllStandardError();
    QString converterOutput = converterProcess.readAllStandardOutput();
    int converterReturnCode = converterProcess.exitCode();
    QProcess::ExitStatus converterExitStatus = converterProcess.exitStatus();

    if(((converterExitStatus == QProcess::NormalExit) && (converterReturnCode != 0)) ||
       (converterExitStatus != QProcess::NormalExit) ||
       (converterError.size() > 0))
    {
        failure_message = setupMessage + " Failure in running converter script.\n" + converterOutput + "\n" + converterError;
        return false;
    }

    // run the difference tool
    QProcess diffProcess;
    QStringList diffArgs;
    diffArgs << converterDiffScript << truthFile << outputFile;
    diffProcess.start(exe, diffArgs);
    diffProcess.waitForFinished();

    QString diffError = diffProcess.readAllStandardError();
    QString diffOutput = diffProcess.readAllStandardOutput();
    int diffReturnCode = diffProcess.exitCode();
    QProcess::ExitStatus diffExitStatus = diffProcess.exitStatus();

    if(((diffExitStatus == QProcess::NormalExit) && (diffReturnCode != 0)) ||
       (diffExitStatus != QProcess::NormalExit) ||
       (diffError.size() > 0))
    {
        failure_message = setupMessage + " Failure in diff of expected vs generated parent.\n" + diffOutput + "\n" + diffError;
        return false;
    }

    //difference each of the child includes of the main output file
    QFile infile(outputFile);
    QString line;
    QList<QString> childIncludes;
    QString match_str="--include";
    if(infile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&infile);
        while(!in.atEnd())
        {
            line = in.readLine();
            if(line.startsWith("--include"))
            {
                QString include = QString::fromStdString(line.toStdString().substr(10));
                childIncludes.append(include);
            }
        }
        infile.close();
    }

    foreach(QString include, childIncludes)
    {
        QString include_name = include.remove(QChar('"'));
        include_name = include_name.remove(QChar('\n'));
        std::string include_name_str = include_name.toStdString();
        unsigned long begin = include_name_str.find('_')+1;
        unsigned long end = include_name_str.rfind('.');
        QString station = QString::fromStdString(include_name_str.substr(begin,end-begin));
        std::string truth_file_str = truthFile.toStdString();
        end = truth_file_str.find(".expected");
        QString expected_file = QString::fromStdString(truth_file_str.substr(0,end)) + "_" + station + ".expected.lsa";
        if(expected_file.contains(QChar(' ')))
        {
            expected_file = "\"" + expected_file + "\"";
        }
        QFileInfo output_file(outputFile);
        QString test_file = output_file.absolutePath() + "/" + include_name;
        if(test_file.contains(QChar(' ')))
        {
            test_file = "\"" + test_file + "\"";
        }
        diffArgs.clear();
        diffArgs << converterDiffScript << expected_file << test_file;
        diffProcess.start(exe, diffArgs);
        diffProcess.waitForFinished();

        diffError = diffProcess.readAllStandardError();
        diffOutput = diffProcess.readAllStandardOutput();
        diffReturnCode = diffProcess.exitCode();
        diffExitStatus = diffProcess.exitStatus();

        if(((diffExitStatus == QProcess::NormalExit) && (diffReturnCode != 0)) ||
           (diffExitStatus != QProcess::NormalExit) ||
           (diffError.size() > 0))
        {
            failure_message = setupMessage + " Failure in diff of expected vs generated child include.\n"
                    + diffOutput + "\n" + diffError;
            return false;
        }
    }

    return true;
}

void TestLSAGui::converterWarningFile()
{
    QVERIFY2(setupTestDir("converters/warningFiles"), "FAILURE to setup directory for converterWarningFile test!");

    // Load the input file
    MainWindow mainWindow;
    QString projectDir(testoutputPath + "/gui/converters/warningFiles/");
    QString fileName(projectDir + "warningFiles.proj");
    mainWindow.loadLsaFile(fileName);
    QModelIndex rootIndex = mainWindow.guiModel->getNextIndex();
    QModelIndex firstChildIndex = rootIndex.child(1,0);
    mainWindow.guiModel->select(firstChildIndex);

    //Import SetsOfAngles file
    QStringList fileList;
    QString errMsg;
    QString statusWindowContents;
    QModelIndex currentIndex = mainWindow.guiModel->getSingleSelectedRecord();
    fileList << QString(projectDir + "SetsOfAngles.log");
    mainWindow.convertFilesAndAddToProject(fileList, lsa::CONVERTER_TYPE_SETSOFANGLES, currentIndex.parent(), currentIndex.row() + 1);
    //verify status window points to .wrn file
    statusWindowContents = mainWindow.ui->statusWindow->toPlainText();
    QVERIFY2(statusWindowContents.contains("Warning - warnings generated when parsing SetsOfAngles.log. Click"),
                                           statusWindowContents.toStdString().c_str());
    //verify expected .wrn file contents
    QVERIFY2(diff_files(QString(projectDir + "SetsOfAngles.expected.wrn"), QString(projectDir + "SetsOfAngles.log.wrn"), errMsg), errMsg.toStdString().c_str());

    //Import LeicaGSI file
    //SKIP.  Currently no warnings issued from this converter.

    //Import OPUS file
    fileList.clear();
    currentIndex = mainWindow.guiModel->getSingleSelectedRecord();
    fileList << QString(projectDir + "opus.opus");
    mainWindow.convertFilesAndAddToProject(fileList, lsa::CONVERTER_TYPE_OPUS, currentIndex.parent(), currentIndex.row() + 1);
    //verify status window points to .wrn file
    statusWindowContents = mainWindow.ui->statusWindow->toPlainText();
    QVERIFY2(statusWindowContents.contains("Warning - warnings generated when parsing opus.opus. Click"),
                                           statusWindowContents.toStdString().c_str());
    //verify expected .wrn file contents
    QVERIFY2(diff_files(QString(projectDir + "opus.expected.wrn"), QString(projectDir + "opus.opus.wrn"), errMsg), errMsg.toStdString().c_str());

    //Import PPP merged file
    fileList.clear();
    currentIndex = mainWindow.guiModel->getSingleSelectedRecord();
    fileList << QString(projectDir + "Merge_job_683_1.txt");
    mainWindow.convertFilesAndAddToProject(fileList, lsa::CONVERTER_TYPE_PPP, currentIndex.parent(), currentIndex.row() + 1);
    //verify status window points to .wrn file
    statusWindowContents = mainWindow.ui->statusWindow->toPlainText();
    QVERIFY2(statusWindowContents.contains("Warning - warnings generated when parsing Merge_job_683_1.txt. Click"),
                                           statusWindowContents.toStdString().c_str());
    //verify expected .wrn file contents
    QVERIFY2(diff_files(QString(projectDir + "Merge_job_683_1.expected.wrn"), QString(projectDir + "Merge_job_683_1.txt.wrn"), errMsg), errMsg.toStdString().c_str());

    //Import TrimbleTDEF file
    fileList.clear();
    currentIndex = mainWindow.guiModel->getSingleSelectedRecord();
    fileList << QString(projectDir + "TrimbleTDEF.asc");
    mainWindow.convertFilesAndAddToProject(fileList, lsa::CONVERTER_TYPE_TRIMBLETDEF, currentIndex.parent(), currentIndex.row() + 1);
    //verify status window points to .wrn file
    statusWindowContents = mainWindow.ui->statusWindow->toPlainText();
    QVERIFY2(statusWindowContents.contains("Warning - warnings generated when parsing TrimbleTDEF.asc. Click"),
                                           statusWindowContents.toStdString().c_str());
    //verify expected .wrn file contents
    QVERIFY2(diff_files(QString(projectDir + "TrimbleTDEF.expected.wrn"), QString(projectDir + "TrimbleTDEF.asc.wrn"), errMsg), errMsg.toStdString().c_str());

    //TODO: Uncomment this section once the TrimbleRounds .jxl file has been restored as a supported instrument file
/*
    //Import TrimbleRounds file
    fileList.clear();
    currentIndex = mainWindow.guiModel->getSingleSelectedRecord();
    fileList << QString(projectDir + "TrimbleRounds.jxl");
    mainWindow.convertFilesAndAddToProject(fileList, lsa::CONVERTER_TYPE_TRIMBLEROUNDS, currentIndex.parent(), currentIndex.row() + 1);
    //verify status window points to .wrn file
    statusWindowContents = mainWindow.ui->statusWindow->toPlainText();
    QVERIFY2(statusWindowContents.contains("Warning - warnings generated when parsing TrimbleRounds.jxl. Click"),
                                           statusWindowContents.toStdString().c_str());
    //verify expected .wrn file contents
    QVERIFY2(diff_files(QString(projectDir + "TrimbleRounds.expected.wrn"), QString(projectDir + "TrimbleRounds.wrn"), errMsg), errMsg.toStdString().c_str());
*/
}

QTEST_MAIN(TestLSAGui)

#include "testpubliclsagui.moc"
