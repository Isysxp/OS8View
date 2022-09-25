#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QMessageBox>
#include <vector>
#include <QProcess>
#include <QClipboard>
#include <QFileInfo>
#include <QFileDialog>
#include <stdio.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0, int arg = 0, char* argv[] = 0);
    ~MainWindow();
    void ShowDir(unsigned long long*,bool);
    void ShowFile(int,bool);
    void ShowBinary(int);
    void Unpack(int,int);
    void Pack(short*,short*);
    void WriteFile(std::string,int);

private slots:

    void on_Flist_clicked(const QModelIndex &index);

    void on_Rg0_toggled(bool checked);

    void on_Rg1_toggled(bool checked);

    void on_CpModeA_toggled(bool checked);

    void on_CpModeB_toggled(bool checked);

    void on_actionSelect_Image_triggered();

    void on_actionExit_triggered();

    void on_pushButton_clicked();

    void on_actionSave_View_triggered();

    void on_actionCopy_File_triggered();

    void on_actionSave_Edit_to_image_triggered();

private:
    Ui::MainWindow *ui;
    short bfr[256];
    std::vector<unsigned char> Tmbf;
    std::string DskImg;
    std::string unix2dos_command;
    int current_index;
    unsigned char xtx[4];
    int Adinfo;
    int fbk[512];
    int fsz[512];
    std::string fname[512];
    int Fspace;
    int Dblk;
    int Dofs;
    int PtOfs;
    QStringList fnames;
public:
    int argc;
};

#endif // MAINWINDOW_H
