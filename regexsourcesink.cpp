#include "regexsourcesink.h"
#include <opencv2/opencv.hpp>
#include <QVector>

bool RegexSourceSink::Init()
{
    return true;
}

bool RegexSourceSink::StartAcquisition(QString dev)
{
    QRegExp rx("(.+)/(.+)(\\..+)"); //somepath/somename.someextension
    int pos=0;

    pos=rx.indexIn(dev);
    dir=rx.cap(1);
    basename=rx.cap(2);
    extension=rx.cap(3);

    QRegExp rt("(\\d+$)");  // now look for digits in the basename
    pos=rt.indexIn(basename);
    basename.truncate(pos);

//    qDebug()<<dir<<basename<<rt.cap(1)<<extension;

    QString regexString=basename+"(\\d+)"+extension;

//    qDebug()<<"Will load files that match"<<regexString;
    QRegExp movieRegex(regexString);
    QDir basedir(dir);
    if (!basedir.exists()) qDebug()<<"Extracted dir does not exist"<<dir;

    QStringList filt;
    filt<<"*"+extension;
    basedir.setNameFilters(filt);
    QStringList files=basedir.entryList();

    QMap<int, QString> map;


    for (int i=0;i<files.count();i++) {
        if (movieRegex.indexIn(files.at(i))!=-1) {
            int newnr=movieRegex.cap(1).toInt();
            map.insert(newnr,files.at(i));
        }
    }
    if (map.count()==0) {
        qDebug()<<"No matching files found";
        return false;
    }

    // now sort along captured number
    goodFiles= new QStringList(map.values());

    index=0;
    nFrames=goodFiles->count();

    return true;
}

bool RegexSourceSink::StopAcquisition()
{
    return true;
}

bool RegexSourceSink::ReleaseCamera()
{
    delete goodFiles;
    return true;
}

bool RegexSourceSink::GrabFrame(ImagePacket &target, int indexIncrement)
{
    if (index+indexIncrement<goodFiles->count() && index+indexIncrement>0) {
        index+=indexIncrement;
        QFile dum(dir+"/"+goodFiles->at(index));
        if (dum.exists()) {
#ifdef Q_OS_WIN32
            target.image=cv::imread(dum.fileName().toStdString().c_str());
#else
            target.image=cv::imread(dum.fileName().toUtf8().data());
#endif
            target.seqNumber=index;
            return true;
        } else {
            qDebug()<<"File did not exist"<<dum.fileName();
        }
    } else {
        return true; // it is not a real error after all
    }
    return false;
}

bool RegexSourceSink::RecordFrame(ImagePacket &source)
{
    if (source.pixFormat=="BAYERRG8") {
        cv::Mat dummy(source.image.rows,source.image.cols,CV_8UC3);
        cv::cvtColor(source.image,dummy,CV_BayerRG2RGB);
        source.image=dummy;
    } else if (source.pixFormat=="BAYERGB8") {
        cv::Mat dummy(source.image.rows,source.image.cols,CV_8UC3);
        cv::cvtColor(source.image,dummy,CV_BayerGB2RGB);
        source.image=dummy;
    } else if (source.pixFormat=="BAYERRG12") {
        cv::Mat dummy(source.image.rows,source.image.cols,CV_16UC3);
        cv::cvtColor(source.image,dummy,CV_BayerRG2RGB);
        source.image=dummy;
    }


    QString filenam=QString(dir+"/"+basename+"%1"+extension).arg(index,8,10,QLatin1Char('0'));
#ifdef Q_OS_WIN32
    if (cv::imwrite(filenam.toStdString().c_str(),source.image)) {
#else
    if (cv::imwrite(filenam.toUtf8().data(),source.image)) {
#endif
        index++;
        if (startTime==-1) {
            startTime=source.timeStamp;
        }

        timestamps.push_back((source.timeStamp-startTime));
        return true;
    }
    return false;
}

bool RegexSourceSink::StartRecording(QString recFold, QString codec, int , int, int)
{
    QDir basedir(recFold);
    if (basedir.exists()) {
        dir=recFold;
        basename="image-";
    } else {
        QRegExp rx("(.+)/(.+)$"); //somepath/somename.someextension
        int pos=0;
        pos=rx.indexIn(recFold);
        if (pos>-1) {
            dir=rx.cap(1);
            basename=rx.cap(2);
        } else {
            qDebug()<<"Recording Folder does not exist";
            dir=recFold;
            basename="image-";
        }
//        qDebug()<<"recFold"<<dir<<" basename "<<basename;
    }


    extension=".png";
    if (codec=="BMP") {
        extension=".bmp";
    } else if (codec=="JPG") {
        extension=".jpg";
    }

    index=0;
    timestamps.clear();
    startTime=-1;
    return true;
}

bool RegexSourceSink::StopRecording()
{
    QString filename=QString(dir+"/"+basename+"timestamps.txt");
//    QString filename = "/home/sam/times.txt";
    QFile fileout(filename);
    if (fileout.open(QFile::ReadWrite | QFile::Text| QFile::Truncate)){
     QTextStream out(&fileout);
     for (QVector<double>::iterator iter = timestamps.begin(); iter != timestamps.end(); iter++){
         out << *iter<<"\n";
     }
     fileout.close();
    }



    return true;
}

bool RegexSourceSink::IsOpened()
{
    return true;
}

bool RegexSourceSink::SkipFrames(bool forward)
{
    int skipping = 0;
    if (forward) {
        skipping=nFrames/10;
    } else {
        skipping=-nFrames/50;
    }
//    qDebug()<<"Will try to skip "<<skipping<<" frames";

    if (index+skipping<nFrames && index+skipping>0) {
        index+=skipping;
        return true;
    } else {
        return true; // it is not a real error after all
    }
}



