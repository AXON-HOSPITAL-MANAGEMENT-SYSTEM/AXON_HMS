#include "doctorwindow.h"
#include "ui_doctorwindow.h"
#include "mainwindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QDateTime>
#include <QFrame>
#include <QScrollArea>
#include <QComboBox>
#include <QDebug>
#include<QMessageBox>
#include <QDialog>
#include <QGroupBox>
#include <QTextEdit>


static const QString kPlainLabelD =
    "border:none;background:transparent;background-color:transparent;";

static void applyStatusBadgeD(QLabel *label, const QString &status) {
    label->setText(status.toUpper());
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-weight:bold;font-size:11px;padding:4px 8px;border:none;"
                         "background-color:#e8f0fe;color:#1a4f8a;border-radius:10px;");
}

// ===== CONSTRUCTOR =====
doctorwindow::doctorwindow(const QString &employeeName, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::doctorwindow),
    currentUserName(employeeName)
{
    ui->setupUi(this);
    QString displayName = employeeName;
    if (displayName.isEmpty()) {
        displayName = currentUserName;
    }

    // Capitalize properly if needed
    QStringList nameParts = displayName.split(" ", Qt::SkipEmptyParts);
    for (int i = 0; i < nameParts.size(); ++i) {
        if (nameParts[i].length() > 1) {
            nameParts[i] = nameParts[i].left(1).toUpper() + nameParts[i].mid(1).toLower();
        }
    }
    QString formattedName = nameParts.join(" ");

    if (ui->message)
        ui->message->setText(QString("Welcome, Dr. %1!").arg(formattedName));

    // Shared backend — same CSVs Admin & Receptionist windows read/write.
    patientMgr = new PatientManager();
    apptMgr    = new AppointmentManager();

    ui->overviewBtn->setCheckable(true);
    ui->patientlistBtn->setCheckable(true);
    ui->scheduleBtn->setCheckable(true);
    ui->overviewBtn->setChecked(true);

    // Clock
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &doctorwindow::updateDateTime);
    updateDateTime();
    timer->start(1000);

    // Sidebar Navigation
    connect(ui->overviewBtn, &QPushButton::clicked, this, &doctorwindow::switchPage);
    connect(ui->patientlistBtn, &QPushButton::clicked, this, &doctorwindow::switchPage);
    connect(ui->scheduleBtn, &QPushButton::clicked, this, &doctorwindow::switchPage);
    ui->stackedWidget->setCurrentIndex(0);

    // Logout Menu
    QMenu *logoutMenu = new QMenu(this);
    logoutMenu->addAction("  Logout  ", this, &doctorwindow::logout);
    ui->logoutBtn->setMenu(logoutMenu);

    logoutMenu->setStyleSheet(
        "QMenu {"
        "background-color: #ffffff ;"
        "color:#2c3e50;"
        "border: 1 px solid #dce1e6;"
        "border-radius: 10 px;"
        "padding: 8 px;"
        "width: 120 px;"
        "height:50 px;"
        "}"
        "QMenu::item {"
        "padding: 0 px;"
        "border-radius: 6 px;"
        "margin: 0 px;"
        "font-size: 13 px;"
        "color: 2c3e50;"
        "width: 100 px;"
        "height: 40 px;"
        "text-align:center;"
        "}"
        "QMenu::item:selected {"
        "background-color: #e8f0fe;"
        "color: #1a73e8;"
        "}"
        );

    // Apply light theme
    this->setStyleSheet(
        "QWidget {"
        "   background-color: #f8f9fc;"
        "   font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QListWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 10px;"
        "   padding: 5px;"
        "}"
        "QListWidget::item {"
        "   padding: 10px 8px;"
        "   border-bottom: 1px solid #f2f4f7;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #f0f4f9;"
        "}"
        "QTableWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 10px;"
        "   gridline-color: #f2f4f7;"
        "}"
        "QTableWidget::item {"
        "   padding: 8px;"
        "   color: #2c3e50;"
        "}"
        "QTableWidget::item:selected {"
        "   background-color: #e8f0fe;"
        "   color: #1a73e8;"
        "}"
        "QHeaderView::section {"
        "   background-color: #f8f9fc;"
        "   padding: 8px;"
        "   border: none;"
        "   font-weight: 600;"
        "   color: #5a6c7d;"
        "}"
        "#sideBarFrame {"
        "   background-color: #ffffff;"
        "   border-right: 1px solid #e9ecef;"
        "}"
        "#sideBarFrame QPushButton {"
        "   background-color: transparent;"
        "   color: #5a6c7d;"
        "   text-align: left;"
        "   padding: 12px 20px;"
        "   border: none;"
        "   border-radius: 0px;"
        "   font-size: 14px;"
        "   font-weight: 500;"
        "}"
        "#sideBarFrame QPushButton:hover {"
        "   background-color: #eef3f7;"
        "   color: #1a73e8;"
        "}"
        "#sideBarFrame QPushButton:checked {"
        "   background-color: #e8f0fe;"
        "   color: #2c3e50;"
        "   border-left: 4px solid #1a73e8;"
        "   font-weight: 600;"
        "}"
        );

    // Setup UI
    setupStatsSection();
    populatePatientList();
    populateAppointmentsTable();
    setupOverviewLayout();

    // Setup other pages
    setupPatientListPage();
    setupSchedulingPage();

    doctorBackend = new Doctor();
    this->setWindowTitle("AXON-HMS: Doctor Dashboard");
}

// ===== DESTRUCTOR =====
doctorwindow::~doctorwindow()
{
    delete doctorBackend;
    delete patientMgr;
    delete apptMgr;
    delete ui;
}

// ===== UPDATE CLOCK =====
void doctorwindow::updateDateTime()
{
    QDateTime now = QDateTime::currentDateTime();
    QString text = now.toString("ddd MMM d yyyy, hh:mm AP");
    ui->label_clock->setText(text);
}

// ===== SWITCH PAGE =====
void doctorwindow::switchPage()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());

    // Uncheck all buttons first
    ui->overviewBtn->setChecked(false);
    ui->patientlistBtn->setChecked(false);
    ui->scheduleBtn->setChecked(false);

    if(btn == ui->overviewBtn) {
        ui->stackedWidget->setCurrentIndex(0);
        ui->overviewBtn->setChecked(true);
        patientMgr->reload();
        apptMgr->reload();
        populatePatientList();
        populateAppointmentsTable();
        setupStatsSection();
    } else if(btn == ui->patientlistBtn) {
        ui->stackedWidget->setCurrentIndex(1);
        ui->patientlistBtn->setChecked(true);
        patientMgr->reload();
        refreshPatientList();
    } else if(btn == ui->scheduleBtn) {
        ui->stackedWidget->setCurrentIndex(2);
        ui->scheduleBtn->setChecked(true);
        apptMgr->reload();
        refreshSchedulingTable();
    }
}

// ===== LOGOUT =====
void doctorwindow::logout()
{
    this->close();
    MainWindow *login = new MainWindow();
    login->show();
}

void doctorwindow::setupOverviewLayout()
{
    QWidget *overviewPage = ui->pageOverview;
    if (!overviewPage) return;

    // Clear existing layout
    QLayout *oldLayout = overviewPage->layout();
    if (oldLayout) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            // Don't delete widgets
        }
        delete oldLayout;
    }

    // Reparent all widgets to the page
    ui->statsContainer->setParent(overviewPage);
    ui->lblPatientsTitle->setParent(overviewPage);
    ui->patientListWidget->setParent(overviewPage);
    ui->lblAppointmentsTtitle->setParent(overviewPage);
    ui->appointmentsTable->setParent(overviewPage);

    // Set size policies
    ui->statsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->statsContainer->setFixedHeight(170);

    ui->patientListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->appointmentsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ===== MAIN LAYOUT =====
    QVBoxLayout *mainLayout = new QVBoxLayout(overviewPage);
    mainLayout->setSpacing(8);  // Reduced spacing
    mainLayout->setContentsMargins(15, 8, 15, 15);

    // Stats
    mainLayout->addWidget(ui->statsContainer);

    // ===== CONTENT =====
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(12);

    // ----- LEFT: My Patients -----
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(4);

    ui->lblPatientsTitle->setText("My Patients");
    ui->lblPatientsTitle->setStyleSheet(
        "font-size: 13px;"
        "font-weight: bold;"
        "color: #2c3e50;"
        );
    leftLayout->addWidget(ui->lblPatientsTitle);

    ui->patientListWidget->setMinimumHeight(80);
    leftLayout->addWidget(ui->patientListWidget, 1);

    // ----- RIGHT: Today's Appointments -----
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(4);

    ui->lblAppointmentsTtitle->setText("Today's Appointments");
    ui->lblAppointmentsTtitle->setStyleSheet(
        "font-size: 13px;"
        "font-weight: bold;"
        "color: #2c3e50;"
        );
    rightLayout->addWidget(ui->lblAppointmentsTtitle);

    ui->appointmentsTable->setMinimumHeight(80);
    rightLayout->addWidget(ui->appointmentsTable, 1);

    contentLayout->addLayout(leftLayout, 1);
    contentLayout->addLayout(rightLayout, 1);

    mainLayout->addLayout(contentLayout, 1);

    overviewPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
// ===== SETUP STATS SECTION =====
void doctorwindow::setupStatsSection()
{
    QLayout *oldLayout = ui->statsContainer->layout();
    if (oldLayout) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if(item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete oldLayout;
    }

    QHBoxLayout *newLayout = new QHBoxLayout(ui->statsContainer);
    newLayout->setSpacing(15);
    newLayout->setContentsMargins(0, 0, 0, 0);

    // Get real data from PatientManager
    patientMgr->reload();
    apptMgr->reload();

    QVector<Patient> myPatients;
    for (const auto &p : patientMgr->getAllPatients()) {
        if (p.assignedDoctor.trimmed().compare(currentUserName.trimmed(), Qt::CaseInsensitive) == 0)
            myPatients.append(p);
    }

    int totalPatients = myPatients.size();
    int admitted = 0, discharged = 0;
    for (const auto &p : myPatients) {
        if (p.status.trimmed().compare("Discharged", Qt::CaseInsensitive) == 0)
            discharged++;
        else
            admitted++;
    }

    // Get today's appointments for this doctor
    int todaysAppointments = 0;
    int pendingTreatments = 0;
    for (const auto &a : apptMgr->getTodaysAppointments()) {
        if (a.doctorName.trimmed().compare(currentUserName.trimmed(), Qt::CaseInsensitive) == 0) {
            todaysAppointments++;
            if (a.status.trimmed().compare("Pending", Qt::CaseInsensitive) == 0 ||
                a.status.trimmed().compare("Waiting", Qt::CaseInsensitive) == 0) {
                pendingTreatments++;
            }
        }
    }

    // 4 Stats with accent colors
    struct StatData {
        QString number;
        QString label;
        QString subtitle;
        QString accentColor;
    };

    QList<StatData> stats = {
        {QString::number(totalPatients), "Total Patients", "Under your care", "#1a73e8"},
        {QString::number(todaysAppointments), "Today's Appointments", "Scheduled", "#1e8449"},
        {QString::number(pendingTreatments), "Pending Treatments", "Need action", "#a04000"},
        {QString::number(discharged), "Completed Today", "Discharged", "#6c3483"}
    };

    for (const StatData &data : stats) {
        QWidget *card = new QWidget();
        card->setObjectName("statCard");
        card->setStyleSheet(
            QString(
                "QWidget#statCard {"
                "   background-color: #ffffff;"
                "   border: 1px solid #e9ecef;"
                "   border-left: 4px solid %1;"
                "   border-radius: 8px;"
                "   padding: 15px;"
                "   min-width: 150px;"
                "   min-height: 100px;"
                "}"
                ).arg(data.accentColor)
            );

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setAlignment(Qt::AlignCenter);
        cardLayout->setSpacing(3);

        QLabel *numberLabel = new QLabel(data.number);
        numberLabel->setStyleSheet(
            "font-size: 28px;"
            "font-weight: bold;"
            "color: #2c3e50;"
            );
        numberLabel->setAlignment(Qt::AlignCenter);

        QLabel *labelText = new QLabel(data.label);
        labelText->setStyleSheet(
            "font-size: 13px;"
            "color: #7f8c8d;"
            );
        labelText->setAlignment(Qt::AlignCenter);

        QLabel *subtitleLabel = new QLabel(data.subtitle);
        subtitleLabel->setStyleSheet(
            "font-size: 11px;"
            "color: #95a5a6;"
            );
        subtitleLabel->setAlignment(Qt::AlignCenter);

        cardLayout->addWidget(numberLabel);
        cardLayout->addWidget(labelText);
        cardLayout->addWidget(subtitleLabel);

        ui->statsContainer->layout()->addWidget(card);
    }
}

// ===== POPULATE PATIENT LIST (Overview Page) =====
void doctorwindow::populatePatientList()
{
    patientMgr->reload();
    ui->patientListWidget->clear();

    ui->patientListWidget->setStyleSheet(
        "QListWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 8px;"
        "   padding: 2px;"
        "}"
        "QListWidget::item {"
        "   padding: 4px 8px;"
        "   border-bottom: 1px solid #f1f5f9;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #f8fafc;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #f0f4f9;"
        "}"
        );

    for (const auto &p : patientMgr->getAllPatients()) {
        if (p.assignedDoctor.trimmed().compare(currentUserName.trimmed(), Qt::CaseInsensitive) != 0)
            continue;

        QWidget *itemWidget = new QWidget();
        QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(4, 2, 4, 2);  // Minimal padding
        itemLayout->setSpacing(8);

        QString diagnosisText = p.diagnosisTreatment.isEmpty() ? "No diagnosis" : p.diagnosisTreatment;

        QString displayText = QString("%1 — %2")
                                  .arg(p.name)
                                  .arg(diagnosisText);

        QLabel *infoLabel = new QLabel(displayText);
        infoLabel->setStyleSheet(
            "font-size: 12px;"
            "font-weight: 500;"
            "color: #1e293b;"
            );
        infoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QLabel *statusLabel = new QLabel(p.status.isEmpty() ? "UNKNOWN" : p.status.toUpper());
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setStyleSheet(
            "font-weight: bold;"
            "font-size: 10px;"
            "padding: 2px 10px;"
            "border: none;"
            "background-color: #e8f0fe;"
            "color: #1a4f8a;"
            "border-radius: 10px;"
            );
        statusLabel->setFixedWidth(90);
        statusLabel->setFixedHeight(22);

        QPushButton *viewBtn = new QPushButton("View");
        viewBtn->setObjectName("viewBtn");
        viewBtn->setStyleSheet(
            "QPushButton#viewBtn {"
            "   background-color: #e8f0fe;"
            "   color: #1a73e8;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 2px 12px;"
            "   font-size: 11px;"
            "   font-weight: 500;"
            "}"
            "QPushButton#viewBtn:hover {"
            "   background-color: #d2e3fc;"
            "}"
            );
        viewBtn->setFixedHeight(24);
        viewBtn->setFixedWidth(55);
        viewBtn->setCursor(Qt::PointingHandCursor);
        viewBtn->setProperty("patientId", p.id);

        connect(viewBtn, &QPushButton::clicked, this, [this, p]() {
            viewPatientDetails(p.id);
        });

        itemLayout->addWidget(infoLabel, 1);
        itemLayout->addWidget(statusLabel);
        itemLayout->addWidget(viewBtn);

        QListWidgetItem *listItem = new QListWidgetItem(ui->patientListWidget);
        listItem->setSizeHint(QSize(itemWidget->sizeHint().width(), 40));
        ui->patientListWidget->setItemWidget(listItem, itemWidget);
    }

    if (ui->patientListWidget->count() == 0) {
        QListWidgetItem *emptyItem = new QListWidgetItem(ui->patientListWidget);
        emptyItem->setText("No patients currently assigned.");
        emptyItem->setTextAlignment(Qt::AlignCenter);
        emptyItem->setForeground(QColor("#95a5a6"));
        emptyItem->setSizeHint(QSize(ui->patientListWidget->width(), 40));
        ui->patientListWidget->addItem(emptyItem);
    }
}

// ===== POPULATE APPOINTMENTS TABLE (Overview Page) =====
void doctorwindow::populateAppointmentsTable()
{
    apptMgr->reload();

    ui->appointmentsTable->clearContents();
    ui->appointmentsTable->setRowCount(0);
    ui->appointmentsTable->setColumnCount(4);
    ui->appointmentsTable->setHorizontalHeaderLabels(
        QStringList() << "Time" << "Patient" << "Reason" << "Status"
        );

    ui->appointmentsTable->setStyleSheet(
        "QTableWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 8px;"
        "   gridline-color: #f1f5f9;"
        "}"
        "QTableWidget::item {"
        "   padding: 4px 8px;"
        "   color: #1e293b;"
        "}"
        "QTableWidget::item:hover {"
        "   background-color: #f8fafc;"
        "}"
        "QHeaderView::section {"
        "   background-color: #f8fafc;"
        "   padding: 6px 8px;"
        "   border: none;"
        "   font-weight: 600;"
        "   font-size: 11px;"
        "   color: #94a3b8;"
        "   text-transform: uppercase;"
        "}"
        );

    ui->appointmentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->appointmentsTable->verticalHeader()->setDefaultSectionSize(32);  // Compact rows

    QVector<Appointment> todaysForDoctor;
    for (const auto &a : apptMgr->getTodaysAppointments()) {
        if (a.doctorName.trimmed().compare(currentUserName.trimmed(), Qt::CaseInsensitive) == 0)
            todaysForDoctor.append(a);
    }

    for (int row = 0; row < todaysForDoctor.size(); row++) {
        const Appointment &a = todaysForDoctor[row];
        ui->appointmentsTable->insertRow(row);

        QTableWidgetItem *timeItem = new QTableWidgetItem(a.time);
        QTableWidgetItem *patientItem = new QTableWidgetItem(a.patientName);
        QTableWidgetItem *reasonItem = new QTableWidgetItem(a.reason);
        QTableWidgetItem *statusItem = new QTableWidgetItem(a.status);

        timeItem->setTextAlignment(Qt::AlignCenter);
        patientItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        reasonItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        statusItem->setTextAlignment(Qt::AlignCenter);

        QString status = a.status.trimmed();
        if (status.compare("Confirmed", Qt::CaseInsensitive) == 0) {
            statusItem->setBackground(QColor("#eafaf1"));
            statusItem->setForeground(QColor("#1e8449"));
        } else if (status.compare("Waiting", Qt::CaseInsensitive) == 0 ||
                   status.compare("Pending", Qt::CaseInsensitive) == 0) {
            statusItem->setBackground(QColor("#fdf2e9"));
            statusItem->setForeground(QColor("#a04000"));
        } else if (status.compare("Now", Qt::CaseInsensitive) == 0) {
            statusItem->setBackground(QColor("#fef9e7"));
            statusItem->setForeground(QColor("#b7950b"));
        } else if (status.compare("Cancelled", Qt::CaseInsensitive) == 0) {
            statusItem->setForeground(QColor("#e74c3c"));
        } else if (status.compare("Completed", Qt::CaseInsensitive) == 0) {
            statusItem->setBackground(QColor("#e8f0fe"));
            statusItem->setForeground(QColor("#1a4f8a"));
        }

        ui->appointmentsTable->setItem(row, 0, timeItem);
        ui->appointmentsTable->setItem(row, 1, patientItem);
        ui->appointmentsTable->setItem(row, 2, reasonItem);
        ui->appointmentsTable->setItem(row, 3, statusItem);
    }

    ui->appointmentsTable->verticalHeader()->setVisible(false);
    ui->appointmentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->appointmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->appointmentsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    if (ui->appointmentsTable->rowCount() == 0) {
        ui->appointmentsTable->setRowCount(1);
        QTableWidgetItem *emptyItem = new QTableWidgetItem("No appointments today.");
        emptyItem->setTextAlignment(Qt::AlignCenter);
        emptyItem->setForeground(QColor("#95a5a6"));
        ui->appointmentsTable->setSpan(0, 0, 1, 4);
        ui->appointmentsTable->setItem(0, 0, emptyItem);
    }
}

// ===== VIEW PATIENT DETAILS =====
void doctorwindow::viewPatientDetails(const QString &patientId)
{
    Patient p = patientMgr->searchPatient(patientId);
    if (p.id.isEmpty()) {
        QMessageBox::warning(this, "Patient Not Found",
                             "No patient found with ID: " + patientId);
        return;
    }

    // Show patient details in a message box
    QString details = QString(
                          "Patient ID: %1\n"
                          "Name: %2\n"
                          "Age: %3\n"
                          "Gender: %4\n"
                          "Doctor: %5\n"
                          "Diagnosis: %6\n"
                          "Status: %7"
                          ).arg(p.id, p.name, p.age, p.gender, p.assignedDoctor,
                               p.diagnosisTreatment.isEmpty() ? "N/A" : p.diagnosisTreatment,
                               p.status.isEmpty() ? "Unknown" : p.status);

    QMessageBox::information(this, "Patient Details", details);
}

// ===== PATIENT LIST PAGE =====
void doctorwindow::setupPatientListPage()
{
    QWidget *page = ui->pagePatient;
    if (!page) return;

    // Clear existing layout
    if (page->layout()) {
        QLayoutItem *item;
        while ((item = page->layout()->takeAt(0))) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete page->layout();
    }

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(20, 16, 20, 16);
    pageLayout->setSpacing(12);

    // Title
    QLabel *title = new QLabel("My Patients");
    title->setStyleSheet("font-size:20px;font-weight:700;color:#0f172a;border:none;background:transparent;");
    pageLayout->addWidget(title);

    // Count label
    patientCountLabel = new QLabel();
    patientCountLabel->setStyleSheet("font-size:13px;color:#64748b;border:none;background:transparent;");
    pageLayout->addWidget(patientCountLabel);

    // ===== CREATE THE TABLE =====
    QTableWidget *patientTable = new QTableWidget();
    patientTable->setObjectName("patientTable");  // ← CRITICAL: Must match findChild
    patientTable->setColumnCount(7);
    patientTable->setHorizontalHeaderLabels(
        QStringList() << "PATIENT ID" << "NAME" << "AGE" << "GENDER"
                      << "DIAGNOSIS/PROBLEM" << "STATUS" << "ACTION"
        );

    // Set column widths
    patientTable->setColumnWidth(0, 100);
    patientTable->setColumnWidth(1, 160);
    patientTable->setColumnWidth(2, 50);
    patientTable->setColumnWidth(3, 80);
    patientTable->setColumnWidth(4, 220);
    patientTable->setColumnWidth(5, 110);
    patientTable->setColumnWidth(6, 100);

    patientTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    patientTable->setStyleSheet(
        "QTableWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 8px;"
        "   gridline-color: #f1f5f9;"
        "}"
        "QTableWidget::item {"
        "   padding: 8px 10px;"
        "   color: #2c3e50;"
        "}"
        "QTableWidget::item:selected {"
        "   background-color: #e8f0fe;"
        "}"
        "QHeaderView::section {"
        "   background-color: #f8f9fc;"
        "   padding: 8px 10px;"
        "   border: none;"
        "   font-weight: 600;"
        "   font-size: 11px;"
        "   color: #94a3b8;"
        "   text-transform: uppercase;"
        "}"
        );

    patientTable->verticalHeader()->setVisible(false);
    patientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    patientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    patientTable->setSelectionMode(QAbstractItemView::SingleSelection);

    pageLayout->addWidget(patientTable, 1);

    // Populate the table
    refreshPatientTable(patientTable);
}

//Refresh Patient Table
void doctorwindow::refreshPatientTable(QTableWidget *table)
{
    if (!table) return;

    table->setRowCount(0);
    int count = 0;

    for (const auto &p : patientMgr->getAllPatients()) {
        if (p.assignedDoctor.trimmed().compare(currentUserName.trimmed(), Qt::CaseInsensitive) != 0)
            continue;

        int row = table->rowCount();
        table->insertRow(row);
        table->setRowHeight(row, 40);

        table->setItem(row, 0, new QTableWidgetItem(p.id));
        table->setItem(row, 1, new QTableWidgetItem(p.name));
        table->setItem(row, 2, new QTableWidgetItem(p.age));
        table->setItem(row, 3, new QTableWidgetItem(p.gender));
        table->setItem(row, 4, new QTableWidgetItem(p.diagnosisTreatment));
        table->setItem(row, 5, new QTableWidgetItem(p.status.toUpper()));

        // Status color
        QTableWidgetItem *statusItem = table->item(row, 5);
        if (p.status.trimmed().compare("Discharged", Qt::CaseInsensitive) == 0) {
            statusItem->setBackground(QColor("#eafaf1"));
            statusItem->setForeground(QColor("#1e8449"));
        } else if (p.status.trimmed().compare("Admitted", Qt::CaseInsensitive) == 0) {
            statusItem->setBackground(QColor("#fef9e7"));
            statusItem->setForeground(QColor("#b7950b"));
        } else if (p.status.trimmed().compare("Checked In", Qt::CaseInsensitive) == 0) {
            statusItem->setBackground(QColor("#e8f0fe"));
            statusItem->setForeground(QColor("#1a4f8a"));
        }

        // Treat Button
        QPushButton *treatBtn = new QPushButton("Treat");
        treatBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #1a73e8;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 4px 12px;"
            "   font-size: 12px;"
            "   font-weight: 500;"
            "}"
            "QPushButton:hover {"
            "   background-color: #1557b0;"
            "}"
            );
        treatBtn->setProperty("patientId", p.id);
        connect(treatBtn, &QPushButton::clicked, this, [this, p]() {
            openTreatmentDialog(p.id);
        });

        table->setCellWidget(row, 6, treatBtn);

        count++;
    }

    if (patientCountLabel)
        patientCountLabel->setText(QString("Total Patients Assigned: <b>%1</b>").arg(count));
}
void doctorwindow::openTreatmentDialog(const QString &patientId)
{
    Patient p = patientMgr->searchPatient(patientId);
    if (p.id.isEmpty()) {
        QMessageBox::warning(this, "Patient Not Found", "Patient not found.");
        return;
    }

    // Create a dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Treatment - " + p.name + " (" + p.id + ")");
    dialog.setMinimumSize(550, 600);
    dialog.setModal(true);

    // Modern clean stylesheet
    dialog.setStyleSheet(
        "QDialog { background-color: #f4f6f9; }"

        "QGroupBox {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 10px;"
        "   margin-top: 12px;"
        "   padding-top: 12px;"
        "   font-weight: 600;"
        "   font-size: 13px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   left: 12px;"
        "   padding: 0 8px;"
        "   color: #1a2332;"
        "}"

        "QLabel#patientNameLabel {"
        "   font-size: 18px;"
        "   font-weight: 700;"
        "   color: #1a2332;"
        "}"
        "QLabel#patientDetailsLabel {"
        "   font-size: 13px;"
        "   color: #5a6c7d;"
        "}"
        "QLabel#currentDiagnosisLabel {"
        "   font-size: 13px;"
        "   color: #7f8c8d;"
        "   font-style: italic;"
        "   background-color: #f8f9fc;"
        "   padding: 8px 12px;"
        "   border-radius: 6px;"
        "}"
        "QLabel#statusLabel {"
        "   font-weight: 600;"
        "   color: #2c3e50;"
        "   font-size: 13px;"
        "}"

        "QTextEdit {"
        "   background-color: #ffffff;"
        "   border: 1px solid #dce1e6;"
        "   border-radius: 6px;"
        "   padding: 10px;"
        "   font-size: 13px;"
        "}"
        "QTextEdit:focus {"
        "   border-color: #1a73e8;"
        "}"

        "QComboBox {"
        "   background-color: #ffffff;"
        "   border: 1px solid #dce1e6;"
        "   border-radius: 6px;"
        "   padding: 8px 14px;"
        "   font-size: 13px;"
        "   min-width: 140px;"
        "}"
        "QComboBox:focus {"
        "   border-color: #1a73e8;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
        "QComboBox::down-arrow {"
        "   image: none;"
        "   border-left: 4px solid transparent;"
        "   border-right: 4px solid transparent;"
        "   border-top: 5px solid #5a6c7d;"
        "   margin-right: 8px;"
        "}"

        "QPushButton {"
        "   border-radius: 6px;"
        "   font-weight: 500;"
        "   padding: 10px 30px;"
        "   font-size: 14px;"
        "}"
        "QPushButton#saveBtn {"
        "   background-color: #1a73e8;"
        "   color: white;"
        "   border: none;"
        "}"
        "QPushButton#saveBtn:hover {"
        "   background-color: #1557b0;"
        "}"
        "QPushButton#cancelBtn {"
        "   background-color: #e8f0fe;"
        "   color: #1a73e8;"
        "   border: none;"
        "}"
        "QPushButton#cancelBtn:hover {"
        "   background-color: #d2e3fc;"
        "}"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(25, 25, 25, 25);

    // ===== PATIENT INFO =====
    QGroupBox *infoGroup = new QGroupBox("Patient Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setSpacing(8);
    infoLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *nameLabel = new QLabel(p.name + " (" + p.id + ")");
    nameLabel->setObjectName("patientNameLabel");
    infoLayout->addWidget(nameLabel);

    QLabel *detailsLabel = new QLabel(
        "Age: " + p.age + "   |   Gender: " + p.gender +
        "   |   Doctor: " + p.assignedDoctor
        );
    detailsLabel->setObjectName("patientDetailsLabel");
    infoLayout->addWidget(detailsLabel);

    QLabel *currentDiagnosisLabel = new QLabel(
        "Current Diagnosis: " + (p.diagnosisTreatment.isEmpty() ? "None" : p.diagnosisTreatment)
        );
    currentDiagnosisLabel->setObjectName("currentDiagnosisLabel");
    currentDiagnosisLabel->setWordWrap(true);
    infoLayout->addWidget(currentDiagnosisLabel);

    mainLayout->addWidget(infoGroup);

    // ===== DIAGNOSIS =====
    QGroupBox *diagGroup = new QGroupBox("Diagnosis");
    QVBoxLayout *diagLayout = new QVBoxLayout(diagGroup);
    diagLayout->setContentsMargins(12, 12, 12, 12);
    QTextEdit *diagnosisEdit = new QTextEdit();
    diagnosisEdit->setPlaceholderText("Enter diagnosis here...");
    diagnosisEdit->setMinimumHeight(60);
    diagLayout->addWidget(diagnosisEdit);
    mainLayout->addWidget(diagGroup);

    // ===== TREATMENT PLAN =====
    QGroupBox *treatGroup = new QGroupBox("Treatment Plan");
    QVBoxLayout *treatLayout = new QVBoxLayout(treatGroup);
    treatLayout->setContentsMargins(12, 12, 12, 12);
    QTextEdit *treatmentEdit = new QTextEdit();
    treatmentEdit->setPlaceholderText("Enter treatment plan here...");
    treatmentEdit->setMinimumHeight(60);
    treatLayout->addWidget(treatmentEdit);
    mainLayout->addWidget(treatGroup);

    // ===== PRESCRIPTION =====
    QGroupBox *presGroup = new QGroupBox("Prescription");
    QVBoxLayout *presLayout = new QVBoxLayout(presGroup);
    presLayout->setContentsMargins(12, 12, 12, 12);
    QTextEdit *prescriptionEdit = new QTextEdit();
    prescriptionEdit->setPlaceholderText("Enter prescription here...");
    prescriptionEdit->setMinimumHeight(60);
    presLayout->addWidget(prescriptionEdit);
    mainLayout->addWidget(presGroup);

    // ===== STATUS =====
    QWidget *statusWidget = new QWidget();
    QHBoxLayout *statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(0, 5, 0, 5);
    statusLayout->setSpacing(15);

    QLabel *statusLabel = new QLabel("Update Status:");
    statusLabel->setObjectName("statusLabel");

    QComboBox *statusCombo = new QComboBox();
    statusCombo->addItems({"Admitted", "Checked In", "Discharged", "Completed", "No Show"});
    statusCombo->setCurrentText(p.status.isEmpty() ? "Admitted" : p.status);
    statusCombo->setMinimumWidth(150);

    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(statusCombo);
    statusLayout->addStretch();

    mainLayout->addWidget(statusWidget);

    // ===== BUTTONS =====
    QWidget *buttonWidget = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 10, 0, 0);
    buttonLayout->setSpacing(12);

    QPushButton *cancelBtn = new QPushButton("Cancel");
    cancelBtn->setObjectName("cancelBtn");
    QPushButton *saveBtn = new QPushButton("Save Treatment");
    saveBtn->setObjectName("saveBtn");

    bool saved = false;

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, &dialog, [&]() {
        saved = true;
        dialog.accept();
    });

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(saveBtn);

    mainLayout->addWidget(buttonWidget);

    // ===== EXECUTE =====
    if (dialog.exec() == QDialog::Accepted && saved) {
        QString diagnosis = diagnosisEdit->toPlainText().trimmed();
        QString treatment = treatmentEdit->toPlainText().trimmed();
        QString prescription = prescriptionEdit->toPlainText().trimmed();
        QString status = statusCombo->currentText();

        // Build combined string
        QString combined;
        if (!diagnosis.isEmpty()) {
            combined = "Diagnosis: " + diagnosis;
        }
        if (!treatment.isEmpty()) {
            if (!combined.isEmpty()) combined += " | Treatment: " + treatment;
            else combined = "Treatment: " + treatment;
        }
        if (!prescription.isEmpty()) {
            if (!combined.isEmpty()) combined += " | Rx: " + prescription;
            else combined = "Rx: " + prescription;
        }

        // Save to CSV via PatientManager
        if (!combined.isEmpty()) {
            patientMgr->diagnose(p.id, combined);
        }

        // Update status
        if (!status.isEmpty() && status != p.status) {
            patientMgr->setStatus(p.id, status);
        }

        // Refresh the patient list
        refreshPatientList();

        QMessageBox::information(this, "Success",
                                 "Treatment saved for patient " + p.name + "\n\n" +
                                     "Status: " + status);
    }
}

void doctorwindow::refreshPatientList()
{
    // Find the patient table in the page
    QTableWidget *table = ui->pagePatient->findChild<QTableWidget *>("patientTable");

    if (table) {
        // Use the table-based approach
        refreshPatientTable(table);
    } else {
        // Fallback: use the old row-based layout
        if (!patientRowsLayout) return;

        while (patientRowsLayout->count() > 1) {
            QLayoutItem *item = patientRowsLayout->takeAt(0);
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        int count = 0;
        for (const auto &p : patientMgr->getAllPatients()) {
            if (p.assignedDoctor.trimmed().compare(currentUserName.trimmed(), Qt::CaseInsensitive) != 0)
                continue;
            addPatientRowWithTreat(p);
            count++;
        }

        if (patientCountLabel)
            patientCountLabel->setText(QString("Total Patients Assigned: <b>%1</b>").arg(count));
    }
}

void doctorwindow::addPatientRowWithTreat(const Patient &p)
{
    QFrame *row = new QFrame();
    row->setObjectName("docPatientRow");
    row->setStyleSheet(
        "QFrame#docPatientRow{background-color:#ffffff;border-bottom:1px solid #f1f5f9;}"
        "QFrame#docPatientRow:hover{background-color:#f8fafc;}");

    QHBoxLayout *rl = new QHBoxLayout(row);
    rl->setContentsMargins(16, 10, 16, 10);
    rl->setSpacing(10);

    QLabel *lblId       = new QLabel(p.id);
    QLabel *lblName     = new QLabel(p.name);
    QLabel *lblAge      = new QLabel(p.age);
    QLabel *lblGender   = new QLabel(p.gender);
    QLabel *lblProblem  = new QLabel(p.diagnosisTreatment);
    QLabel *lblStatus   = new QLabel();

    lblId->setStyleSheet("font-weight:bold;color:#64748b;" + kPlainLabelD);
    lblName->setStyleSheet("font-weight:bold;color:#1e293b;" + kPlainLabelD);
    lblAge->setStyleSheet("color:#334155;" + kPlainLabelD);
    lblGender->setStyleSheet("color:#334155;" + kPlainLabelD);
    lblProblem->setStyleSheet("color:#334155;" + kPlainLabelD);
    applyStatusBadgeD(lblStatus, p.status);

    // ===== TREAT BUTTON =====
    QPushButton *treatBtn = new QPushButton("Treat");
    treatBtn->setObjectName("treatBtn");
    treatBtn->setStyleSheet(
        "QPushButton#treatBtn {"
        "   background-color: #1a73e8;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 6px;"
        "   padding: 4px 16px;"
        "   font-size: 12px;"
        "   font-weight: 500;"
        "}"
        "QPushButton#treatBtn:hover {"
        "   background-color: #1557b0;"
        "}"
        );
    treatBtn->setFixedHeight(28);
    treatBtn->setFixedWidth(60);
    treatBtn->setCursor(Qt::PointingHandCursor);
    treatBtn->setProperty("patientId", p.id);

    connect(treatBtn, &QPushButton::clicked, this, [this, p]() {
        openTreatmentDialog(p.id);
    });

    rl->addWidget(lblId,      1);
    rl->addWidget(lblName,    2);
    rl->addWidget(lblAge,     1);
    rl->addWidget(lblGender,  1);
    rl->addWidget(lblProblem, 3);
    rl->addWidget(lblStatus,  1);
    rl->addWidget(treatBtn,   1);

    row->setLayout(rl);

    patientRowsLayout->insertWidget(patientRowsLayout->count() - 1, row);
}

void doctorwindow::addPatientRow(const Patient &p)
{
    QFrame *row = new QFrame();
    row->setObjectName("docPatientRow");
    row->setStyleSheet(
        "QFrame#docPatientRow{background-color:#ffffff;border-bottom:1px solid #f1f5f9;}"
        "QFrame#docPatientRow:hover{background-color:#f8fafc;}");

    QHBoxLayout *rl = new QHBoxLayout(row);
    rl->setContentsMargins(16, 10, 16, 10);
    rl->setSpacing(10);

    QLabel *lblId       = new QLabel(p.id);
    QLabel *lblName     = new QLabel(p.name);
    QLabel *lblAge      = new QLabel(p.age);
    QLabel *lblGender   = new QLabel(p.gender);
    QLabel *lblProblem  = new QLabel(p.diagnosisTreatment);
    QLabel *lblStatus   = new QLabel();

    lblId->setStyleSheet("font-weight:bold;color:#64748b;" + kPlainLabelD);
    lblName->setStyleSheet("font-weight:bold;color:#1e293b;" + kPlainLabelD);
    lblAge->setStyleSheet("color:#334155;" + kPlainLabelD);
    lblGender->setStyleSheet("color:#334155;" + kPlainLabelD);
    lblProblem->setStyleSheet("color:#334155;" + kPlainLabelD);
    applyStatusBadgeD(lblStatus, p.status);

    rl->addWidget(lblId,      1);
    rl->addWidget(lblName,    2);
    rl->addWidget(lblAge,     1);
    rl->addWidget(lblGender,  1);
    rl->addWidget(lblProblem, 3);
    rl->addWidget(lblStatus,  1);
    row->setLayout(rl);

    patientRowsLayout->insertWidget(patientRowsLayout->count() - 1, row);
}

// ===== SCHEDULING PAGE =====
void doctorwindow::setupSchedulingPage()
{
    QWidget *page = ui->pageScheduling;
    if (!page) return;

    if (page->layout()) {
        QLayoutItem *item;
        while ((item = page->layout()->takeAt(0))) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete page->layout();
    }

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(20, 16, 20, 16);
    pageLayout->setSpacing(12);

    QLabel *title = new QLabel("My Schedule");
    title->setStyleSheet("font-size:20px;font-weight:700;color:#0f172a;border:none;background:transparent;");
    pageLayout->addWidget(title);

    scheduleCountLabel = new QLabel();
    scheduleCountLabel->setStyleSheet("font-size:13px;color:#64748b;border:none;background:transparent;");
    pageLayout->addWidget(scheduleCountLabel);

    QWidget *colHeader = new QWidget();
    colHeader->setStyleSheet("background-color:#f8fafc;border-bottom:2px solid #e2e8f0;");
    QHBoxLayout *chLayout = new QHBoxLayout(colHeader);
    chLayout->setContentsMargins(16, 8, 16, 8);
    chLayout->setSpacing(10);
    const QString chStyle = "font-weight:700;font-size:11px;color:#94a3b8;" + kPlainLabelD;
    auto makeCol = [&](const QString &txt, int s) {
        QLabel *l = new QLabel(txt); l->setStyleSheet(chStyle);
        chLayout->addWidget(l, s);
    };
    makeCol("DATE", 2); makeCol("TIME", 1); makeCol("PATIENT", 2);
    makeCol("DEPARTMENT", 2); makeCol("REASON", 3); makeCol("STATUS", 1);
    pageLayout->addWidget(colHeader);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}");

    scheduleRowContainer = new QWidget();
    scheduleRowContainer->setStyleSheet("background:transparent;");
    scheduleRowsLayout = new QVBoxLayout(scheduleRowContainer);
    scheduleRowsLayout->setContentsMargins(0, 0, 0, 0);
    scheduleRowsLayout->setSpacing(0);
    scheduleRowsLayout->addStretch();

    scroll->setWidget(scheduleRowContainer);
    pageLayout->addWidget(scroll, 1);

    refreshSchedulingTable();
}

void doctorwindow::refreshSchedulingTable()
{
    if (!scheduleRowsLayout) return;

    while (scheduleRowsLayout->count() > 1) {
        QLayoutItem *item = scheduleRowsLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    QVector<Appointment> mine = apptMgr->getAppointmentsForDoctor(currentUserName);
    for (const auto &a : mine)
        addScheduleRow(a);

    if (scheduleCountLabel)
        scheduleCountLabel->setText(QString("Total Appointments: <b>%1</b>").arg(mine.size()));
}

void doctorwindow::addScheduleRow(const Appointment &a)
{
    QFrame *row = new QFrame();
    row->setObjectName("docScheduleRow");
    row->setStyleSheet(
        "QFrame#docScheduleRow{background-color:#ffffff;border-bottom:1px solid #f1f5f9;}"
        "QFrame#docScheduleRow:hover{background-color:#f8fafc;}");

    QHBoxLayout *rl = new QHBoxLayout(row);
    rl->setContentsMargins(16, 10, 16, 10);
    rl->setSpacing(10);

    QLabel *lblDate   = new QLabel(a.date);
    QLabel *lblTime   = new QLabel(a.time);
    QLabel *lblPat    = new QLabel(a.patientName);
    QLabel *lblDept   = new QLabel(a.department);
    QLabel *lblReason = new QLabel(a.reason);
    QLabel *lblStatus = new QLabel();

    lblDate->setStyleSheet("font-weight:bold;color:#64748b;" + kPlainLabelD);
    lblTime->setStyleSheet("color:#334155;" + kPlainLabelD);
    lblPat->setStyleSheet("font-weight:bold;color:#1e293b;" + kPlainLabelD);
    lblDept->setStyleSheet("color:#334155;" + kPlainLabelD);
    lblReason->setStyleSheet("color:#334155;" + kPlainLabelD);
    applyStatusBadgeD(lblStatus, a.status);

    rl->addWidget(lblDate,   2);
    rl->addWidget(lblTime,   1);
    rl->addWidget(lblPat,    2);
    rl->addWidget(lblDept,   2);
    rl->addWidget(lblReason, 3);
    rl->addWidget(lblStatus, 1);
    row->setLayout(rl);

    scheduleRowsLayout->insertWidget(scheduleRowsLayout->count() - 1, row);
}