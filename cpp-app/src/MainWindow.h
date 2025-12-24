#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTextEdit>
#include <QWidget>

#include "ApiClient.h"

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void refreshRocDate();
    void refreshFollowups();
    void toggleFields();
    void submitRecord();
    void queryRecords();
    void waterReplace();

    QStringList selectedCheckboxes(const QList<QCheckBox *> &boxes) const;
    void updateTable(QStandardItemModel *model, const QList<QStringList> &rows, const QStringList &headers);
    void fillResults(const QJsonArray &rows, bool onlyWater);

    ApiClient apiClient;

    QTabWidget *tabs = nullptr;

    QLineEdit *serviceDateInput = nullptr;
    QLabel *rocDateLabel = nullptr;
    QLineEdit *nameInput = nullptr;
    QLineEdit *phoneInput = nullptr;
    QLineEdit *addressInput = nullptr;
    QList<QCheckBox *> purposeBoxes;
    QList<QCheckBox *> itemBoxes;
    QLineEdit *otherItemInput = nullptr;
    QLabel *waterCycleLabel = nullptr;
    QComboBox *waterCycleCombo = nullptr;
    QLineEdit *nextReplaceInput = nullptr;
    QLineEdit *warrantyEndInput = nullptr;
    QTextEdit *notesInput = nullptr;
    QLineEdit *submitResult = nullptr;
    QPushButton *submitButton = nullptr;

    QLineEdit *queryPhoneInput = nullptr;
    QCheckBox *onlyWaterCheckbox = nullptr;
    QLineEdit *queryMessage = nullptr;
    QStandardItemModel *resultsModel = nullptr;
    QStandardItemModel *latestModel = nullptr;
    QPushButton *queryButton = nullptr;

    QCheckBox *replacedConfirm = nullptr;
    QLineEdit *replaceDateInput = nullptr;
    QComboBox *replaceCycleCombo = nullptr;
    QLineEdit *replaceNoteInput = nullptr;
    QLineEdit *replaceResult = nullptr;
    QPushButton *replaceButton = nullptr;
};
