#include "receptionistwindow.h"
#include "mainwindow.h"
#include "ui_receptionistwindow.h"
#include "staffmanager.h"
#include "billingmanager.h"
#include "appointmentmanager.h"

#include <QDateTime>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QList>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDialog>
#include <QDialogButtonBox>
#include <QColor>

ReceptionistWindow::ReceptionistWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ReceptionistWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("AXON-HMS: Receptionist Dashboard");

    // Enable stylesheet painting and force background
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet("ReceptionistWindow { background-color: #F8FAFC; }");

    if (ui->contentAreaWidget) {
        ui->contentAreaWidget->setAttribute(Qt::WA_StyledBackground, true);
        ui->contentAreaWidget->setStyleSheet("QWidget#contentAreaWidget { background-color: #F8FAFC; }");
    }

    if (ui->page_3 && ui->page_3->layout()) {
        ui->page_3->layout()->setAlignment(Qt::Alignment());
    }

    // Shared backend managers
    patientMgr = new PatientManager();
    staffMgr   = new StaffManager();
    billingMgr = new BillingManager();
    apptMgr    = new AppointmentManager();   // NEW: backs real appointment records

    // Setup layouts
    setupCardStyles();
    setupDashboardBottomArea();
    setupRegisterPatientPage();
    setupSchedulePage();
    setupBillingPage();

    setupConnections();

    // Initial view setup
    if (ui->widgetstackedtogether && ui->page_3) {
        ui->widgetstackedtogether->setCurrentWidget(ui->page_3);
    }
    updateSidebarSelection(ui->btnDashboard);

    populateDoctorDropdowns();
    populatePatientDropdowns();
    refreshDashboardStats();

    // Live clock
    dateTimeTimer = new QTimer(this);
    connect(dateTimeTimer, &QTimer::timeout, this, &ReceptionistWindow::updateDateTime);
    dateTimeTimer->start(1000);
    updateDateTime();
}

ReceptionistWindow::~ReceptionistWindow()
{
    delete ui;
    delete patientMgr;
    delete staffMgr;
    delete billingMgr;
    delete apptMgr;
}

void ReceptionistWindow::setupConnections()
{
    connect(ui->btnDashboard,       &QPushButton::clicked, this, &ReceptionistWindow::onDashboardClicked);
    connect(ui->btnRegisterPatient, &QPushButton::clicked, this, &ReceptionistWindow::onRegisterPatientClicked);
    connect(ui->btnSchedule,        &QPushButton::clicked, this, &ReceptionistWindow::onScheduleClicked);
    connect(ui->btnBilling,         &QPushButton::clicked, this, &ReceptionistWindow::onBillingClicked);
    connect(ui->btnMenu,            &QPushButton::clicked, this, &ReceptionistWindow::onMenuClicked);
}

void ReceptionistWindow::setupCardStyles()
{
    QString cardStyle =
        "QFrame {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #E2E8F0;"
        "   border-radius: 12px;"
        "}"
        "QFrame:hover {"
        "   border: 1px solid #38BDF8;"
        "}";

    if (ui->appointments) ui->appointments->setStyleSheet(cardStyle);
    if (ui->bedsavailable) ui->bedsavailable->setStyleSheet(cardStyle);
    if (ui->billspending) ui->billspending->setStyleSheet(cardStyle);

    QString numStyle = "color: #0F172A; font-size: 24px; font-weight: bold; border: none; background: transparent;";
    if (ui->numberofappointment) ui->numberofappointment->setStyleSheet(numStyle);
    if (ui->numberofavailablebeds) ui->numberofavailablebeds->setStyleSheet(numStyle);
    if (ui->numberofpendingbills) ui->numberofpendingbills->setStyleSheet(numStyle);

    QString titleStyle = "color: #64748B; font-size: 12px; font-weight: 600; text-transform: uppercase; border: none; background: transparent;";
    if (ui->appointmentTitle) ui->appointmentTitle->setStyleSheet(titleStyle);

    if (ui->bedsTitle) {
        ui->bedsTitle->setText("AVAILABLE DOCTORS");
        ui->bedsTitle->setStyleSheet(titleStyle);
    }

    if (ui->billingTitle) ui->billingTitle->setStyleSheet(titleStyle);

    QString statusBaseStyle = "font-size: 11px; font-weight: 600; border: none; background: transparent;";
    if (ui->appointmentStatus) ui->appointmentStatus->setStyleSheet(statusBaseStyle);
    if (ui->bedsStatus) ui->bedsStatus->setStyleSheet(statusBaseStyle);
    if (ui->billingStatus) ui->billingStatus->setStyleSheet(statusBaseStyle);

    if (ui->appointmentStatus) {
        ui->appointmentStatus->setText(
            "<span style='color: #10B981;'>• Completed</span> &nbsp;&nbsp;"
            "<span style='color: #0F172A;'>• Pending</span>"
            );
    }

    if (ui->bedsStatus) {
        ui->bedsStatus->setText(
            "<span style='color: #10B981;'>• On Duty</span> &nbsp;&nbsp;"
            "<span style='color: #64748B;'>• On Leave</span>"
            );
    }

    if (ui->billingStatus) {
        ui->billingStatus->setText(
            "<span style='color: #EF4444;'>• Unpaid</span>"
            );
    }
}

void ReceptionistWindow::setupDashboardBottomArea()
{
    if (!ui->page_3) return;

    if (!ui->page_3->layout()) {
        QVBoxLayout *p3Layout = new QVBoxLayout(ui->page_3);
        p3Layout->setContentsMargins(0, 0, 0, 0);
    }

    QWidget *bottomContainer = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomContainer);
    bottomLayout->setContentsMargins(0, 10, 0, 0);
    bottomLayout->setSpacing(20);

    // Queue Table Frame
    QFrame *tableFrame = new QFrame(this);
    tableFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(15, 15, 15, 15);

    QHBoxLayout *tableHeaderLayout = new QHBoxLayout();
    QLabel *tableTitle = new QLabel("Today's Patient Queue", tableFrame);
    tableTitle->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: bold; border: none;");

    tableHeaderLayout->addWidget(tableTitle);
    tableHeaderLayout->addStretch();
    tableLayout->addLayout(tableHeaderLayout);

    // 7 Columns: S.No., ID, Patient Name, Doctor, Time, Status, Actions
    queueTable = new QTableWidget(0, 7, tableFrame);
    queueTable->setHorizontalHeaderLabels({"S.No.", "ID", "Patient Name", "Doctor", "Time", "Status", "Actions"});

    queueTable->verticalHeader()->setVisible(false);

    QHeaderView *header = queueTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::Fixed);

    queueTable->setColumnWidth(0, 45);
    queueTable->setColumnWidth(1, 85);
    queueTable->setColumnWidth(6, 100);

    queueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    queueTable->setSelectionMode(QAbstractItemView::SingleSelection);
    queueTable->setStyleSheet(
        "QTableWidget { border: none; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QHeaderView::section { background-color: #F8FAFC; color: #475569; font-weight: bold; border: none; padding: 8px; }"
        "QTableWidget::item { padding: 6px; }"
        );
    tableLayout->addWidget(queueTable);

    // Quick Actions Frame
    QFrame *actionsFrame = new QFrame(this);
    actionsFrame->setFixedWidth(240);
    actionsFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }");
    QVBoxLayout *actionsLayout = new QVBoxLayout(actionsFrame);
    actionsLayout->setContentsMargins(15, 15, 15, 15);
    actionsLayout->setSpacing(12);

    QLabel *actionsTitle = new QLabel("Quick Actions", actionsFrame);
    actionsTitle->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: bold; border: none;");
    actionsLayout->addWidget(actionsTitle);

    QString btnStyle =
        "QPushButton {"
        "   background-color: #E0F2FE;"
        "   color: #0369A1;"
        "   border: 1px solid #BAE6FD;"
        "   border-radius: 8px;"
        "   padding: 12px;"
        "   font-weight: bold;"
        "   font-size: 13px;"
        "   text-align: left;"
        "}"
        "QPushButton:hover {"
        "   background-color: #BAE6FD;"
        "   color: #0284C7;"
        "}";

    QPushButton *btnQuickReg = new QPushButton("+ Register Patient", actionsFrame);
    QPushButton *btnQuickApp = new QPushButton("📅 Book Appointment", actionsFrame);
    QPushButton *btnQuickBill = new QPushButton("💳 Create Invoice", actionsFrame);

    btnQuickReg->setStyleSheet(btnStyle);
    btnQuickApp->setStyleSheet(btnStyle);
    btnQuickBill->setStyleSheet(btnStyle);

    connect(btnQuickReg, &QPushButton::clicked, this, &ReceptionistWindow::onRegisterPatientClicked);
    connect(btnQuickApp, &QPushButton::clicked, this, &ReceptionistWindow::onScheduleClicked);
    connect(btnQuickBill, &QPushButton::clicked, this, &ReceptionistWindow::onBillingClicked);

    actionsLayout->addWidget(btnQuickReg);
    actionsLayout->addWidget(btnQuickApp);
    actionsLayout->addWidget(btnQuickBill);
    actionsLayout->addStretch();

    bottomLayout->addWidget(tableFrame, 3);
    bottomLayout->addWidget(actionsFrame, 1);

    ui->page_3->layout()->addWidget(bottomContainer);
    if (QVBoxLayout *vbox = qobject_cast<QVBoxLayout*>(ui->page_3->layout())) {
        vbox->setStretchFactor(bottomContainer, 1);
    }
}

void ReceptionistWindow::setupRegisterPatientPage()
{
    if (!ui->page_4) return;

    if (!ui->page_4->layout()) {
        QVBoxLayout *p4Layout = new QVBoxLayout(ui->page_4);
        p4Layout->setContentsMargins(10, 10, 10, 10);
    }

    QFrame *formFrame = new QFrame(this);
    formFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }");

    QVBoxLayout *mainFormLayout = new QVBoxLayout(formFrame);
    mainFormLayout->setContentsMargins(25, 25, 25, 25);
    mainFormLayout->setSpacing(15);

    QLabel *header = new QLabel("Register New Patient", formFrame);
    header->setStyleSheet("color: #0F172A; font-size: 18px; font-weight: bold; border: none;");
    mainFormLayout->addWidget(header);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    QString inputStyle =
        "QLineEdit, QComboBox, QSpinBox {"
        "   background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px; font-size: 13px;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border: 1px solid #38BDF8; background-color: #FFFFFF; }";

    txtPatientName = new QLineEdit(formFrame);
    txtPatientName->setPlaceholderText("Enter full name");
    txtPatientName->setStyleSheet(inputStyle);

    spnPatientAge = new QSpinBox(formFrame);
    spnPatientAge->setRange(0, 120);
    spnPatientAge->setValue(25);
    spnPatientAge->setStyleSheet(inputStyle);

    cmbPatientGender = new QComboBox(formFrame);
    cmbPatientGender->addItems({"Male", "Female", "Other"});
    cmbPatientGender->setStyleSheet(inputStyle);

    txtPatientContact = new QLineEdit(formFrame);
    txtPatientContact->setPlaceholderText("Phone number");
    txtPatientContact->setStyleSheet(inputStyle);

    cmbBloodGroup = new QComboBox(formFrame);
    cmbBloodGroup->addItems({"A+", "A-", "B+", "B-", "O+", "O-", "AB+", "AB-"});
    cmbBloodGroup->setStyleSheet(inputStyle);

    txtReasonForVisit = new QLineEdit(formFrame);
    txtReasonForVisit->setPlaceholderText("e.g. Routine Checkup, Fever, General Consultation");
    txtReasonForVisit->setStyleSheet(inputStyle);

    // FIX: previously hardcoded fake doctor names (e.g. "Dr. Sharma
    // (General Medicine)") that matched no real login. That mismatch is
    // what made a doctor's "My Patients" list end up empty/wrong — the
    // Doctor window filters patients by exact assignedDoctor == currentUserName.
    // Now populated from real staff records in populateDoctorDropdowns().
    cmbAssignedDoctor = new QComboBox(formFrame);
    cmbAssignedDoctor->setStyleSheet(inputStyle);

    formLayout->addRow("Full Name:", txtPatientName);
    formLayout->addRow("Age:", spnPatientAge);
    formLayout->addRow("Gender:", cmbPatientGender);
    formLayout->addRow("Contact No:", txtPatientContact);
    formLayout->addRow("Blood Group:", cmbBloodGroup);
    formLayout->addRow("Reason for Visit:", txtReasonForVisit);
    formLayout->addRow("Assigned Doctor:", cmbAssignedDoctor);

    mainFormLayout->addLayout(formLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    QPushButton *btnSave = new QPushButton("Save Patient", formFrame);
    btnSave->setStyleSheet("QPushButton { background-color: #10B981; color: white; font-weight: bold; border-radius: 6px; padding: 10px 20px; border: none; } QPushButton:hover { background-color: #059669; }");

    QPushButton *btnClear = new QPushButton("Clear Form", formFrame);
    btnClear->setStyleSheet("QPushButton { background-color: #64748B; color: white; font-weight: bold; border-radius: 6px; padding: 10px 20px; border: none; } QPushButton:hover { background-color: #475569; }");

    connect(btnSave, &QPushButton::clicked, this, &ReceptionistWindow::onSavePatientClicked);
    connect(btnClear, &QPushButton::clicked, this, &ReceptionistWindow::onClearPatientFormClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(btnClear);
    btnLayout->addWidget(btnSave);

    mainFormLayout->addLayout(btnLayout);
    mainFormLayout->addStretch();

    ui->page_4->layout()->addWidget(formFrame);
}

// NEW: pulls real doctor names from staff_database.csv (via StaffManager)
// instead of a hardcoded fake list, so patients get assigned to a doctor
// who can actually log in and see them.
void ReceptionistWindow::populateDoctorDropdowns()
{
    if (!staffMgr) return;
    staffMgr->reload();

    QStringList doctorNames;
    for (const auto &s : staffMgr->getAllStaff()) {
        if (s.role.compare("Doctor", Qt::CaseInsensitive) == 0) {
            QString displayName = s.name.isEmpty() ? s.username : s.name;
            if (!doctorNames.contains(displayName))
                doctorNames << displayName;
        }
    }

    if (cmbAssignedDoctor) {
        QString previous = cmbAssignedDoctor->currentText();
        cmbAssignedDoctor->clear();
        cmbAssignedDoctor->addItem("Unassigned");
        cmbAssignedDoctor->addItems(doctorNames);
        int idx = cmbAssignedDoctor->findText(previous);
        if (idx >= 0) cmbAssignedDoctor->setCurrentIndex(idx);
    }

    if (cmbSchedDoctor) {
        QString previous = cmbSchedDoctor->currentText();
        cmbSchedDoctor->clear();
        cmbSchedDoctor->addItems(doctorNames);
        int idx = cmbSchedDoctor->findText(previous);
        if (idx >= 0) cmbSchedDoctor->setCurrentIndex(idx);
    }
}

void ReceptionistWindow::onSavePatientClicked()
{
    if (!txtPatientName || !txtPatientContact) return;

    QString name = txtPatientName->text().trimmed();
    QString contact = txtPatientContact->text().trimmed();
    int age = spnPatientAge ? spnPatientAge->value() : 0;
    QString gender = cmbPatientGender ? cmbPatientGender->currentText() : "Other";
    QString bloodGroup = cmbBloodGroup ? cmbBloodGroup->currentText() : "N/A";
    QString reason = txtReasonForVisit ? txtReasonForVisit->text().trimmed() : "";
    QString doctor = cmbAssignedDoctor ? cmbAssignedDoctor->currentText() : "Unassigned";

    if (name.isEmpty() || contact.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please fill in Name and Contact Number.");
        return;
    }

    Patient newPatient;

    int maxIdNum = 0;
    if (patientMgr) {
        const auto existingPatients = patientMgr->getAllPatients();
        for (const Patient &p : existingPatients) {
            QString numStr = p.id;
            numStr.remove("PT-");
            bool ok = false;
            int currentNum = numStr.toInt(&ok);
            if (ok && currentNum > maxIdNum) {
                maxIdNum = currentNum;
            }
        }
    }

    newPatient.id = QString("PT-%1").arg(maxIdNum + 1, 4, 10, QChar('0'));
    newPatient.name = name;
    newPatient.age = QString::number(age);
    newPatient.gender = gender;
    newPatient.contact = contact;
    newPatient.bloodGroup = bloodGroup;
    newPatient.diagnosisTreatment = reason.isEmpty() ? "General Checkup" : reason;
    newPatient.assignedDoctor = doctor;
    newPatient.status = "Waiting";

    if (patientMgr) {
        patientMgr->addPatient(newPatient);
    }

    QMessageBox::information(this, "Success", "Patient " + name + " registered successfully with ID " + newPatient.id + "!");

    onClearPatientFormClicked();
    populateDoctorDropdowns();
    populatePatientDropdowns();
    refreshDashboardStats();
    onDashboardClicked();
}

void ReceptionistWindow::onClearPatientFormClicked()
{
    if (txtPatientName) txtPatientName->clear();
    if (txtPatientContact) txtPatientContact->clear();
    if (spnPatientAge) spnPatientAge->setValue(25);
    if (cmbPatientGender) cmbPatientGender->setCurrentIndex(0);
    if (cmbBloodGroup) cmbBloodGroup->setCurrentIndex(0);
    if (txtReasonForVisit) txtReasonForVisit->clear();
    if (cmbAssignedDoctor) cmbAssignedDoctor->setCurrentIndex(0);
}

void ReceptionistWindow::onClearScheduleFormClicked()
{
    if (cmbSchedPatient && cmbSchedPatient->count() > 0) cmbSchedPatient->setCurrentIndex(0);
    if (cmbSchedDoctor && cmbSchedDoctor->count() > 0) cmbSchedDoctor->setCurrentIndex(0);
    if (dtSchedDate) dtSchedDate->setDate(QDate::currentDate());
    if (tmSchedTime) tmSchedTime->setTime(QTime::currentTime());
    if (txtSchedReason) txtSchedReason->clear();
}

void ReceptionistWindow::onClearBillingFormClicked()
{
    if (cmbBillPatient && cmbBillPatient->count() > 0) cmbBillPatient->setCurrentIndex(0);
    if (cmbBillType && cmbBillType->count() > 0) cmbBillType->setCurrentIndex(0);
    if (spnBillAmount) spnBillAmount->setValue(1000.0);
    if (cmbBillStatus && cmbBillStatus->count() > 0) cmbBillStatus->setCurrentIndex(0);
}

void ReceptionistWindow::setupSchedulePage()
{
    if (!ui->page_5) return;

    if (!ui->page_5->layout()) {
        QVBoxLayout *p5Layout = new QVBoxLayout(ui->page_5);
        p5Layout->setContentsMargins(0, 0, 0, 0);
    }

    QWidget *container = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(20);

    QFrame *formFrame = new QFrame(this);
    formFrame->setFixedWidth(340);
    formFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }");

    QVBoxLayout *formLayout = new QVBoxLayout(formFrame);
    formLayout->setContentsMargins(20, 20, 20, 20);
    formLayout->setSpacing(12);

    QLabel *lblTitle = new QLabel("📅 Book Appointment", formFrame);
    lblTitle->setStyleSheet("color: #0F172A; font-size: 16px; font-weight: bold; border: none;");
    formLayout->addWidget(lblTitle);

    QString inputStyle =
        "QLineEdit, QComboBox, QDateEdit, QTimeEdit {"
        "   background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px; font-size: 12px;"
        "}"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #38BDF8; background-color: #FFFFFF; }";

    cmbSchedPatient = new QComboBox(formFrame);
    cmbSchedPatient->setStyleSheet(inputStyle);

    // FIX: previously hardcoded fake doctor names — see populateDoctorDropdowns()
    cmbSchedDoctor = new QComboBox(formFrame);
    cmbSchedDoctor->setStyleSheet(inputStyle);

    dtSchedDate = new QDateEdit(QDate::currentDate(), formFrame);
    dtSchedDate->setCalendarPopup(true);
    dtSchedDate->setStyleSheet(inputStyle);

    tmSchedTime = new QTimeEdit(QTime::currentTime(), formFrame);
    tmSchedTime->setDisplayFormat("hh:mm AP");
    tmSchedTime->setStyleSheet(inputStyle);

    txtSchedReason = new QLineEdit(formFrame);
    txtSchedReason->setPlaceholderText("Consultation Reason");
    txtSchedReason->setStyleSheet(inputStyle);

    QFormLayout *inputs = new QFormLayout();
    inputs->setSpacing(10);
    inputs->addRow("Patient:", cmbSchedPatient);
    inputs->addRow("Doctor:", cmbSchedDoctor);
    inputs->addRow("Date:", dtSchedDate);
    inputs->addRow("Time:", tmSchedTime);
    inputs->addRow("Reason:", txtSchedReason);

    formLayout->addLayout(inputs);

    QPushButton *btnBook = new QPushButton("Schedule Appointment", formFrame);
    btnBook->setStyleSheet("QPushButton { background-color: #0284C7; color: white; font-weight: bold; border-radius: 6px; padding: 10px; border: none; } QPushButton:hover { background-color: #0369A1; }");
    connect(btnBook, &QPushButton::clicked, this, &ReceptionistWindow::onBookAppointmentClicked);

    formLayout->addWidget(btnBook);
    formLayout->addStretch();

    QFrame *tableFrame = new QFrame(this);
    tableFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }");

    QVBoxLayout *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *tableTitle = new QLabel("Scheduled Appointments", tableFrame);
    tableTitle->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: bold; border: none;");
    tableLayout->addWidget(tableTitle);

    // 6 Columns: Appt ID, Patient, Doctor, Date & Time, Status, Action
    scheduleTable = new QTableWidget(0, 6, tableFrame);
    scheduleTable->setHorizontalHeaderLabels({"Appt ID", "Patient Name", "Doctor", "Date & Time", "Status", "Action"});
    scheduleTable->verticalHeader()->setVisible(false);

    QHeaderView *header = scheduleTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::Fixed);

    scheduleTable->setColumnWidth(0, 80);
    scheduleTable->setColumnWidth(5, 90);

    scheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    scheduleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    scheduleTable->setStyleSheet(
        "QTableWidget { border: none; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QHeaderView::section { background-color: #F8FAFC; color: #475569; font-weight: bold; border: none; padding: 8px; }"
        );

    tableLayout->addWidget(scheduleTable);

    mainLayout->addWidget(formFrame);
    mainLayout->addWidget(tableFrame);

    ui->page_5->layout()->addWidget(container);
    populateDoctorDropdowns();
    populatePatientDropdowns();
    refreshScheduleTable();
}

// FIX: previously this ONLY mutated the Patient record (assignedDoctor +
// status) — it never wrote anything to appointment_database.csv, so bookings
// made here were invisible to Admin's Scheduling tab and to the Doctor's
// own Schedule tab, both of which read AppointmentManager. Now it creates a
// real Appointment record via AppointmentManager::addAppointment(), in
// addition to still updating the Patient's assigned doctor / status.
void ReceptionistWindow::onBookAppointmentClicked()
{
    if (!cmbSchedPatient || cmbSchedPatient->count() == 0) {
        QMessageBox::warning(this, "Notice", "No patient selected. Please register a patient first.");
        return;
    }
    if (!cmbSchedDoctor || cmbSchedDoctor->count() == 0) {
        QMessageBox::warning(this, "Notice", "No doctors available. Please add a doctor in Admin > Staff Manager first.");
        return;
    }

    QString patientData = cmbSchedPatient->currentText();
    QString patientId   = patientData.section(" - ", 0, 0).trimmed();
    QString patientName = patientData.section(" - ", 1).trimmed();
    QString doctor       = cmbSchedDoctor->currentText();
    QString reason       = txtSchedReason ? txtSchedReason->text().trimmed() : "";

    if (apptMgr) {
        Appointment appt;
        appt.id          = apptMgr->generateNextId();
        appt.patientName = patientName.isEmpty() ? patientId : patientName;
        appt.doctorName  = doctor;
        appt.department  = "General";
        appt.date        = dtSchedDate ? dtSchedDate->date().toString("yyyy-MM-dd")
                                        : QDate::currentDate().toString("yyyy-MM-dd");
        appt.time        = tmSchedTime ? tmSchedTime->time().toString("hh:mm AP")
                                        : QTime::currentTime().toString("hh:mm AP");
        appt.reason      = reason.isEmpty() ? "Consultation" : reason;
        appt.status      = "Confirmed";
        apptMgr->addAppointment(appt);
    }

    if (patientMgr) {
        Patient p = patientMgr->getPatientById(patientId);
        if (!p.id.isEmpty()) {
            p.assignedDoctor = doctor;
            p.status = "Waiting";
            patientMgr->updatePatient(p);
        }
    }

    QMessageBox::information(this, "Scheduled", "Appointment scheduled successfully for " + patientData);
    onClearScheduleFormClicked();
    refreshScheduleTable();
    refreshDashboardStats();
}

// FIX: previously rebuilt this table from the Patient list (one row per
// patient, regardless of whether an appointment was ever booked). Now reads
// real Appointment records from AppointmentManager so it matches what
// Admin and the assigned Doctor see.
void ReceptionistWindow::refreshScheduleTable()
{
    if (!scheduleTable || !apptMgr) return;
    apptMgr->reload();

    const auto appts = apptMgr->getAllAppointments();
    scheduleTable->setRowCount(0);

    for (int i = 0; i < appts.size(); ++i) {
        const Appointment &a = appts[i];

        scheduleTable->insertRow(i);
        scheduleTable->setItem(i, 0, new QTableWidgetItem(a.id));
        scheduleTable->setItem(i, 1, new QTableWidgetItem(a.patientName));
        scheduleTable->setItem(i, 2, new QTableWidgetItem(a.doctorName));
        scheduleTable->setItem(i, 3, new QTableWidgetItem(a.date + "  " + a.time));

        QTableWidgetItem *statusItem = new QTableWidgetItem(a.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(
            a.status.compare("Completed", Qt::CaseInsensitive) == 0
                ? QColor(0x059669) : QColor(0x0F172A));
        scheduleTable->setItem(i, 4, statusItem);

        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setAlignment(Qt::AlignCenter);

        bool isDone = (a.status.compare("Completed", Qt::CaseInsensitive) == 0);
        QPushButton *btnDone = new QPushButton(isDone ? "Done ✓" : "✓ Done", actionWidget);
        btnDone->setEnabled(!isDone);
        btnDone->setStyleSheet(
            isDone ?
                "QPushButton { background-color: #F1F5F9; color: #94A3B8; border: none; border-radius: 4px; padding: 4px 8px; font-size: 11px; }" :
                "QPushButton { background-color: #D1FAE5; color: #065F46; border: none; border-radius: 4px; padding: 4px 8px; font-weight: bold; font-size: 11px; }"
            );

        QString apptId = a.id;
        connect(btnDone, &QPushButton::clicked, this, [this, apptId]() {
            apptMgr->updateStatus(apptId, "Completed");
            refreshScheduleTable();
            refreshDashboardStats();
        });

        actionLayout->addWidget(btnDone);
        scheduleTable->setCellWidget(i, 5, actionWidget);
    }
}

void ReceptionistWindow::setupBillingPage()
{
    if (!ui->page_6) return;

    if (!ui->page_6->layout()) {
        QVBoxLayout *p6Layout = new QVBoxLayout(ui->page_6);
        p6Layout->setContentsMargins(0, 0, 0, 0);
    }

    QWidget *container = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(20);

    QFrame *formFrame = new QFrame(this);
    formFrame->setFixedWidth(340);
    formFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }");

    QVBoxLayout *formLayout = new QVBoxLayout(formFrame);
    formLayout->setContentsMargins(20, 20, 20, 20);
    formLayout->setSpacing(12);

    QLabel *lblTitle = new QLabel("💳 Create Patient Invoice", formFrame);
    lblTitle->setStyleSheet("color: #0F172A; font-size: 16px; font-weight: bold; border: none;");
    formLayout->addWidget(lblTitle);

    QString inputStyle =
        "QLineEdit, QComboBox, QDoubleSpinBox {"
        "   background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px; font-size: 12px;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { border: 1px solid #38BDF8; background-color: #FFFFFF; }";

    cmbBillPatient = new QComboBox(formFrame);
    cmbBillPatient->setStyleSheet(inputStyle);

    cmbBillType = new QComboBox(formFrame);
    cmbBillType->setStyleSheet(inputStyle);
    cmbBillType->addItems({"Consultation Fee", "Admission Charges", "Lab Test", "Emergency Services", "Surgery"});

    spnBillAmount = new QDoubleSpinBox(formFrame);
    spnBillAmount->setRange(0.0, 500000.0);
    spnBillAmount->setValue(1000.0);
    spnBillAmount->setPrefix("Rs. ");
    spnBillAmount->setStyleSheet(inputStyle);

    cmbBillStatus = new QComboBox(formFrame);
    cmbBillStatus->setStyleSheet(inputStyle);
    cmbBillStatus->addItems({"Unpaid", "Paid"});

    QFormLayout *inputs = new QFormLayout();
    inputs->setSpacing(10);
    inputs->addRow("Patient:", cmbBillPatient);
    inputs->addRow("Charge Type:", cmbBillType);
    inputs->addRow("Amount:", spnBillAmount);
    inputs->addRow("Status:", cmbBillStatus);

    formLayout->addLayout(inputs);

    QPushButton *btnCreateBill = new QPushButton("Generate Invoice", formFrame);
    btnCreateBill->setStyleSheet("QPushButton { background-color: #10B981; color: white; font-weight: bold; border-radius: 6px; padding: 10px; border: none; } QPushButton:hover { background-color: #059669; }");
    connect(btnCreateBill, &QPushButton::clicked, this, &ReceptionistWindow::onCreateInvoiceClicked);

    formLayout->addWidget(btnCreateBill);
    formLayout->addStretch();

    QFrame *tableFrame = new QFrame(this);
    tableFrame->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }");

    QVBoxLayout *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *tableTitle = new QLabel("Billing & Invoices Record", tableFrame);
    tableTitle->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: bold; border: none;");
    tableLayout->addWidget(tableTitle);

    billingTable = new QTableWidget(0, 6, tableFrame);
    billingTable->setHorizontalHeaderLabels({"Bill ID", "Patient ID", "Description", "Amount", "Status", "Action"});
    billingTable->verticalHeader()->setVisible(false);

    QHeaderView *header = billingTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::Fixed);

    billingTable->setColumnWidth(0, 80);
    billingTable->setColumnWidth(1, 85);
    billingTable->setColumnWidth(5, 90);

    billingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    billingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    billingTable->setStyleSheet(
        "QTableWidget { border: none; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QHeaderView::section { background-color: #F8FAFC; color: #475569; font-weight: bold; border: none; padding: 8px; }"
        );

    tableLayout->addWidget(billingTable);

    mainLayout->addWidget(formFrame);
    mainLayout->addWidget(tableFrame);

    ui->page_6->layout()->addWidget(container);
    refreshBillingTable();
}

void ReceptionistWindow::onCreateInvoiceClicked()
{
    if (!cmbBillPatient || cmbBillPatient->count() == 0) {
        QMessageBox::warning(this, "Notice", "Please select a patient.");
        return;
    }

    QString patientData = cmbBillPatient->currentText();
    QString patientId = patientData.section(" - ", 0, 0).trimmed();
    double amount = spnBillAmount ? spnBillAmount->value() : 0.0;
    QString chargeType = cmbBillType ? cmbBillType->currentText() : "Medical Charge";
    bool isPaid = (cmbBillStatus && cmbBillStatus->currentText() == "Paid");

    if (billingMgr) {
        BillItem item;
        item.serviceCode = "SVC";
        item.description = chargeType;
        item.amount = amount;

        QVector<BillItem> items = { item };

        QString billId = billingMgr->generateBill(patientId, items, 0.0, 0.0, "Generated via Receptionist Dashboard");

        if (isPaid && !billId.isEmpty()) {
            billingMgr->processPayment(billId, amount, "Cash", "Full payment collected upon billing.");
        }

        QMessageBox::information(this, "Success", QString("Invoice %1 generated successfully for patient %2!").arg(billId, patientId));
    }

    refreshBillingTable();
    refreshDashboardStats();
}

void ReceptionistWindow::refreshBillingTable()
{
    if (!billingTable || !billingMgr) return;
    billingMgr->reload();

    const auto bills = billingMgr->getAllBills();
    billingTable->setRowCount(0);

    for (int i = 0; i < bills.size(); ++i) {
        const BillingRecord &b = bills[i];

        billingTable->insertRow(i);
        billingTable->setItem(i, 0, new QTableWidgetItem(b.billId));
        billingTable->setItem(i, 1, new QTableWidgetItem(b.patientId));

        QString desc = b.items.isEmpty() ? "General Services" : b.items.first().description;
        if (b.items.size() > 1) {
            desc += QString(" (+%1 more)").arg(b.items.size() - 1);
        }
        billingTable->setItem(i, 2, new QTableWidgetItem(desc));

        billingTable->setItem(i, 3, new QTableWidgetItem(QString("Rs. %1").arg(b.subtotal, 0, 'f', 2)));

        bool isPaid = (b.remainingBalance <= 0.001);
        QTableWidgetItem *statusItem = new QTableWidgetItem(isPaid ? "Paid" : "Unpaid");
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(isPaid ? QColor(0x10B981) : QColor(0xEF4444));
        billingTable->setItem(i, 4, statusItem);

        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setAlignment(Qt::AlignCenter);

        QPushButton *btnPay = new QPushButton(isPaid ? "Paid ✓" : "Pay Now", actionWidget);
        btnPay->setEnabled(!isPaid);
        btnPay->setStyleSheet(
            isPaid ?
                "QPushButton { background-color: #F1F5F9; color: #94A3B8; border: none; border-radius: 4px; padding: 4px 8px; font-size: 11px; }" :
                "QPushButton { background-color: #FEF3C7; color: #D97706; border: 1px solid #FCD34D; border-radius: 4px; padding: 4px 8px; font-weight: bold; font-size: 11px; }"
            );

        QString targetBillId = b.billId;
        double remBalance = b.remainingBalance;

        connect(btnPay, &QPushButton::clicked, this, [this, targetBillId, remBalance]() {
            if (billingMgr) {
                billingMgr->processPayment(targetBillId, remBalance, "Cash", "Cleared via Receptionist Table");
                refreshBillingTable();
                refreshDashboardStats();
            }
        });

        actionLayout->addWidget(btnPay);
        billingTable->setCellWidget(i, 5, actionWidget);
    }
}

void ReceptionistWindow::onDashboardClicked()
{
    if (ui->widgetstackedtogether && ui->page_3) {
        ui->widgetstackedtogether->setCurrentWidget(ui->page_3);
    }
    updateSidebarSelection(ui->btnDashboard);
    refreshDashboardStats();
}

void ReceptionistWindow::onRegisterPatientClicked()
{
    populateDoctorDropdowns();
    if (ui->widgetstackedtogether && ui->page_4) {
        ui->widgetstackedtogether->setCurrentWidget(ui->page_4);
    }
    updateSidebarSelection(ui->btnRegisterPatient);
}

void ReceptionistWindow::onScheduleClicked()
{
    populateDoctorDropdowns();
    if (ui->widgetstackedtogether && ui->page_5) {
        ui->widgetstackedtogether->setCurrentWidget(ui->page_5);
    }
    updateSidebarSelection(ui->btnSchedule);
    populatePatientDropdowns();
    refreshScheduleTable();
}

void ReceptionistWindow::onBillingClicked()
{
    if (ui->widgetstackedtogether && ui->page_6) {
        ui->widgetstackedtogether->setCurrentWidget(ui->page_6);
    }
    updateSidebarSelection(ui->btnBilling);
    populatePatientDropdowns();
    refreshBillingTable();
}

void ReceptionistWindow::onMenuClicked()
{
    QMenu menu(this);
    QAction *actLogout = menu.addAction("Logout");

    connect(actLogout, &QAction::triggered, this, [this]() {
        MainWindow *mw = new MainWindow();
        mw->show();
        this->close();
    });

    if (ui->btnMenu) {
        menu.exec(ui->btnMenu->mapToGlobal(QPoint(0, ui->btnMenu->height())));
    }
}

void ReceptionistWindow::updateSidebarSelection(QPushButton *activeBtn)
{
    const QList<QPushButton*> buttons = {ui->btnDashboard, ui->btnRegisterPatient, ui->btnSchedule, ui->btnBilling};

    QString activeStyle = "QPushButton { background-color: #38BDF8; color: #FFFFFF; font-weight: bold; border-radius: 8px; text-align: left; padding-left: 15px; }";
    QString normalStyle = "QPushButton { background-color: transparent; color: #64748B; font-weight: 500; border-radius: 8px; text-align: left; padding-left: 15px; } QPushButton:hover { background-color: #E2E8F0; }";

    for (QPushButton *btn : buttons) {
        if (btn) {
            btn->setStyleSheet(btn == activeBtn ? activeStyle : normalStyle);
        }
    }
}

void ReceptionistWindow::updateDateTime()
{
    QLabel *lblClock = this->findChild<QLabel*>("lbldatetime");
    if (!lblClock) lblClock = this->findChild<QLabel*>("lblDateTime");
    if (!lblClock) lblClock = this->findChild<QLabel*>("dateTimeLabel");

    if (lblClock) {
        lblClock->setText(QDateTime::currentDateTime().toString("dddd, MMMM d, yyyy | hh:mm:ss AP"));
    }

    if (ui->dateLevel) {
        ui->dateLevel->setText(QDate::currentDate().toString("ddd MMM d, yyyy"));
    }
}

void ReceptionistWindow::populatePatientDropdowns()
{
    if (!patientMgr) return;
    patientMgr->reload();

    const auto patients = patientMgr->getAllPatients();

    if (cmbSchedPatient) {
        QString previous = cmbSchedPatient->currentText();
        cmbSchedPatient->clear();
        for (const Patient &p : patients) {
            cmbSchedPatient->addItem(QString("%1 - %2").arg(p.id, p.name));
        }
        int idx = cmbSchedPatient->findText(previous);
        if (idx >= 0) cmbSchedPatient->setCurrentIndex(idx);
    }

    if (cmbBillPatient) {
        QString previous = cmbBillPatient->currentText();
        cmbBillPatient->clear();
        for (const Patient &p : patients) {
            cmbBillPatient->addItem(QString("%1 - %2").arg(p.id, p.name));
        }
        int idx = cmbBillPatient->findText(previous);
        if (idx >= 0) cmbBillPatient->setCurrentIndex(idx);
    }
}

// FIX: the appointment count (Completed / Total) now comes from real
// AppointmentManager records instead of "one row per patient regardless of
// whether they were ever scheduled".
void ReceptionistWindow::refreshDashboardStats()
{
    if (!patientMgr) return;
    patientMgr->reload();

    const auto patients = patientMgr->getAllPatients();

    if (apptMgr) apptMgr->reload();
    const auto appts = apptMgr ? apptMgr->getAllAppointments() : QVector<Appointment>();

    int totalAppointments = appts.size();
    int completedAppointments = 0;
    for (const auto &a : appts) {
        if (a.status.compare("Completed", Qt::CaseInsensitive) == 0)
            completedAppointments++;
    }

    // 1. Update Total Appointments (Completed / Total)
    if (ui->numberofappointment) {
        ui->numberofappointment->setText(QString("%1 / %2").arg(completedAppointments).arg(totalAppointments));
    }

    // 2. Dynamically Count On-Duty vs On-Leave Doctors
    int onDutyDoctors = 0;
    int onLeaveDoctors = 0;

    if (staffMgr) {
        staffMgr->reload();
        const auto allStaff = staffMgr->getAllStaff();
        for (const auto &s : allStaff) {
            if (s.role.compare("Doctor", Qt::CaseInsensitive) == 0) {
                if (s.status.compare("On Leave", Qt::CaseInsensitive) == 0) {
                    onLeaveDoctors++;
                } else {
                    onDutyDoctors++;
                }
            }
        }
    }

    if (ui->numberofavailablebeds) {
        ui->numberofavailablebeds->setText(QString("%1 / %2").arg(onDutyDoctors).arg(onLeaveDoctors));
    }

    // 3. Update Pending Bills
    double pendingAmount = 0.0;
    if (billingMgr) {
        billingMgr->reload();
        const auto bills = billingMgr->getAllBills();
        for (const BillingRecord &b : bills) {
            if (b.remainingBalance > 0.0) {
                pendingAmount += b.remainingBalance;
            }
        }
    }

    if (ui->numberofpendingbills) {
        ui->numberofpendingbills->setText(QString("Rs. %1").arg(pendingAmount, 0, 'f', 2));
    }

    // Populate Queue Table
    if (queueTable) {
        queueTable->setRowCount(0);
        int row = 0;
        for (int i = 0; i < patients.size(); ++i) {
            const Patient &p = patients[i];

            queueTable->insertRow(row);
            queueTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
            queueTable->setItem(row, 1, new QTableWidgetItem(p.id));
            queueTable->setItem(row, 2, new QTableWidgetItem(p.name));
            queueTable->setItem(row, 3, new QTableWidgetItem(p.assignedDoctor.isEmpty() ? "Unassigned" : p.assignedDoctor));
            queueTable->setItem(row, 4, new QTableWidgetItem(QTime::currentTime().toString("hh:mm AP")));

            QTableWidgetItem *statusItem = new QTableWidgetItem(p.status.isEmpty() ? "Waiting" : p.status);
            statusItem->setTextAlignment(Qt::AlignCenter);
            statusItem->setForeground(QColor(0x0F172A));
            queueTable->setItem(row, 5, statusItem);

            QWidget *actionWidget = new QWidget();
            QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
            actionLayout->setContentsMargins(0, 0, 0, 0);
            actionLayout->setAlignment(Qt::AlignCenter);

            QPushButton *btnEdit = new QPushButton("Edit", actionWidget);
            btnEdit->setStyleSheet(
                "QPushButton {"
                "   background-color: #E0F2FE;"
                "   color: #0284C7;"
                "   border: 1px solid #BAE6FD;"
                "   border-radius: 4px;"
                "   padding: 4px 12px;"
                "   font-weight: bold;"
                "   font-size: 11px;"
                "}"
                "QPushButton:hover { background-color: #BAE6FD; }"
                );

            QString pid = p.id;

            connect(btnEdit, &QPushButton::clicked, this, [this, pid]() {
                Patient p = patientMgr->getPatientById(pid);
                if (p.id.isEmpty()) return;

                QDialog dlg(this);
                dlg.setWindowTitle("Edit Patient - " + p.id);
                dlg.setFixedWidth(360);

                // Explicit stylesheet for dialog UI
                dlg.setStyleSheet(
                    "QDialog { background-color: #FFFFFF; }"
                    "QLabel { color: #1E293B; font-weight: 600; font-size: 13px; }"
                    "QComboBox {"
                    "   background-color: #F8FAFC;"
                    "   color: #0F172A;"
                    "   border: 1px solid #CBD5E1;"
                    "   border-radius: 6px;"
                    "   padding: 6px 10px;"
                    "   font-size: 12px;"
                    "}"
                    "QComboBox:focus { border: 1px solid #38BDF8; background-color: #FFFFFF; }"
                    "QComboBox QAbstractItemView {"
                    "   background-color: #FFFFFF;"
                    "   color: #0F172A;"
                    "   selection-background-color: #E0F2FE;"
                    "   selection-color: #0284C7;"
                    "}"
                    "QPushButton {"
                    "   background-color: #F1F5F9;"
                    "   color: #334155;"
                    "   border: 1px solid #CBD5E1;"
                    "   border-radius: 6px;"
                    "   padding: 6px 14px;"
                    "   font-weight: bold;"
                    "}"
                    "QPushButton:hover { background-color: #E2E8F0; }"
                    );

                QVBoxLayout mainLayout(&dlg);
                mainLayout.setSpacing(16);
                mainLayout.setContentsMargins(20, 20, 20, 20);

                QFormLayout form;
                form.setSpacing(12);

                // FIX: was a hardcoded fake doctor list — now real staff.
                QComboBox cmbDoc(&dlg);
                if (staffMgr) {
                    staffMgr->reload();
                    for (const auto &s : staffMgr->getAllStaff()) {
                        if (s.role.compare("Doctor", Qt::CaseInsensitive) == 0) {
                            QString dn = s.name.isEmpty() ? s.username : s.name;
                            if (cmbDoc.findText(dn) < 0) cmbDoc.addItem(dn);
                        }
                    }
                }
                cmbDoc.setCurrentText(p.assignedDoctor);

                QComboBox cmbStat(&dlg);
                cmbStat.addItems({"Waiting", "In Progress", "Admitted", "Completed", "Discharged"});
                cmbStat.setCurrentText(p.status);

                form.addRow("Assigned Doctor:", &cmbDoc);
                form.addRow("Status:", &cmbStat);
                mainLayout.addLayout(&form);

                // Bottom action buttons
                QHBoxLayout bottomBtnLayout;

                QPushButton btnRemove("Remove Patient", &dlg);
                btnRemove.setStyleSheet(
                    "QPushButton {"
                    "   background-color: #FEE2E2;"
                    "   color: #DC2626;"
                    "   border: 1px solid #FCA5A5;"
                    "   border-radius: 6px;"
                    "   padding: 6px 12px;"
                    "   font-weight: bold;"
                    "   font-size: 11px;"
                    "}"
                    "QPushButton:hover { background-color: #FCA5A5; }"
                    );

                QDialogButtonBox btns(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);

                if (QPushButton *okBtn = btns.button(QDialogButtonBox::Ok)) {
                    okBtn->setStyleSheet(
                        "QPushButton {"
                        "   background-color: #0284C7;"
                        "   color: #FFFFFF;"
                        "   border: none;"
                        "   border-radius: 6px;"
                        "   padding: 6px 14px;"
                        "   font-weight: bold;"
                        "}"
                        "QPushButton:hover { background-color: #0369A1; }"
                        );
                }

                bottomBtnLayout.addWidget(&btnRemove);
                bottomBtnLayout.addStretch();
                bottomBtnLayout.addWidget(&btns);

                mainLayout.addLayout(&bottomBtnLayout);

                // Handle Remove action
                connect(&btnRemove, &QPushButton::clicked, this, [&dlg, this, pid]() {
                    QMessageBox::StandardButton reply = QMessageBox::question(
                        &dlg, "Remove Patient", "Are you sure you want to remove patient " + pid + "?",
                        QMessageBox::Yes | QMessageBox::No
                        );

                    if (reply == QMessageBox::Yes) {
                        patientMgr->removePatient(pid);
                        dlg.reject();
                        populatePatientDropdowns();
                        refreshDashboardStats();
                        refreshScheduleTable();
                    }
                });

                connect(&btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                connect(&btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

                if (dlg.exec() == QDialog::Accepted) {
                    p.assignedDoctor = cmbDoc.currentText();
                    p.status = cmbStat.currentText();
                    patientMgr->updatePatient(p);
                    refreshDashboardStats();
                    refreshScheduleTable();
                }
            });

            actionLayout->addWidget(btnEdit);
            queueTable->setCellWidget(row, 6, actionWidget);
            row++;
        }
    }
}
