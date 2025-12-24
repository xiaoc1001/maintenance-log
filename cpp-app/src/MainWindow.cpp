#include "MainWindow.h"

#include <QDate>
#include <QDateTime>
#include <QDateTime>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>

#include "DateUtils.h"

namespace {
const QString kWaterItem = QStringLiteral("淨水設備");
const QString kGasItem = QStringLiteral("瓦斯爐具器具");

const QStringList kItems = {
    QStringLiteral("淨水設備"),
    QStringLiteral("瓦斯爐具器具"),
    QStringLiteral("系統櫃廚具"),
    QStringLiteral("水電及室內裝修工程"),
    QStringLiteral("其他（自行輸入）")
};

const QStringList kPurposes = {QStringLiteral("安裝"), QStringLiteral("購買")};

const QStringList kWaterCycles = {QStringLiteral("半年"), QStringLiteral("一年"), QStringLiteral("一年半"), QStringLiteral("兩年")};

int cycleToMonths(const QString &cycle) {
    if (cycle == QStringLiteral("半年")) {
        return 6;
    }
    if (cycle == QStringLiteral("一年")) {
        return 12;
    }
    if (cycle == QStringLiteral("一年半")) {
        return 18;
    }
    if (cycle == QStringLiteral("兩年")) {
        return 24;
    }
    return 0;
}

QStringList toStringList(const QJsonValue &value) {
    if (value.isArray()) {
        QStringList list;
        for (const auto &item : value.toArray()) {
            list.append(item.toString());
        }
        return list;
    }

    if (value.isString()) {
        const QString text = value.toString();
        if (text.trimmed().startsWith('[')) {
            QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
            if (doc.isArray()) {
                QStringList list;
                for (const auto &item : doc.array()) {
                    list.append(item.toString());
                }
                return list;
            }
        }
    }
    return {};
}

QString joinList(const QStringList &values) {
    QStringList cleaned;
    for (const auto &value : values) {
        if (!value.trimmed().isEmpty()) {
            cleaned.append(value.trimmed());
        }
    }
    return cleaned.join(" / ");
}

struct DisplayRow {
    QStringList columns;
};

} // namespace

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
    buildUi();
    refreshRocDate();
    refreshFollowups();
}

void MainWindow::buildUi() {
    tabs = new QTabWidget(this);

    auto *addTab = new QWidget(this);
    auto *addLayout = new QVBoxLayout(addTab);

    auto *dateRow = new QHBoxLayout();
    serviceDateInput = new QLineEdit(QDate::currentDate().toString("yyyy-MM-dd"), this);
    serviceDateInput->setPlaceholderText("YYYY-MM-DD");
    rocDateLabel = new QLabel(this);
    dateRow->addWidget(new QLabel("日期（YYYY-MM-DD）：", this));
    dateRow->addWidget(serviceDateInput);
    dateRow->addWidget(rocDateLabel);
    addLayout->addLayout(dateRow);

    connect(serviceDateInput, &QLineEdit::textChanged, this, &MainWindow::refreshRocDate);
    connect(serviceDateInput, &QLineEdit::textChanged, this, &MainWindow::refreshFollowups);

    auto *namePhoneRow = new QHBoxLayout();
    nameInput = new QLineEdit(this);
    phoneInput = new QLineEdit(this);
    namePhoneRow->addWidget(new QLabel("姓名：", this));
    namePhoneRow->addWidget(nameInput);
    namePhoneRow->addWidget(new QLabel("電話：", this));
    namePhoneRow->addWidget(phoneInput);
    addLayout->addLayout(namePhoneRow);

    addressInput = new QLineEdit(this);
    auto *addressRow = new QHBoxLayout();
    addressRow->addWidget(new QLabel("地址：", this));
    addressRow->addWidget(addressInput);
    addLayout->addLayout(addressRow);

    auto *purposeGroup = new QGroupBox("用途（可複選）", this);
    auto *purposeLayout = new QHBoxLayout(purposeGroup);
    for (const auto &purpose : kPurposes) {
        auto *box = new QCheckBox(purpose, this);
        if (purpose == QStringLiteral("安裝")) {
            box->setChecked(true);
        }
        purposeBoxes.append(box);
        purposeLayout->addWidget(box);
    }
    addLayout->addWidget(purposeGroup);

    auto *itemGroup = new QGroupBox("保養/工程項目（可複選）", this);
    auto *itemLayout = new QHBoxLayout(itemGroup);
    for (const auto &item : kItems) {
        auto *box = new QCheckBox(item, this);
        itemBoxes.append(box);
        itemLayout->addWidget(box);
        connect(box, &QCheckBox::stateChanged, this, &MainWindow::toggleFields);
        connect(box, &QCheckBox::stateChanged, this, &MainWindow::refreshFollowups);
    }
    addLayout->addWidget(itemGroup);

    otherItemInput = new QLineEdit(this);
    otherItemInput->setPlaceholderText("其他內容");
    otherItemInput->setVisible(false);
    addLayout->addWidget(otherItemInput);

    waterCycleCombo = new QComboBox(this);
    waterCycleCombo->addItems(kWaterCycles);
    waterCycleCombo->setVisible(false);
    connect(waterCycleCombo, &QComboBox::currentTextChanged, this, &MainWindow::refreshFollowups);
    waterCycleLabel = new QLabel("淨水設備更換週期（選 1 個）：", this);
    waterCycleLabel->setVisible(false);
    addLayout->addWidget(waterCycleLabel);
    addLayout->addWidget(waterCycleCombo);

    auto *followupRow = new QHBoxLayout();
    nextReplaceInput = new QLineEdit(this);
    warrantyEndInput = new QLineEdit(this);
    nextReplaceInput->setReadOnly(true);
    warrantyEndInput->setReadOnly(true);
    followupRow->addWidget(new QLabel("下次更換日期（民國）：", this));
    followupRow->addWidget(nextReplaceInput);
    followupRow->addWidget(new QLabel("保固期限（民國）：", this));
    followupRow->addWidget(warrantyEndInput);
    addLayout->addLayout(followupRow);

    notesInput = new QTextEdit(this);
    notesInput->setPlaceholderText("備註");
    addLayout->addWidget(notesInput);

    auto *submitButton = new QPushButton("🚀 送出新增", this);
    submitResult = new QLineEdit(this);
    submitResult->setReadOnly(true);
    addLayout->addWidget(submitButton);
    addLayout->addWidget(submitResult);

    connect(submitButton, &QPushButton::clicked, this, &MainWindow::submitRecord);

    tabs->addTab(addTab, "➕ 新增紀錄");

    auto *queryTab = new QWidget(this);
    auto *queryLayout = new QVBoxLayout(queryTab);

    auto *queryRow = new QHBoxLayout();
    queryPhoneInput = new QLineEdit(this);
    queryPhoneInput->setPlaceholderText("例如：0912345678");
    onlyWaterCheckbox = new QCheckBox("只列出淨水設備", this);
    queryRow->addWidget(new QLabel("電話（需輸入完整）：", this));
    queryRow->addWidget(queryPhoneInput);
    queryRow->addWidget(onlyWaterCheckbox);
    queryLayout->addLayout(queryRow);

    auto *queryButton = new QPushButton("查詢", this);
    queryMessage = new QLineEdit(this);
    queryMessage->setReadOnly(true);
    queryLayout->addWidget(queryButton);
    queryLayout->addWidget(queryMessage);

    resultsModel = new QStandardItemModel(this);
    auto *resultsTable = new QTableView(this);
    resultsTable->setModel(resultsModel);
    resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    queryLayout->addWidget(new QLabel("查詢結果（依民國日期降冪排序）", this));
    queryLayout->addWidget(resultsTable);

    latestModel = new QStandardItemModel(this);
    auto *latestTable = new QTableView(this);
    latestTable->setModel(latestModel);
    latestTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    queryLayout->addWidget(new QLabel("最新一筆", this));
    queryLayout->addWidget(latestTable);

    connect(queryButton, &QPushButton::clicked, this, &MainWindow::queryRecords);

    queryLayout->addWidget(new QLabel("✅ 淨水設備：更換/未更換（勾選已更換可直接新增一筆更換紀錄）", this));

    auto *replaceRow = new QHBoxLayout();
    replacedConfirm = new QCheckBox("已更換（勾選才會新增一筆更換紀錄）", this);
    replaceDateInput = new QLineEdit(QDate::currentDate().toString("yyyy-MM-dd"), this);
    replaceDateInput->setPlaceholderText("YYYY-MM-DD");
    replaceRow->addWidget(replacedConfirm);
    replaceRow->addWidget(new QLabel("更換日期：", this));
    replaceRow->addWidget(replaceDateInput);
    queryLayout->addLayout(replaceRow);

    auto *cycleRow = new QHBoxLayout();
    replaceCycleCombo = new QComboBox(this);
    replaceCycleCombo->addItems(kWaterCycles);
    replaceNoteInput = new QLineEdit(this);
    replaceNoteInput->setPlaceholderText("例如：更換濾心/更換RO膜…");
    cycleRow->addWidget(new QLabel("更換週期：", this));
    cycleRow->addWidget(replaceCycleCombo);
    cycleRow->addWidget(new QLabel("更換備註：", this));
    cycleRow->addWidget(replaceNoteInput);
    queryLayout->addLayout(cycleRow);

    auto *replaceButton = new QPushButton("🧾 新增『淨水更換』紀錄並刷新", this);
    replaceResult = new QLineEdit(this);
    replaceResult->setReadOnly(true);
    queryLayout->addWidget(replaceButton);
    queryLayout->addWidget(replaceResult);

    connect(replaceButton, &QPushButton::clicked, this, &MainWindow::waterReplace);

    tabs->addTab(queryTab, "🔍 查詢（完整電話）");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);
    setLayout(mainLayout);

    toggleFields();
}

void MainWindow::refreshRocDate() {
    const QString text = serviceDateInput->text().trimmed();
    if (!DateUtils::isYmd(text)) {
        rocDateLabel->setText("民國日期：（日期格式錯誤，請用 YYYY-MM-DD）");
        return;
    }
    QDate date = DateUtils::parseYmd(text);
    rocDateLabel->setText(QString("民國日期：%1").arg(DateUtils::dateToRoc(date)));
}

void MainWindow::refreshFollowups() {
    QDate date = DateUtils::parseYmd(serviceDateInput->text().trimmed());
    QStringList items = selectedCheckboxes(itemBoxes);

    QString nextReplace;
    QString warrantyEnd;

    if (date.isValid() && items.contains(kWaterItem)) {
        int months = cycleToMonths(waterCycleCombo->currentText());
        if (months > 0) {
            nextReplace = DateUtils::dateToRoc(DateUtils::addMonths(date, months));
        }
    }

    if (date.isValid() && items.contains(kGasItem)) {
        warrantyEnd = DateUtils::dateToRoc(DateUtils::addOneYear(date));
    }

    nextReplaceInput->setText(nextReplace);
    warrantyEndInput->setText(warrantyEnd);
}

void MainWindow::toggleFields() {
    QStringList items = selectedCheckboxes(itemBoxes);
    bool showOther = items.contains("其他（自行輸入）");
    bool showCycle = items.contains(kWaterItem);

    otherItemInput->setVisible(showOther);
    waterCycleCombo->setVisible(showCycle);
    waterCycleLabel->setVisible(showCycle);
}

QStringList MainWindow::selectedCheckboxes(const QList<QCheckBox *> &boxes) const {
    QStringList values;
    for (auto *box : boxes) {
        if (box->isChecked()) {
            values.append(box->text());
        }
    }
    return values;
}

void MainWindow::submitRecord() {
    QString dateText = serviceDateInput->text().trimmed();
    if (!DateUtils::isYmd(dateText)) {
        submitResult->setText("❌ 日期格式錯誤，請用 YYYY-MM-DD");
        return;
    }

    QDate date = DateUtils::parseYmd(dateText);
    if (!date.isValid()) {
        submitResult->setText("❌ 日期解析失敗，請確認 YYYY-MM-DD 是否為有效日期");
        return;
    }

    if (nameInput->text().trimmed().isEmpty() || phoneInput->text().trimmed().isEmpty()) {
        submitResult->setText("❌ 必填：姓名、電話");
        return;
    }

    QStringList purposes = selectedCheckboxes(purposeBoxes);
    QStringList items = selectedCheckboxes(itemBoxes);

    if (items.contains("其他（自行輸入）") && otherItemInput->text().trimmed().isEmpty()) {
        submitResult->setText("❌ 你有勾選「其他（自行輸入）」但未填內容");
        return;
    }

    QString waterCycle;
    if (items.contains(kWaterItem)) {
        waterCycle = waterCycleCombo->currentText();
        if (cycleToMonths(waterCycle) <= 0) {
            submitResult->setText("❌ 請選擇更換週期（半年/一年/一年半/兩年）");
            return;
        }
    }

    QJsonObject data;
    data.insert("service_date_ad", DateUtils::dateToIso(date));
    data.insert("service_date_roc", DateUtils::dateToRoc(date));
    data.insert("customer_name", nameInput->text().trimmed());
    data.insert("phone", phoneInput->text().trimmed());
    data.insert("address", addressInput->text().trimmed());

    QJsonArray purposeArray;
    for (const auto &purpose : purposes) {
        purposeArray.append(purpose);
    }
    data.insert("purposes", purposeArray);

    QJsonArray itemArray;
    for (const auto &item : items) {
        itemArray.append(item);
    }
    data.insert("items", itemArray);
    data.insert("other_item_text", otherItemInput->text().trimmed());
    data.insert("water_replace_cycle", waterCycle);
    data.insert("next_replace_date_roc", nextReplaceInput->text().trimmed());
    data.insert("warranty_end_date_roc", warrantyEndInput->text().trimmed());
    data.insert("notes", notesInput->toPlainText().trimmed());
    data.insert("created_at", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    ApiClient::Result result = apiClient.postRecord(data);
    submitResult->setText(result.message);
}

void MainWindow::queryRecords() {
    QString phone = queryPhoneInput->text().trimmed();
    if (phone.isEmpty()) {
        queryMessage->setText("❌ 請輸入完整電話");
        resultsModel->clear();
        latestModel->clear();
        return;
    }

    bool onlyWater = onlyWaterCheckbox->isChecked();
    ApiClient::Result result = apiClient.getRecords(phone, onlyWater);
    if (!result.ok) {
        queryMessage->setText(result.message);
        resultsModel->clear();
        latestModel->clear();
        return;
    }

    if (result.rows.isEmpty()) {
        queryMessage->setText("查無資料");
        resultsModel->clear();
        latestModel->clear();
        return;
    }

    fillResults(result.rows, onlyWater);
    queryMessage->setText("✅ 已依民國日期降冪排序");
}

void MainWindow::updateTable(QStandardItemModel *model, const QList<QStringList> &rows, const QStringList &headers) {
    model->clear();
    model->setColumnCount(headers.size());
    model->setHorizontalHeaderLabels(headers);

    for (const auto &row : rows) {
        QList<QStandardItem *> items;
        for (const auto &cell : row) {
            items.append(new QStandardItem(cell));
        }
        model->appendRow(items);
    }
}

void MainWindow::fillResults(const QJsonArray &rows, bool onlyWater) {
    struct Record {
        QJsonObject obj;
        QDate rocDate;
        QDateTime createdAt;
    };

    QVector<Record> records;
    records.reserve(rows.size());

    for (const auto &value : rows) {
        if (!value.isObject()) {
            continue;
        }
        QJsonObject obj = value.toObject();
        QString rocText = obj.value("service_date_roc").toString();
        QString normalized = DateUtils::normalizeRocStr(rocText);
        QDate rocDate = DateUtils::rocToAdDate(normalized);
        QDateTime createdAt = QDateTime::fromString(obj.value("created_at").toString(), "yyyy-MM-dd HH:mm:ss");

        records.push_back({obj, rocDate, createdAt});
    }

    std::sort(records.begin(), records.end(), [](const Record &a, const Record &b) {
        if (a.rocDate != b.rocDate) {
            return a.rocDate > b.rocDate;
        }
        return a.createdAt > b.createdAt;
    });

    QVector<int> waterIndices;
    for (int i = 0; i < records.size(); ++i) {
        QStringList items = toStringList(records[i].obj.value("items"));
        if (items.contains(kWaterItem)) {
            waterIndices.append(i);
        }
    }

    QList<QStringList> displayRows;
    displayRows.reserve(records.size());

    for (int i = 0; i < records.size(); ++i) {
        const auto &record = records[i];
        const QJsonObject &obj = record.obj;

        QStringList purposes = toStringList(obj.value("purposes"));
        QStringList items = toStringList(obj.value("items"));

        if (onlyWater && !items.contains(kWaterItem)) {
            continue;
        }

        QString itemsDisplay = onlyWater ? kWaterItem : joinList(items);

        QString nextReplace = obj.value("next_replace_date_roc").toString().trimmed();
        QString warrantyEnd = obj.value("warranty_end_date_roc").toString().trimmed();
        QString followup;
        if (onlyWater) {
            followup = nextReplace.isEmpty() ? QString() : QString("更換：%1").arg(nextReplace);
        } else if (!nextReplace.isEmpty() && !warrantyEnd.isEmpty()) {
            followup = QString("更換：%1 / 保固：%2").arg(nextReplace, warrantyEnd);
        } else if (!nextReplace.isEmpty()) {
            followup = QString("更換：%1").arg(nextReplace);
        } else if (!warrantyEnd.isEmpty()) {
            followup = QString("保固：%1").arg(warrantyEnd);
        }

        QString waterStatus;
        int waterIndex = waterIndices.indexOf(i);
        if (waterIndex == 0) {
            waterStatus = "未更換";
        } else if (waterIndex > 0) {
            waterStatus = "已更換";
        }

        QString normalizedRoc = DateUtils::normalizeRocStr(obj.value("service_date_roc").toString());

        displayRows.append({
            normalizedRoc,
            obj.value("customer_name").toString(),
            obj.value("phone").toString(),
            obj.value("address").toString(),
            joinList(purposes),
            itemsDisplay,
            waterStatus,
            followup,
            obj.value("notes").toString()
        });
    }

    QStringList headers = {
        "日期(民國)",
        "姓名",
        "電話",
        "地址",
        "用途(安裝/購買)",
        "項目",
        "淨水狀態",
        "更換日期或保固期限",
        "備註"
    };

    updateTable(resultsModel, displayRows, headers);

    QList<QStringList> latest;
    if (!displayRows.isEmpty()) {
        latest.append(displayRows.first());
    }
    updateTable(latestModel, latest, headers);
}

void MainWindow::waterReplace() {
    QString phone = queryPhoneInput->text().trimmed();
    QString replaceDateText = replaceDateInput->text().trimmed();
    QString cycleChoice = replaceCycleCombo->currentText();
    QString extraNote = replaceNoteInput->text().trimmed();

    if (phone.isEmpty()) {
        replaceResult->setText("❌ 請輸入完整電話");
        return;
    }

    if (!replacedConfirm->isChecked()) {
        replaceResult->setText("ℹ️ 未勾選「已更換」，未新增");
        return;
    }

    if (!DateUtils::isYmd(replaceDateText)) {
        replaceResult->setText("❌ 更換日期格式錯誤，請用 YYYY-MM-DD");
        return;
    }

    if (cycleToMonths(cycleChoice) <= 0) {
        replaceResult->setText("❌ 請選擇更換週期（半年/一年/一年半/兩年）");
        return;
    }

    ApiClient::Result rawResult = apiClient.fetchRaw(phone);
    if (!rawResult.ok) {
        replaceResult->setText(QString("❌ 讀取原始資料失敗：%1").arg(rawResult.message));
        return;
    }

    if (rawResult.rows.isEmpty()) {
        replaceResult->setText("❌ 查無此電話資料，無法建立更換紀錄");
        return;
    }

    QJsonObject latestRecord;
    QDateTime latestCreated;
    for (const auto &value : rawResult.rows) {
        if (!value.isObject()) {
            continue;
        }
        QJsonObject obj = value.toObject();
        QDateTime createdAt = QDateTime::fromString(obj.value("created_at").toString(), "yyyy-MM-dd HH:mm:ss");
        if (!latestCreated.isValid() || createdAt > latestCreated) {
            latestCreated = createdAt;
            latestRecord = obj;
        }
    }

    QDate replaceDate = DateUtils::parseYmd(replaceDateText);
    QString nextReplace = DateUtils::dateToRoc(DateUtils::addMonths(replaceDate, cycleToMonths(cycleChoice)));

    QString note = "淨水設備更換";
    if (!extraNote.isEmpty()) {
        note = QString("%1｜%2").arg(note, extraNote);
    }

    QJsonObject data;
    data.insert("service_date_ad", DateUtils::dateToIso(replaceDate));
    data.insert("service_date_roc", DateUtils::dateToRoc(replaceDate));
    data.insert("customer_name", latestRecord.value("customer_name").toString());
    data.insert("phone", phone);
    data.insert("address", latestRecord.value("address").toString());

    QJsonArray purposeArray;
    purposeArray.append("安裝");
    data.insert("purposes", purposeArray);

    QJsonArray itemArray;
    itemArray.append(kWaterItem);
    data.insert("items", itemArray);
    data.insert("other_item_text", "");
    data.insert("water_replace_cycle", cycleChoice);
    data.insert("next_replace_date_roc", nextReplace);
    data.insert("warranty_end_date_roc", "");
    data.insert("notes", note);
    data.insert("created_at", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    ApiClient::Result postResult = apiClient.postRecord(data);
    if (!postResult.ok) {
        replaceResult->setText(QString("❌ 新增更換紀錄失敗：%1").arg(postResult.message));
        return;
    }

    replaceResult->setText(QString("✅ 已新增一筆『淨水設備更換』紀錄（下次更換：%1）").arg(nextReplace));
    queryRecords();
}
