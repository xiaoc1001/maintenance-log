#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("客戶保養/安裝/購買紀錄系統");
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
