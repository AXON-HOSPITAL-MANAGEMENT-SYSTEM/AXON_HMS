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
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
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
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>

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

// REWRITTEN for the new Billing page layout (Patient Lookup + Current Bill
// Summary + Payment Details). See onSearchBillingPatientClicked() below for
// what "clearing" the billing form now means.
void ReceptionistWindow::onClearBillingFormClicked()
{
    if (txtBillSearch) txtBillSearch->clear();
    if (lblBillPatientId) lblBillPatientId->setText("Patient ID: —");
    if (lblBillPatientName) lblBillPatientName->setText("Name: —");
    if (lblBillPatientAge) lblBillPatientAge->setText("Age: —");
    if (lblBillPatientGender) lblBillPatientGender->setText("Gender: —");

    if (tblCurrentBillSummary) tblCurrentBillSummary->setRowCount(0);

    if (radCash) radCash->setChecked(true);
    if (spnAmountToPay) spnAmountToPay->setValue(0.0);
    if (spnDiscount) spnDiscount->setValue(0.0);
    if (cmbInsuranceProvider) cmbInsuranceProvider->setCurrentIndex(0);
    if (txtPolicyMemberId) txtPolicyMemberId->clear();
    if (txtPreAuthCode) txtPreAuthCode->clear();
    if (spnCoveredAmount) spnCoveredAmount->setValue(0.0);
    if (txtBillNotes) txtBillNotes->clear();

    // NEW: reset payment-mode-specific stacked-widget fields
    if (spnCashReceived) spnCashReceived->setValue(0.0);
    if (lblChangeDue) lblChangeDue->setText("Change Due: Rs. 0.00");
    if (cmbCardType) cmbCardType->setCurrentIndex(0);
    if (txtCardLast4) txtCardLast4->clear();
    if (txtCardAuthCode) txtCardAuthCode->clear();
    if (cmbOnlineMethod) cmbOnlineMethod->setCurrentIndex(0);
    if (txtOnlineTxnId) txtOnlineTxnId->clear();
    if (txtOnlinePayerRef) txtOnlinePayerRef->clear();

    currentLookupPatientId.clear();
    currentGeneratedBillId.clear();

    onPaymentModeChanged();

    if (btnGenerateBill) btnGenerateBill->setEnabled(false);
    if (btnProcessPayment) btnProcessPayment->setEnabled(false);
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

// ======================================================================
// BILLING PAGE — REWRITTEN to match the new mockup:
//   Row 1: [Patient Lookup / Information]  [Current Bill Summary]
//   Row 2: [Payment Details]  (payment mode, amount, discount, and a
//           QStackedWidget with mode-specific fields for
//           Cash / Card / Online / Insurance, plus notes and the
//           Generate Bill / Process Payment buttons)
// ======================================================================
void ReceptionistWindow::setupBillingPage()
{
    if (!ui->page_6) return;

    if (!ui->page_6->layout()) {
        QVBoxLayout *p6Layout = new QVBoxLayout(ui->page_6);
        p6Layout->setContentsMargins(10, 10, 10, 10);
    }

    QString inputStyle =
        "QLineEdit, QComboBox, QDoubleSpinBox, QTextEdit {"
        "   background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px; font-size: 12px;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QTextEdit:focus { border: 1px solid #38BDF8; background-color: #FFFFFF; }";

    QString sectionTitleStyle = "color: #0F172A; font-size: 16px; font-weight: bold; border: none;";
    QString frameStyle = "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }";
    QString fieldLabelStyle = "color: #334155; font-size: 12px; font-weight: 600; border: none;";

    // FIX: previously this built a "naked" QVBoxLayout and added it to
    // page_6's layout via layout()->addItem(pageLayout). addItem() does
    // NOT reparent the widgets inside a nested layout to page_6 - only
    // addLayout()/addWidget() do that. As a result every billing widget
    // stayed parented directly to ReceptionistWindow itself and got drawn
    // as a floating layer across the WHOLE window, covering the sidebar
    // (logo + nav buttons) and the top bar (clock, receptionist name).
    // Building everything inside a real container QWidget and adding that
    // with addWidget() keeps the page properly confined inside page_6,
    // same as every other page in this file.
    QWidget *billingContainer = new QWidget(ui->page_6);
    QVBoxLayout *pageLayout = new QVBoxLayout(billingContainer);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(16);

    // ---------- Row 1: Patient Lookup + Current Bill Summary ----------
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(16);

    // -- Patient Lookup / Information --
    QFrame *lookupFrame = new QFrame(this);
    lookupFrame->setStyleSheet(frameStyle);
    QVBoxLayout *lookupLayout = new QVBoxLayout(lookupFrame);
    lookupLayout->setContentsMargins(18, 18, 18, 18);
    lookupLayout->setSpacing(10);

    QLabel *lblLookupTitle = new QLabel("Patient Lookup / Information", lookupFrame);
    lblLookupTitle->setStyleSheet(sectionTitleStyle);
    lookupLayout->addWidget(lblLookupTitle);

    QHBoxLayout *searchRow = new QHBoxLayout();
    txtBillSearch = new QLineEdit(lookupFrame);
    txtBillSearch->setPlaceholderText("Patient Name (or ID)");
    txtBillSearch->setStyleSheet(inputStyle);

    btnBillSearch = new QPushButton("Search", lookupFrame);
    btnBillSearch->setStyleSheet(
        "QPushButton { background-color: #0284C7; color: white; font-weight: bold; border-radius: 6px; padding: 8px 18px; border: none; } "
        "QPushButton:hover { background-color: #0369A1; }"
        );
    connect(btnBillSearch, &QPushButton::clicked, this, &ReceptionistWindow::onSearchBillingPatientClicked);
    connect(txtBillSearch, &QLineEdit::returnPressed, this, &ReceptionistWindow::onSearchBillingPatientClicked);

    searchRow->addWidget(txtBillSearch);
    searchRow->addWidget(btnBillSearch);
    lookupLayout->addLayout(searchRow);

    QFrame *infoBox = new QFrame(lookupFrame);
    infoBox->setStyleSheet("QFrame { background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 8px; }");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoBox);
    infoLayout->setContentsMargins(14, 12, 14, 12);
    infoLayout->setSpacing(4);

    QString infoLblStyle = "color: #0F172A; font-size: 13px; border: none; background: transparent;";

    lblBillPatientId = new QLabel("Patient ID: —", infoBox);
    lblBillPatientId->setStyleSheet("color: #0F172A; font-size: 13px; font-weight: bold; border: none; background: transparent;");
    lblBillPatientName = new QLabel("Name: —", infoBox);
    lblBillPatientName->setStyleSheet(infoLblStyle);
    lblBillPatientAge = new QLabel("Age: —", infoBox);
    lblBillPatientAge->setStyleSheet(infoLblStyle);
    lblBillPatientGender = new QLabel("Gender: —", infoBox);
    lblBillPatientGender->setStyleSheet(infoLblStyle);

    infoLayout->addWidget(lblBillPatientId);
    infoLayout->addWidget(lblBillPatientName);
    infoLayout->addWidget(lblBillPatientAge);
    infoLayout->addWidget(lblBillPatientGender);

    lookupLayout->addWidget(infoBox);
    lookupLayout->addStretch();

    // -- Current Bill Summary --
    QFrame *summaryFrame = new QFrame(this);
    summaryFrame->setStyleSheet(frameStyle);
    QVBoxLayout *summaryLayout = new QVBoxLayout(summaryFrame);
    summaryLayout->setContentsMargins(18, 18, 18, 18);
    summaryLayout->setSpacing(10);

    QLabel *lblSummaryTitle = new QLabel("Current Bill Summary", summaryFrame);
    lblSummaryTitle->setStyleSheet(sectionTitleStyle);
    summaryLayout->addWidget(lblSummaryTitle);

    tblCurrentBillSummary = new QTableWidget(0, 3, summaryFrame);
    tblCurrentBillSummary->setHorizontalHeaderLabels({"Service", "Description", "Amount($)"});
    tblCurrentBillSummary->verticalHeader()->setVisible(false);
    tblCurrentBillSummary->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblCurrentBillSummary->setSelectionBehavior(QAbstractItemView::SelectRows);
    QHeaderView *sumHeader = tblCurrentBillSummary->horizontalHeader();
    sumHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    sumHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    sumHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tblCurrentBillSummary->setStyleSheet(
        "QTableWidget { border: none; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QHeaderView::section { background-color: #F8FAFC; color: #475569; font-weight: bold; border: none; padding: 8px; }"
        );
    summaryLayout->addWidget(tblCurrentBillSummary);

    topRow->addWidget(lookupFrame, 1);
    topRow->addWidget(summaryFrame, 1);
    pageLayout->addLayout(topRow);

    // ---------- Row 2: Payment Details ----------
    QFrame *paymentFrame = new QFrame(this);
    paymentFrame->setStyleSheet(frameStyle);
    QVBoxLayout *paymentLayout = new QVBoxLayout(paymentFrame);
    paymentLayout->setContentsMargins(18, 18, 18, 18);
    paymentLayout->setSpacing(12);

    QLabel *lblPaymentTitle = new QLabel("Payment Details", paymentFrame);
    lblPaymentTitle->setStyleSheet(sectionTitleStyle);
    paymentLayout->addWidget(lblPaymentTitle);

    // Payment Mode row
    QHBoxLayout *modeRow = new QHBoxLayout();
    QLabel *lblMode = new QLabel("Payment Mode:", paymentFrame);
    lblMode->setStyleSheet("color: #334155; font-size: 13px; font-weight: 600; border: none;");

    radCash      = new QRadioButton("Cash", paymentFrame);
    radCard      = new QRadioButton("Card", paymentFrame);
    radOnline    = new QRadioButton("Online", paymentFrame);
    radInsurance = new QRadioButton("Insurance", paymentFrame);
    radCash->setChecked(true);

    paymentModeGroup = new QButtonGroup(this);
    paymentModeGroup->addButton(radCash);
    paymentModeGroup->addButton(radCard);
    paymentModeGroup->addButton(radOnline);
    paymentModeGroup->addButton(radInsurance);

    connect(radCash,      &QRadioButton::toggled, this, &ReceptionistWindow::onPaymentModeChanged);
    connect(radCard,      &QRadioButton::toggled, this, &ReceptionistWindow::onPaymentModeChanged);
    connect(radOnline,    &QRadioButton::toggled, this, &ReceptionistWindow::onPaymentModeChanged);
    connect(radInsurance, &QRadioButton::toggled, this, &ReceptionistWindow::onPaymentModeChanged);

    modeRow->addWidget(lblMode);
    modeRow->addWidget(radCash);
    modeRow->addWidget(radCard);
    modeRow->addWidget(radOnline);
    modeRow->addWidget(radInsurance);
    modeRow->addStretch();
    paymentLayout->addLayout(modeRow);

    // Amount to Pay / Discount
    QGridLayout *amountGrid = new QGridLayout();
    amountGrid->setHorizontalSpacing(20);
    amountGrid->setVerticalSpacing(4);

    QLabel *lblAmount = new QLabel("Amount to Pay", paymentFrame);
    lblAmount->setStyleSheet("color: #334155; font-size: 12px; font-weight: 600; border: none;");
    QLabel *lblDiscount = new QLabel("Discount", paymentFrame);
    lblDiscount->setStyleSheet("color: #334155; font-size: 12px; font-weight: 600; border: none;");

    spnAmountToPay = new QDoubleSpinBox(paymentFrame);
    spnAmountToPay->setRange(0.0, 500000.0);
    spnAmountToPay->setPrefix("Rs. ");
    spnAmountToPay->setStyleSheet(inputStyle);

    spnDiscount = new QDoubleSpinBox(paymentFrame);
    spnDiscount->setRange(0.0, 500000.0);
    spnDiscount->setPrefix("Rs. ");
    spnDiscount->setStyleSheet(inputStyle);

    amountGrid->addWidget(lblAmount, 0, 0);
    amountGrid->addWidget(lblDiscount, 0, 1);
    amountGrid->addWidget(spnAmountToPay, 1, 0);
    amountGrid->addWidget(spnDiscount, 1, 1);
    paymentLayout->addLayout(amountGrid);

    // NEW: recalc change-due whenever Amount to Pay changes (cash page)
    connect(spnAmountToPay, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ReceptionistWindow::updateChangeDue);

    // ---------- Stacked payment-mode-specific details ----------
    stackedPaymentDetails = new QStackedWidget(paymentFrame);

    // -- Cash page --
    pageCash = new QWidget();
    QFormLayout *cashForm = new QFormLayout(pageCash);
    cashForm->setSpacing(10);
    cashForm->setContentsMargins(0, 0, 0, 0);

    QLabel *lblCashReceived = new QLabel("Cash Received", pageCash);
    lblCashReceived->setStyleSheet(fieldLabelStyle);

    spnCashReceived = new QDoubleSpinBox(pageCash);
    spnCashReceived->setRange(0.0, 500000.0);
    spnCashReceived->setPrefix("Rs. ");
    spnCashReceived->setStyleSheet(inputStyle);
    connect(spnCashReceived, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ReceptionistWindow::updateChangeDue);

    lblChangeDue = new QLabel("Change Due: Rs. 0.00", pageCash);
    lblChangeDue->setStyleSheet("color: #059669; font-size: 13px; font-weight: bold; border: none;");

    cashForm->addRow(lblCashReceived, spnCashReceived);
    cashForm->addRow(new QLabel("", pageCash), lblChangeDue);

    // -- Card page --
    pageCard = new QWidget();
    QGridLayout *cardGrid = new QGridLayout(pageCard);
    cardGrid->setContentsMargins(0, 0, 0, 0);
    cardGrid->setHorizontalSpacing(20);
    cardGrid->setVerticalSpacing(4);

    QLabel *lblCardType = new QLabel("Card Type", pageCard);
    lblCardType->setStyleSheet(fieldLabelStyle);
    QLabel *lblCardLast4 = new QLabel("Last 4 Digits", pageCard);
    lblCardLast4->setStyleSheet(fieldLabelStyle);

    cmbCardType = new QComboBox(pageCard);
    cmbCardType->addItems({"Debit", "Credit"});
    cmbCardType->setStyleSheet(inputStyle);

    txtCardLast4 = new QLineEdit(pageCard);
    txtCardLast4->setPlaceholderText("XXXX");
    txtCardLast4->setMaxLength(4);
    txtCardLast4->setStyleSheet(inputStyle);

    QLabel *lblCardAuth = new QLabel("Authorization / Approval Code", pageCard);
    lblCardAuth->setStyleSheet(fieldLabelStyle);
    txtCardAuthCode = new QLineEdit(pageCard);
    txtCardAuthCode->setStyleSheet(inputStyle);

    cardGrid->addWidget(lblCardType, 0, 0);
    cardGrid->addWidget(lblCardLast4, 0, 1);
    cardGrid->addWidget(cmbCardType, 1, 0);
    cardGrid->addWidget(txtCardLast4, 1, 1);
    cardGrid->addWidget(lblCardAuth, 2, 0, 1, 2);
    cardGrid->addWidget(txtCardAuthCode, 3, 0, 1, 2);

    // -- Online page --
    pageOnline = new QWidget();
    QGridLayout *onlineGrid = new QGridLayout(pageOnline);
    onlineGrid->setContentsMargins(0, 0, 0, 0);
    onlineGrid->setHorizontalSpacing(20);
    onlineGrid->setVerticalSpacing(4);

    QLabel *lblOnlineMethod = new QLabel("Payment Method", pageOnline);
    lblOnlineMethod->setStyleSheet(fieldLabelStyle);
    QLabel *lblOnlineTxn = new QLabel("Transaction ID", pageOnline);
    lblOnlineTxn->setStyleSheet(fieldLabelStyle);

    cmbOnlineMethod = new QComboBox(pageOnline);
    cmbOnlineMethod->addItems({"eSewa", "Khalti", "Online Banking", "Other"});
    cmbOnlineMethod->setStyleSheet(inputStyle);

    txtOnlineTxnId = new QLineEdit(pageOnline);
    txtOnlineTxnId->setStyleSheet(inputStyle);

    QLabel *lblOnlineRef = new QLabel("Payer Reference (eSewa ID / Wallet No.)", pageOnline);
    lblOnlineRef->setStyleSheet(fieldLabelStyle);
    txtOnlinePayerRef = new QLineEdit(pageOnline);
    txtOnlinePayerRef->setStyleSheet(inputStyle);

    onlineGrid->addWidget(lblOnlineMethod, 0, 0);
    onlineGrid->addWidget(cmbOnlineMethod, 1, 0);
    onlineGrid->addWidget(lblOnlineTxn, 0, 1);
    onlineGrid->addWidget(txtOnlineTxnId, 1, 1);
    onlineGrid->addWidget(lblOnlineRef, 2, 0, 1, 2);
    onlineGrid->addWidget(txtOnlinePayerRef, 3, 0, 1, 2);

    // -- Insurance page (Insurance Provider / Policy / Pre-Auth / Covered Amount) --
    pageInsurance = new QWidget();
    QVBoxLayout *insuranceLayout = new QVBoxLayout(pageInsurance);
    insuranceLayout->setContentsMargins(0, 0, 0, 0);
    insuranceLayout->setSpacing(10);

    QGridLayout *insGrid = new QGridLayout();
    insGrid->setHorizontalSpacing(20);
    insGrid->setVerticalSpacing(4);

    QLabel *lblProvider = new QLabel("Insurance Provider", pageInsurance);
    lblProvider->setStyleSheet(fieldLabelStyle);
    QLabel *lblPolicy = new QLabel("Policy / Member ID", pageInsurance);
    lblPolicy->setStyleSheet(fieldLabelStyle);

    cmbInsuranceProvider = new QComboBox(pageInsurance);
    cmbInsuranceProvider->addItems({"National Health Insurance Board", "Private Health Assurance", "Employer Group Plan", "Other"});
    cmbInsuranceProvider->setStyleSheet(inputStyle);

    txtPolicyMemberId = new QLineEdit(pageInsurance);
    txtPolicyMemberId->setStyleSheet(inputStyle);

    insGrid->addWidget(lblProvider, 0, 0);
    insGrid->addWidget(lblPolicy, 0, 1);
    insGrid->addWidget(cmbInsuranceProvider, 1, 0);
    insGrid->addWidget(txtPolicyMemberId, 1, 1);

    QGridLayout *authGrid = new QGridLayout();
    authGrid->setHorizontalSpacing(20);
    authGrid->setVerticalSpacing(4);

    QLabel *lblPreAuth = new QLabel("Pre-Auth Code", pageInsurance);
    lblPreAuth->setStyleSheet(fieldLabelStyle);
    QLabel *lblCovered = new QLabel("Covered Amount", pageInsurance);
    lblCovered->setStyleSheet(fieldLabelStyle);

    txtPreAuthCode = new QLineEdit(pageInsurance);
    txtPreAuthCode->setStyleSheet(inputStyle);

    spnCoveredAmount = new QDoubleSpinBox(pageInsurance);
    spnCoveredAmount->setRange(0.0, 500000.0);
    spnCoveredAmount->setPrefix("Rs. ");
    spnCoveredAmount->setStyleSheet(inputStyle);

    authGrid->addWidget(lblPreAuth, 0, 0);
    authGrid->addWidget(lblCovered, 0, 1);
    authGrid->addWidget(txtPreAuthCode, 1, 0);
    authGrid->addWidget(spnCoveredAmount, 1, 1);

    insuranceLayout->addLayout(insGrid);
    insuranceLayout->addLayout(authGrid);

    // Assemble stack: index 0=Cash, 1=Card, 2=Online, 3=Insurance
    stackedPaymentDetails->addWidget(pageCash);
    stackedPaymentDetails->addWidget(pageCard);
    stackedPaymentDetails->addWidget(pageOnline);
    stackedPaymentDetails->addWidget(pageInsurance);

    paymentLayout->addWidget(stackedPaymentDetails);

    // Notes
    QLabel *lblNotes = new QLabel("Notes", paymentFrame);
    lblNotes->setStyleSheet("color: #334155; font-size: 12px; font-weight: 600; border: none;");
    paymentLayout->addWidget(lblNotes);

    txtBillNotes = new QTextEdit(paymentFrame);
    txtBillNotes->setPlaceholderText("Enter your notes here...");
    txtBillNotes->setFixedHeight(60);
    txtBillNotes->setStyleSheet(inputStyle);
    paymentLayout->addWidget(txtBillNotes);

    // Bottom buttons
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();

    btnGenerateBill = new QPushButton("Generate Bill", paymentFrame);
    btnGenerateBill->setStyleSheet(
        "QPushButton { background-color: #0284C7; color: white; font-weight: bold; border-radius: 6px; padding: 10px 20px; border: none; } "
        "QPushButton:hover { background-color: #0369A1; } "
        "QPushButton:disabled { background-color: #CBD5E1; }"
        );
    connect(btnGenerateBill, &QPushButton::clicked, this, &ReceptionistWindow::onGenerateBillClicked);

    btnProcessPayment = new QPushButton("Process Payment", paymentFrame);
    btnProcessPayment->setStyleSheet(
        "QPushButton { background-color: #10B981; color: white; font-weight: bold; border-radius: 6px; padding: 10px 20px; border: none; } "
        "QPushButton:hover { background-color: #059669; } "
        "QPushButton:disabled { background-color: #CBD5E1; }"
        );
    connect(btnProcessPayment, &QPushButton::clicked, this, &ReceptionistWindow::onProcessPaymentClicked);

    btnRow->addWidget(btnGenerateBill);
    btnRow->addWidget(btnProcessPayment);
    paymentLayout->addLayout(btnRow);

    pageLayout->addWidget(paymentFrame);

    ui->page_6->layout()->addWidget(billingContainer);

    // Start with no patient looked up yet
    onClearBillingFormClicked();
}

// NEW: looks up a patient by ID or (partial) name, fills the info box, and
// loads their most recent unpaid bill (if any) into the Current Bill
// Summary table so the receptionist can collect payment or start a new bill.
void ReceptionistWindow::onSearchBillingPatientClicked()
{
    if (!txtBillSearch || !patientMgr) return;

    QString query = txtBillSearch->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::warning(this, "Notice", "Enter a patient name or ID to search.");
        return;
    }

    patientMgr->reload();
    const auto patients = patientMgr->getAllPatients();

    Patient found;
    for (const Patient &p : patients) {
        if (p.id.compare(query, Qt::CaseInsensitive) == 0) {
            found = p;
            break;
        }
    }
    if (found.id.isEmpty()) {
        for (const Patient &p : patients) {
            if (p.name.contains(query, Qt::CaseInsensitive)) {
                found = p;
                break;
            }
        }
    }

    if (found.id.isEmpty()) {
        QMessageBox::warning(this, "Not Found", "No patient matched \"" + query + "\".");
        return;
    }

    currentLookupPatientId = found.id;
    currentGeneratedBillId.clear();

    if (lblBillPatientId) lblBillPatientId->setText("Patient ID: " + found.id);
    if (lblBillPatientName) lblBillPatientName->setText("Name: " + found.name);
    if (lblBillPatientAge) lblBillPatientAge->setText("Age: " + found.age);
    if (lblBillPatientGender) lblBillPatientGender->setText("Gender: " + found.gender);

    if (tblCurrentBillSummary) tblCurrentBillSummary->setRowCount(0);
    if (spnAmountToPay) spnAmountToPay->setValue(0.0);

    // Load the patient's most recent unpaid bill, if one exists
    if (billingMgr) {
        billingMgr->reload();
        const auto bills = billingMgr->getAllBills();
        for (const BillingRecord &b : bills) {
            if (b.patientId.compare(found.id, Qt::CaseInsensitive) == 0 && b.remainingBalance > 0.001) {
                currentGeneratedBillId = b.billId;

                if (tblCurrentBillSummary) {
                    int row = 0;
                    for (const BillItem &item : b.items) {
                        tblCurrentBillSummary->insertRow(row);
                        tblCurrentBillSummary->setItem(row, 0, new QTableWidgetItem(item.serviceCode));
                        tblCurrentBillSummary->setItem(row, 1, new QTableWidgetItem(item.description));
                        tblCurrentBillSummary->setItem(row, 2, new QTableWidgetItem(QString::number(item.amount, 'f', 2)));
                        row++;
                    }
                }

                if (spnAmountToPay) spnAmountToPay->setValue(b.remainingBalance);
                break;
            }
        }
    }

    if (btnGenerateBill) btnGenerateBill->setEnabled(true);
    if (btnProcessPayment) btnProcessPayment->setEnabled(!currentGeneratedBillId.isEmpty());
}

// NEW: switches the stacked widget page based on the selected payment
// mode, so only the fields relevant to Cash / Card / Online / Insurance
// are shown to the receptionist.
void ReceptionistWindow::onPaymentModeChanged()
{
    if (!stackedPaymentDetails) return;

    if (radCash && radCash->isChecked()) {
        stackedPaymentDetails->setCurrentWidget(pageCash);
        updateChangeDue();
    } else if (radCard && radCard->isChecked()) {
        stackedPaymentDetails->setCurrentWidget(pageCard);
    } else if (radOnline && radOnline->isChecked()) {
        stackedPaymentDetails->setCurrentWidget(pageOnline);
    } else if (radInsurance && radInsurance->isChecked()) {
        stackedPaymentDetails->setCurrentWidget(pageInsurance);
    }
}

// NEW: recomputes "Change Due" on the Cash page whenever Cash Received or
// Amount to Pay changes.
void ReceptionistWindow::updateChangeDue()
{
    if (!lblChangeDue || !spnCashReceived || !spnAmountToPay) return;
    double change = spnCashReceived->value() - spnAmountToPay->value();
    lblChangeDue->setText(QString("Change Due: Rs. %1").arg(change > 0 ? change : 0.0, 0, 'f', 2));
}

// NEW: creates a bill for the looked-up patient using the Amount to Pay /
// Discount / Notes fields, mirroring what the old dropdown-driven
// "Generate Invoice" flow used to do, but sourced from the new layout.
void ReceptionistWindow::onGenerateBillClicked()
{
    if (currentLookupPatientId.isEmpty()) {
        QMessageBox::warning(this, "Notice", "Please search for a patient first.");
        return;
    }
    if (!billingMgr) return;

    double amount = spnAmountToPay ? spnAmountToPay->value() : 0.0;
    double discount = spnDiscount ? spnDiscount->value() : 0.0;

    if (amount <= 0.0) {
        QMessageBox::warning(this, "Validation Error", "Amount to Pay must be greater than 0.");
        return;
    }

    QString desc = txtBillNotes && !txtBillNotes->toPlainText().trimmed().isEmpty()
                       ? txtBillNotes->toPlainText().trimmed()
                       : "General Services";

    BillItem item;
    item.serviceCode = "SVC";
    item.description = desc;
    item.amount = amount;

    QVector<BillItem> items = { item };

    QString billId = billingMgr->generateBill(currentLookupPatientId, items, discount, 0.0,
                                              "Generated via Receptionist Dashboard");

    if (billId.isEmpty()) {
        QMessageBox::warning(this, "Error", "Could not generate the bill.");
        return;
    }

    currentGeneratedBillId = billId;

    // If paying by insurance and a covered amount was entered, record it
    // as an immediate partial payment against the new bill.
    if (radInsurance && radInsurance->isChecked() && spnCoveredAmount && spnCoveredAmount->value() > 0.0) {
        QString insNote = QString("Insurance: %1 | Policy: %2 | Pre-Auth: %3")
        .arg(cmbInsuranceProvider ? cmbInsuranceProvider->currentText() : "",
             txtPolicyMemberId ? txtPolicyMemberId->text() : "",
             txtPreAuthCode ? txtPreAuthCode->text() : "");
        billingMgr->processPayment(billId, spnCoveredAmount->value(), "Insurance", insNote);
    }

    QMessageBox::information(this, "Bill Generated", "Bill " + billId + " generated successfully.");

    onSearchBillingPatientClicked(); // refresh summary table + remaining balance
    refreshDashboardStats();
}

// NEW: collects payment against the currently loaded bill using the
// selected Payment Mode and remaining Amount to Pay. Also captures the
// mode-specific details entered in the stacked widget into the bill notes.
void ReceptionistWindow::onProcessPaymentClicked()
{
    if (currentGeneratedBillId.isEmpty()) {
        QMessageBox::warning(this, "Notice", "No active bill to pay. Generate a bill first.");
        return;
    }
    if (!billingMgr) return;

    double amount = spnAmountToPay ? spnAmountToPay->value() : 0.0;
    if (amount <= 0.0) {
        QMessageBox::warning(this, "Validation Error", "Amount to Pay must be greater than 0.");
        return;
    }

    QString mode = "Cash";
    if (radCard && radCard->isChecked()) mode = "Card";
    else if (radOnline && radOnline->isChecked()) mode = "Online";
    else if (radInsurance && radInsurance->isChecked()) mode = "Insurance";

    QString userNotes = txtBillNotes ? txtBillNotes->toPlainText().trimmed() : "";
    QString modeDetails;

    if (mode == "Cash") {
        double received = spnCashReceived ? spnCashReceived->value() : 0.0;
        modeDetails = QString("Cash Received: Rs. %1 | Change: Rs. %2")
                          .arg(received, 0, 'f', 2)
                          .arg(qMax(0.0, received - amount), 0, 'f', 2);
    } else if (mode == "Card") {
        modeDetails = QString("Card: %1 ending %2 | Auth: %3")
        .arg(cmbCardType ? cmbCardType->currentText() : "",
             txtCardLast4 ? txtCardLast4->text() : "",
             txtCardAuthCode ? txtCardAuthCode->text() : "");
    } else if (mode == "Online") {
        modeDetails = QString("%1 | Txn ID: %2 | Ref: %3")
        .arg(cmbOnlineMethod ? cmbOnlineMethod->currentText() : "",
             txtOnlineTxnId ? txtOnlineTxnId->text() : "",
             txtOnlinePayerRef ? txtOnlinePayerRef->text() : "");
    } else if (mode == "Insurance") {
        modeDetails = QString("Insurance: %1 | Policy: %2 | Pre-Auth: %3")
        .arg(cmbInsuranceProvider ? cmbInsuranceProvider->currentText() : "",
             txtPolicyMemberId ? txtPolicyMemberId->text() : "",
             txtPreAuthCode ? txtPreAuthCode->text() : "");
    }

    QString finalNotes = userNotes.isEmpty() ? modeDetails : userNotes + " | " + modeDetails;

    billingMgr->processPayment(currentGeneratedBillId, amount, mode, finalNotes);

    QMessageBox::information(this, "Payment Processed",
                             "Payment recorded for bill " + currentGeneratedBillId + ".");

    onClearBillingFormClicked();
    refreshDashboardStats();
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
    onClearBillingFormClicked();
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

// NOTE: cmbBillPatient no longer exists (Billing page now uses the
// search box instead of a dropdown) — this function now only populates
// the Schedule page's patient dropdown.
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