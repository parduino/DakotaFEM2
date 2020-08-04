/* *****************************************************************************
Copyright (c) 2016-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS 
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, 
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */

// Written: fmckenna

#include "InputWidgetFEM.h"
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QDebug>
#include <QFileDialog>
#include <QPushButton>
#include <sectiontitle.h>
#include <QFileInfo>
#include <QVectorIterator>
#include <OpenSeesPyParser.h>

#include <OpenSeesParser.h>
#include <FEAPpvParser.h>

#include <InputWidgetParameters.h>

InputWidgetFEM::InputWidgetFEM(InputWidgetParameters *param, QWidget *parent)
: SimCenterWidget(parent), theParameters(param), numInputs(1)
{
    femSpecific = 0;

    layout = new QVBoxLayout();

    QVBoxLayout *name= new QVBoxLayout;

    // text and add button at top
    QHBoxLayout *titleLayout = new QHBoxLayout();

    SectionTitle *textFEM=new SectionTitle();
    textFEM->setText(tr("Finite Element Method Application"));
    //textFEM->setMinimumWidth(250);
    QSpacerItem *spacer = new QSpacerItem(50,10);

    femSelection = new QComboBox();
   // femSelection->setMaximumWidth(400);
    //femSelection->setMinimumWidth(200);
    femSelection->setToolTip(tr("Remote Application to Run"));
    titleLayout->addWidget(textFEM);
    titleLayout->addItem(spacer);
    titleLayout->addWidget(femSelection);
    titleLayout->addStretch();
    titleLayout->setSpacing(0);
    titleLayout->setMargin(0);

    layout->addLayout(titleLayout);

    layout->setSpacing(10);
    layout->setMargin(0);
    // name->addStretch();

    femSelection->addItem(tr("OpenSees"));
    femSelection->addItem(tr("FEAPpv"));
    femSelection->addItem(tr("OpenSeesPy"));

    connect(femSelection, SIGNAL(currentIndexChanged(QString)), this, SLOT(femProgramChanged(QString)));

    layout->addLayout(name);
    this->femProgramChanged(tr("OpenSees"));

    // layout->addStretch();
    // layout->setSpacing(10);
    layout->setMargin(0);
    layout->addStretch();

    this->setLayout(layout);

}

InputWidgetFEM::~InputWidgetFEM()
{

}


void InputWidgetFEM::clear(void)
{

}



bool
InputWidgetFEM::outputToJSON(QJsonObject &jsonObject)
{
    QJsonObject fem;
    QJsonArray apps;
    fem["program"]=femSelection->currentText();
    fem["numInputs"]=numInputs;
    if (numInputs == 1) {
        QString fileName1=inputFilenames.at(0)->text();
        QString fileName2=postprocessFilenames.at(0)->text();

        fem["inputFile"]=fileName1;
        fem["postprocessScript"]=fileName2;

        QFileInfo fileInfo(fileName1);

        fem["mainInput"]=fileInfo.fileName();
        QString path = fileInfo.absolutePath();
        fem["dir"]=path;

        QFileInfo fileInfo2(fileName2);
        fem["mainPostprocessScript"]=fileInfo2.fileName();

        if (femSelection->currentText() == "OpenSeesPy") {

             QString fileName3=parametersFilenames.at(0)->text();
             QFileInfo pFileInfo(fileName3);
             fem["parametersFile"]=pFileInfo.fileName();
             fem["parametersScript"]=fileName3;
        }

    } else {
        for (int i=0; i<numInputs; i++) {
            QJsonObject obj;
            QString fileName1=inputFilenames.at(i)->text();
            QString fileName2=postprocessFilenames.at(i)->text();

            obj["inputFile"]=fileName1;
            obj["postprocessScript"]=fileName2;

            QFileInfo fileInfo(fileName1);

            obj["mainInput"]=fileInfo.fileName();
            QString path = fileInfo.absolutePath();
            obj["dir"]=path;

            QFileInfo fileInfo2(fileName2);
            obj["mainPostprocessScript"]=fileInfo2.fileName();
            apps.append(obj);


        }
        fem["fileInfo"] = apps;
    }

    jsonObject["fem"]=fem;

    return true;
}


bool
InputWidgetFEM::inputFromJSON(QJsonObject &jsonObject)
{
    this->clear();

    if (jsonObject.contains("fem")) {

        QJsonObject femObject = jsonObject["fem"].toObject();

        QString program=femObject["program"].toString();

        int index = femSelection->findText(program);
        femSelection->setCurrentIndex(index);
        this->femProgramChanged(program);

        numInputs = 1;
        if (femObject.contains("numInputs") )
            numInputs = femObject["numInputs"].toInt();

        if (numInputs == 1) {

            QString fileName1=femObject["inputFile"].toString();
            QString fileName2=femObject["postprocessScript"].toString();


            inputFilenames.at(0)->setText(fileName1);
            postprocessFilenames.at(0)->setText(fileName2);

            if (femSelection->currentText() == "OpenSeesPy") {
                QString fileName3=femObject["parametersScript"].toString();
                parametersFilenames.at(0)->setText(fileName3);
            }

            this->parseInputfilesForRV(fileName1);
        }
        // call setFilename1 so parser works on input file

    } else {
        emit sendErrorMessage("ERROR: FEM Input - no fem section in input file");
        return false;
    }
    return true;
}

void InputWidgetFEM::femProgramChanged(const QString &arg1)
{
    //
    // remove old widget and clear file names
    //

    if (femSpecific != 0) {
        // layout->rem
        layout->removeWidget(femSpecific);
        delete femSpecific;
        femSpecific = 0;
    }

    inputFilenames.clear();
    postprocessFilenames.clear();
    parametersFilenames.clear();

    femSpecific = new QWidget();   

    if (numInputs == 1) {

        QGridLayout *femLayout = new QGridLayout();

        QLabel *label1 = new QLabel();
        QLineEdit *file1 = new QLineEdit;
        QPushButton *chooseFile1 = new QPushButton();
        chooseFile1->setText(tr("Choose"));
         if ((arg1 == QString("FEAPpv")) || (arg1 == QString("OpenSees"))){
            connect(chooseFile1, &QPushButton::clicked, this, [=](){
                file1->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"C://", "All files (*.*)"));
                this->parseInputfilesForRV(file1->text());
            });
         } else {
             connect(chooseFile1, &QPushButton::clicked, this, [=](){
                 file1->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"C://", "All files (*.*)"));
                 // this->parseInputfilesForRV(file1->text());
             });
         }

        chooseFile1->setToolTip(tr("Push to choose a file from your file system"));

        femLayout->addWidget(label1, 0,0);
        femLayout->addWidget(file1,  0,1);
        femLayout->addWidget(chooseFile1, 0, 2);

        QLabel *label2 = new QLabel();
        label2->setText("Postprocess Script");
        QLineEdit *file2 = new QLineEdit;
        QPushButton *chooseFile2 = new QPushButton();

        connect(chooseFile2, &QPushButton::clicked, this, [=](){
            file2->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"C://", "All files (*.*)"));
        });
         chooseFile2->setText(tr("Choose"));
        chooseFile2->setToolTip(tr("Push to choose a file from your file system"));

        if (arg1 == QString("FEAPpv")){
            label1->setText("Input File");
            file1->setToolTip(tr("Name of FEAPpv input file"));
            file2->setToolTip(tr("Name of Python script that will process FEAPpv output file for UQ engine"));
        } else if (arg1 == "OpenSees") {
            label1->setText("Input Script");
            file1->setToolTip(tr("Name of OpenSees input script"));
            file2->setToolTip(tr("Name of Python/Tcl script that will process OpenSees output file for UQ engine"));
        } else {
            label1->setText("Input Script");
            file1->setToolTip(tr("Name of OpenSeesPy input script"));
            file2->setToolTip(tr("Name of Python script that will process OpenSeesPy output"));
        }

        femLayout->addWidget(label2, 1,0);
        femLayout->addWidget(file2, 1,1);
        femLayout->addWidget(chooseFile2, 1,2);


        if (arg1 == "OpenSeesPy") {
            QLabel *label3 = new QLabel();
            label3->setText("Parameters File");
            QLineEdit *file3 = new QLineEdit;
            file2->setToolTip(tr("Name of Python script that contains defined parameters"));
            QPushButton *chooseFile3 = new QPushButton();

            connect(chooseFile3, &QPushButton::clicked, this, [=](){
                file3->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"C://", "All files (*.*)"));
                this->parseInputfilesForRV(file3->text());
            });
            chooseFile3->setText(tr("Choose"));
            chooseFile3->setToolTip(tr("Push to choose a file from your file system"));

            parametersFilenames.append(file3);

            femLayout->addWidget(label3, 2,0);
            femLayout->addWidget(file3, 2,1);
            femLayout->addWidget(chooseFile3, 2,2);

            femLayout->setColumnStretch(3,1);
            femLayout->setColumnStretch(1,3);

        } else {
            femLayout->setColumnStretch(3,1);
            femLayout->setColumnStretch(1,3);
        }


        inputFilenames.append(file1);
        postprocessFilenames.append(file2);
        femSpecific->setLayout(femLayout);

    } else {

        QVBoxLayout *multiLayout = new QVBoxLayout();
        femSpecific->setLayout(multiLayout);

        for (int i=0; i< numInputs; i++) {
            QGridLayout *femLayout = new QGridLayout();
            QLabel *label1 = new QLabel();

            QLabel *label2 = new QLabel();
            label2->setText("Postprocess Script");

            QLineEdit *file1 = new QLineEdit;
            QPushButton *chooseFile1 = new QPushButton();
            chooseFile1->setText(tr("Choose"));
            connect(chooseFile1, &QPushButton::clicked, this, [=](){
                file1->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"C://", "All files (*.*)"));
                this->parseInputfilesForRV(file1->text());
            });

            if (arg1 == QString("FEAPpv")){
                label1->setText("Input File");
                file1->setToolTip(tr("Name of FEAPpv input file"));
            } else {
                label1->setText("Input Script");
                file1->setToolTip(tr("Name of OpenSees input script"));
            }

            chooseFile1->setToolTip(tr("Push to choose a file from your file system"));

            QLineEdit *file2 = new QLineEdit;
            QPushButton *chooseFile2 = new QPushButton();

            connect(chooseFile2, &QPushButton::clicked, this, [=](){
                file2->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"C://", "All files (*.*)"));
            });

            file2->setToolTip(tr("Name of Python script that will process FEAPpv output file for UQ engine"));
            chooseFile2->setToolTip(tr("Push to choose a file from your file system"));

            chooseFile2->setText(tr("Choose"));

            femLayout->addWidget(label1, 0,0);
            femLayout->addWidget(file1,  0,1);
            femLayout->addWidget(chooseFile1, 0, 2);
            femLayout->addWidget(label2, 1,0);
            femLayout->addWidget(file2, 1,1);
            femLayout->addWidget(chooseFile2, 1,2);

            femLayout->setColumnStretch(3,1);
            femLayout->setColumnStretch(1,3);

            inputFilenames.append(file1);
            postprocessFilenames.append(file2);
            multiLayout->addLayout(femLayout);

        }
        femSpecific->setLayout(multiLayout);
    }

    layout->insertWidget(1, femSpecific);

}

int InputWidgetFEM::parseInputfilesForRV(QString name1){
    QString fileName1 = name1;
    //  file1->setText(name1);

    // if OpenSees or FEAP parse the file for the variables
    QString pName = femSelection->currentText();
    if (pName == "OpenSees" || pName == "OpenSees-2") {
        OpenSeesParser theParser;
        varNamesAndValues = theParser.getVariables(fileName1);
    }  else if (pName == "FEAPpv") {
        FEAPpvParser theParser;
        varNamesAndValues = theParser.getVariables(fileName1);
    } else if (pName == "OpenSeesPy") {
        OpenSeesPyParser theParser;
        varNamesAndValues = theParser.getVariables(fileName1);
    }
    // qDebug() << "VARNAMESANDVALUES: " << varNamesAndValues;
    theParameters->setInitialVarNamesAndValues(varNamesAndValues);

    return 0;
}

void
InputWidgetFEM::numModelsChanged(int newNum) {
    numInputs = newNum;
    this->femProgramChanged(femSelection->currentText());
}

/*
void InputWidgetFEM::chooseFileName1(void)
{
    QString fileName1=QFileDialog::getOpenFileName(this,tr("Open File"),"C://", "All files (*.*)");
    int ok = this->parseInputfilesForRV(fileName1);
}
*/
void
InputWidgetFEM::specialCopyMainInput(QString fileName, QStringList varNames) {
    // if OpenSees or FEAP parse the file for the variables
    if (varNames.size() > 0) {
        QString pName = femSelection->currentText();
        if (pName == "OpenSees" || pName == "OpenSees-2") {
            OpenSeesParser theParser;
            QVectorIterator<QLineEdit *> i(inputFilenames);
            while (i.hasNext())
                theParser.writeFile(i.next()->text(), fileName,varNames);
        }  else if (pName == "FEAPpv") {
            FEAPpvParser theParser;
            QVectorIterator<QLineEdit *> i(inputFilenames);
            while (i.hasNext())
                theParser.writeFile(i.next()->text(), fileName,varNames);

        }  else if (pName == "OpenSeesPy") {
            // do things a little different!
            QFileInfo file(fileName);
            QString fileDir = file.absolutePath();
            //qDebug() << "FILENAME: " << fileName << " path: " << fileDir;
            //qDebug() << "LENGTHS:" << inputFilenames.count() << parametersFilenames.count() << parametersFilenames.length();
            OpenSeesPyParser theParser;
            bool hasParams = false;
            QVectorIterator<QLineEdit *> i(parametersFilenames);
            QString newName = fileDir + QDir::separator() + "tmpSimCenter.params";
            while (i.hasNext()) {
                QString fileName = i.next()->text();
                if (fileName.length() != 0) {
                    theParser.writeFile(fileName, newName,varNames);
                    hasParams = true;
                }
            }

            if (hasParams == false) {
                QString newName = fileDir + QDir::separator() + "tmpSimCenter.script";
                QVectorIterator<QLineEdit *> i(inputFilenames);
                while (i.hasNext()) {
                    QFile::copy(i.next()->text(), newName);
                }
            }
        }
    }
}



QString InputWidgetFEM::getApplicationName() {
    return femSelection->currentText();
}


QString InputWidgetFEM::getMainInput() {
    return inputFilenames.at(0)->text();
}

