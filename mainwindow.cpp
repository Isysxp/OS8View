#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>

MainWindow::MainWindow(QWidget *parent, int argc, char* argv[]) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QString tm;
    int ndx;
    QRegExp qr;

    ui->setupUi(this);
    QString PWD;
    PWD = QProcessEnvironment::systemEnvironment().value("HOME");
//    QDir::setCurrent(PWD);
    current_index = 0;
    unix2dos_command= "unix2dos";
    PtOfs = 0;
    if (argc>1) {
        DskImg.assign(argv[1]);
        if (argc>3) {
            tm=QString::fromLatin1(argv[2]);
            if (tm=="DSK:")
                ui->Rg1->setChecked(true);
            else
                ShowDir(0,false);
            tm=QString::fromLatin1(argv[3]);
            if (tm=="--dir")
                ShowDir(0,true);
            if (tm=="--list" && argc>4) {
                qr=QRegExp(argv[4]);
                ndx=fnames.indexOf(qr);
                if (ndx>0)
                    ShowFile(ndx,true);
            }
            if (tm=="--copyb" && argc>4)
                WriteFile(argv[4],0);
            if (tm=="--copy" && argc>4)
                WriteFile(argv[4],1);

            exit(0);
        }
        ShowDir(0,false);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Utility

std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> v;
    unsigned int i = 0;
    unsigned int j = s.find(c);
    while (j < s.length()) {
        v.push_back(s.substr(i, j - i));
        i = ++j;
        j = s.find(c, j);
        if (j >= s.length()) {
            v.push_back(s.substr(i, s.length()));
            break;
        }
    }
    return v;
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

void MainWindow::ShowDir(unsigned long long *del,bool pflg)   // Sync current partition and 'delete' file with matching name (XXX.YY -> XXX.DE)
{
    int blk = 0;
    int cblk;
    int ecn = 0;
    int cntr = 0;
    int adw = 0;
    int i = 0;
    int j = 0;
    int pt = 0;
    char tm = 0;
    int fblk = 0;
    int rln = 0;
    std::string fnm = "";
    short bfr[1024];
    char xbf[1000];
    unsigned long long *l;
    int tmblk;

    QFile inf(QString::fromStdString(DskImg));
    inf.open(QIODevice::ReadWrite);
    ui->Flist->clear();
    ui->View->clear();
    fnames.clear();

    PtOfs = 0;
    if (ui->Rg1->isChecked())
        PtOfs = 3248;
    blk = PtOfs + 1;
    Fspace = cntr = 0;
    do
    {
        inf.seek(blk*512);
        inf.read((char *)bfr,512);
        tmblk=blk;
        cblk=blk;
        cntr = cntr + 1;
        ecn = 4096 - bfr[0];
        Adinfo = adw = 4096 - bfr[4];
        fblk = bfr[1];
        blk = bfr[2] + PtOfs;
        pt = 5;
        for (i = 1; i <= ecn; i++)
        {
            if (bfr[pt] == 0)
            {
                rln = 4096 - bfr[pt + 1];
                sprintf(xbf,"<Empty>   %04d %04d",rln,fblk);
                fnames.append(QString::fromStdString(ReplaceAll(xbf," ","")));
                ui->Flist->addItem(xbf);
                fbk[ui->Flist->count()-1] = 0;
                fsz[ui->Flist->count()-1] = rln;
                Fspace=fblk+PtOfs;                  // Block number of free area
                Dofs=pt;                            // Offset into the directory block
                Dblk=cblk;                          // Offset of the direcory block containing this entry
                fblk = fblk + rln;
                pt = pt + 2;
            }
            else
            {
                fnm = "";
                l=(unsigned long long *)&bfr[pt];   // 8 byte filename. Use long long for compare.
                if (del)
                    if (*l==*del) {
                        bfr[pt+3]=00405;            // 'Delete' file ... change extension to .DE
                        inf.seek(tmblk*512);
                        inf.write((char *)bfr,512);
                    }
                for (j = 1; j <= 4; j++)
                {
                    tm = ((bfr[pt] & 4032) >> 6);
                    if (tm < 32)
                        tm = tm + 64;
                    if (tm == 64)
                        tm = ' ';
                    fnm = fnm.append(1,tm);
                    tm = (bfr[pt] & 63);
                    if (tm < 32)
                        tm = tm + 64;
                    if (tm == 64)
                        tm = ' ';
                    fnm = fnm.append(1,tm);
                    if (j == 3)
                        fnm = fnm.append(".");
                    pt = pt + 1;
                }
                pt = pt + adw;
                rln = (4096 - bfr[pt]) & 4095;
                sprintf(xbf,"%s %04d %04d",fnm.data(),rln,fblk);
                if (pflg)
                    printf("%s\n",xbf);
                ui->Flist->addItem(xbf);
                fnames.append(QString::fromStdString(ReplaceAll(fnm," ","")));
                fname[ui->Flist->count()-1]=ReplaceAll(fnm," ","");
                fbk[ui->Flist->count()-1] = fblk;
                fsz[ui->Flist->count()-1] = rln;
                fblk = fblk + rln;
                pt = pt + 1;
            }
        }
    } while (!(blk == PtOfs || cntr == 6));
    inf.close();
}


void MainWindow::ShowFile(int StBlk,bool pflg)
{
    int blk, i, j;
    std::string xln = "";

    ui->View->clear();
    ui->View->setReadOnly(false);

    if (fbk[StBlk] == 0)
    {
        QMessageBox::information(
                    this,
                    tr("OS/8 Filesystem Viewer"),
                    tr("Cannot read empty file.") );
        return;
    }

    QFile inf(QString::fromStdString(DskImg));
    inf.open(QIODevice::ReadOnly);
    blk = fbk[StBlk] + PtOfs;
    xln = "";
    while (1)
    {
        inf.seek(blk*512);
        inf.read((char *)bfr,512);
        blk = blk + 1;
        for (i = 0; i <= 255; i += 2)
        {
            Unpack(bfr[i], bfr[i + 1]);
            for (j = 0; j <= 2; j++)
            {
                switch (xtx[j] & 127)
                {
                case 26:
                    ui->View->appendPlainText(QString::fromStdString(xln));
                    inf.close();
                    ui-> View -> moveCursor (QTextCursor::Start) ;
                    return;
                    break;
                case 10:
                    break;
                case 13:
                    ui->View->appendPlainText(QString::fromStdString(xln));
                    if (pflg)
                        printf("%s\n",xln.c_str());
                    xln = "";
                    break;
                default:
                    xln=xln.append(1,xtx[j]&127);
                    break;
                }
            }
        }
    }
}


void MainWindow::ShowBinary(int StBlk)
{
    int blk, i, j;
    std::string xln = "";
    int ofs,lcnt;
    char bf[100];

    ui->View->clear();
    ui->View->setReadOnly(true);

    if (fbk[StBlk] == 0)
    {
        QMessageBox::information(
                    this,
                    tr("OS/8 Filesystem Viewer"),
                    tr("Cannot read empty file.") );
        return;
    }

    QFile inf(QString::fromStdString(DskImg));
    inf.open(QIODevice::ReadOnly);
    blk = fbk[StBlk] + PtOfs;
    xln = "00000: ";
    ofs=lcnt=0;
    while (1)
    {
        inf.seek(blk*512);
        inf.read((char *)bfr,512);
        blk = blk + 1;
        if (blk>fbk[StBlk] + PtOfs + fsz[StBlk]) {
            ui->View->appendPlainText(QString::fromStdString(xln));
            ui-> View -> moveCursor (QTextCursor::Start) ;
            return;
        }
        for (i = 0; i <= 255; i += 2)
        {
            Unpack(bfr[i], bfr[i + 1]);
            for (j = 0; j <= 2; j++,lcnt++,ofs++)
            {
                if (lcnt==16) {
                    lcnt=0;
                    ui->View->appendPlainText(QString::fromStdString(xln));
                    sprintf(bf,"%05o: ",ofs);
                    xln=bf;
                }
                sprintf(bf,"%03o  ",xtx[j]);
                xln.append(bf);
             }
        }
    }
}

void MainWindow::Unpack(int i1, int i2)
{

    xtx[0] = i1 & 255;
    xtx[1] = i2 & 255;
    xtx[2] = ((i2 & 07400) >> 8) | ((i1 & 07400) >> 4);

}

void MainWindow::Pack(short *i1, short *i2)
{

    *i1=(xtx[0] & 255) | ((xtx[2] << 4) & 07400);
    *i2=(xtx[1] & 255) | ((xtx[2] << 8) & 07400);

}
void MainWindow::on_Flist_clicked(const QModelIndex &index)
{
    current_index = index.row();               // Save the current index for later.
    if (ui->CpModeA->isChecked())
        ShowFile(index.row(),false);
    if (ui->CpModeB->isChecked())
        ShowBinary(index.row());
}


void MainWindow::WriteFile(std::string fname, int mode)
{
    std::vector<std::string> sbstr(2);
    std::string tm,fnm;
    std::string tmfile=fname;
    unsigned short Fbfr[10],*p;
    long flsz;
    unsigned int vl,i,rdcnt;
    int proc_status;

    fnm=QFileInfo(QString::fromStdString(fname)).fileName().toStdString();
    memset(Fbfr,0,sizeof(Fbfr));
    sbstr=split(fnm,'.');
    p=Fbfr;
    tm=sbstr.at(0);
    for (i=0;i<6;i+=2,p++) {
        vl=i<tm.length()?toupper(tm[i])&077:0;
        *p=vl<<6;
        vl=i<tm.length()?toupper(tm[i+1])&077:0;
        *p|=vl;
    }
    tm=sbstr.at(1);
    vl=0<tm.length()?toupper(tm[0])&077:0;
    *p=vl<<6;
    vl=1<tm.length()?toupper(tm[1])&077:0;
    *p|=vl;
    p+=Adinfo+1;
    vl=0;

    ShowDir((unsigned long long *)Fbfr,false);     // Sync current partition and 'delete' file with matching name (XXX.YY -> XXX.DE)

    if (mode) {
    tm=unix2dos_command; // + " -n " +fname + " temp.txt";
    QStringList args;
    args << "-n" << QString::fromStdString(fname) << "temp.txt";
        proc_status = QProcess::execute(QString::fromStdString(tm),args);
	if (proc_status) {
	  QString err = QString::fromStdString(tm + " failed with status: ") + QString::number(proc_status);
	  QMessageBox::information(0,"error in WriteFile", err);
	  return;
	}
        tmfile="temp.txt";
        vl++;               // Need to add (at least) 1 byte to ascii file length for a ^Z (EOF)
    }

    QFile inf(QString::fromStdString(DskImg));
    inf.open(QIODevice::ReadWrite);
    QFile src(QString::fromStdString(tmfile));
    if(!src.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0,"error",src.errorString());
        return;
    }
    flsz=(vl+src.size())/384;    // 384 bytes per 256 word block
    if ((vl+src.size()) % 384)     // Round up if required
        flsz++;
    *p++=(-flsz)&4095;      // -Filelength+1
    // Now the empty entry
    *p++=0;
    inf.seek(Dblk*512);
    inf.read((char *)bfr,512);
    *p=bfr[Dofs+1]+flsz+1;           // Set empty entry to new length+1
    bfr[0]--;                   // Entries count
    memcpy(&bfr[Dofs],Fbfr,(Adinfo+7)*2);
    if (Dofs+Adinfo+7>254) {
        QMessageBox::information(
                    this,
                    tr("OS/8 Filesystem error"),
                    tr("Directory segment is full.") );
        inf.close();
        return;
    }
    inf.seek(Dblk*512);
    inf.write((char *)bfr,512);
    inf.seek(Fspace*512);

    vl=0;
    i=0;
    memset(bfr,0,sizeof(bfr));
    while (1) {
        memset(xtx,26,3);
        rdcnt=0;
        if (!src.atEnd())
            rdcnt=src.read((char *)xtx,3);
        if (mode)
            *(int *)xtx|=0x808080;              // Set bit 7 for text
        Pack(&bfr[vl],&bfr[vl+1]);
        vl+=2;
        if (vl==256) {
            inf.write((char *)bfr,512);
            memset(bfr,0,sizeof(bfr));
            vl=0;i++;
        }
        if (rdcnt==0)
            break;
    }
    if (vl)
        inf.write((char *)bfr,512);        // Write final block;
    inf.close();
    src.close();
    remove("temp.txt");
}


void MainWindow::on_Rg0_toggled(__attribute__((unused))bool checked)
{
        ShowDir(0,false);
}

void MainWindow::on_Rg1_toggled(__attribute__((unused))bool checked)
{

}

void MainWindow::on_CpModeA_toggled(__attribute__((unused))bool checked)
{
    // Redraw current selection in new mode.
    if (ui->CpModeA->isChecked())
        ShowFile(current_index,false);
    if (ui->CpModeB->isChecked())
        ShowBinary(current_index);
}

void MainWindow::on_CpModeB_toggled(__attribute__((unused))bool checked)
{

}

void MainWindow::on_actionSelect_Image_triggered()
{
    QString ImgName = QFileDialog::getOpenFileName(this,
            tr("Open disk image"), "",
            tr("All Files (*);;All Files (*)"));
    if (ImgName.isEmpty())
        return;
    this->setWindowTitle("OS\\8 Filesystem Viewer: " + ImgName);
    DskImg=ImgName.toStdString();
    ui->Rg0->setChecked(true);
    ui->Rg1->setChecked(false);
    ShowDir(0,false);
}

void MainWindow::on_actionExit_triggered()
{
    exit(0);
}


void MainWindow::on_pushButton_clicked()
{
    ShowDir(0,false);
}

void MainWindow::on_actionSave_View_triggered()
{
    int blk,bsz,i;

    QString Info = QFileDialog::getSaveFileName(this,
            tr("Save as"), "",
            tr("All Files (*);;All Files (*)"));
    if (Info.isEmpty())
        return;
    QFile svf(Info);
    svf.open(QIODevice::WriteOnly);

    if (ui->CpModeB->isChecked())
    {
        QFile inf(QString::fromStdString(DskImg));
        inf.open(QIODevice::ReadWrite);
        blk = fbk[ui->Flist->currentIndex().row()] + PtOfs;
        bsz = fsz[ui->Flist->currentIndex().row()];
        inf.seek(blk*512);
        while (bsz--)
        {
            inf.read((char *)bfr,512);
            for (i = 0; i <= 255; i += 2)
            {
                Unpack(bfr[i], bfr[i + 1]);
                svf.write((char *)xtx,3);
             }
        }
        svf.close();
        inf.close();
        return;
    }

    QString code = ui->View->toPlainText();
    code.replace("\r\n","\n");
    svf.write(code.toStdString().c_str(),code.length());
    svf.close();
}

void MainWindow::on_actionCopy_File_triggered()
{
    QString filename;
    QStringList Info = QFileDialog::getOpenFileNames(this,
						tr("Open file"), QDir::currentPath(),
            tr("All Files (*);;All Files (*)"));
    if (Info.isEmpty())
        return;
    for (int i=0; i< Info.size(); i++) {
        filename = Info.at(i);
	WriteFile(filename.toStdString(),ui->CpModeA->isChecked());
    }
    ShowDir(0,false);

}

void MainWindow::on_actionSave_Edit_to_image_triggered()
{

    std::string tm;

    if (!ui->CpModeA->isChecked()) {
        QMessageBox::information(
                    this,
                    tr("OS/8 Filesystem error"),
                    tr("ASCII text only.") );
        return;
    }

    tm = fname[ui->Flist->currentIndex().row()]+".TM";
    QFile svf(QString::fromStdString(tm));
    svf.open(QIODevice::WriteOnly);
    QString code = ui->View->toPlainText();
    code.replace("\r\n","\n");
    svf.write(code.toStdString().c_str(),code.length());
    svf.close();
    WriteFile(tm,true);
    svf.remove();
    ShowDir(0,false);
    ui->Flist->scrollToBottom();
}
