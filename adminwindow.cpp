#include "mainwindow.h"
#include "adminwindow.h"
#include "ui_adminwindow.h"
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QDate>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QButtonGroup>
#include <QScrollArea>
#include <QScrollBar>

#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QSpacerItem>
#include <QApplication>


static void embedChart(QWidget *container, QChart *chart) {
    if (!container || !chart) return;
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->legend()->hide();

    QChartView *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);

    if (!container->layout()) {
        QVBoxLayout *l = new QVBoxLayout(container);
        l->setContentsMargins(0, 0, 0, 0);
        container->setLayout(l);
    } else {
        QLayoutItem *item;
        while ((item = container->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) delete item->widget();
            delete item;
        }
    }
    container->layout()->addWidget(view);
}



static void applyStatusBadge(QLabel *label, const QString &status) {
    label->setText(status.toUpper());
    label->setAlignment(Qt::AlignCenter);
    QString norm = status.toLower().trimmed();

    if (norm == "admitted" || norm == "checked in" || norm == "present" || norm == "on duty") {
        label->setStyleSheet("font-weight:bold;font-size:11px;padding:4px 8px;border:none;");
    } else if (norm == "discharged" || norm == "no show" || norm == "absent" || norm == "on leave") {
        label->setStyleSheet("font-weight:bold;font-size:11px;padding:4px 8px;border:none;");
    } else {
        label->setStyleSheet("font-weight:bold;font-size:11px;padding:4px 8px;border:none;");
    }
}

static void applyRoleBadge(QLabel *label, const QString &role) {
    label->setText(role.toUpper());
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-weight:bold;font-size:11px;padding:4px 8px;border:none;");
}

static const QString kPlainLabel =
    "border:none;background:transparent;background-color:transparent;";




adminwindow::adminwindow(const QString &employeeName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AXON_ADMIN)
    , currentAdminName(employeeName)
{
    ui->setupUi(this);
    this->setWindowTitle("AXON-HMS: Admin's Dashboard");

    QButtonGroup *sidebarGroup = new QButtonGroup(this);
    sidebarGroup->addButton(ui->btnOverview);
    sidebarGroup->addButton(ui->btnStaffManager);
    sidebarGroup->addButton(ui->btnScheduling);
    sidebarGroup->setExclusive(true);

    staffMgr     = new StaffManager();
    patientMgr   = new PatientManager();
    apptMgr      = new AppointmentManager();
    billMgr      = new BillingManager();   // NEW: backs Total Revenue (Today)
    adminBackend = nullptr;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &adminwindow::updateDateTime);
    timer->start(1000);
    updateDateTime();

    QPixmap logoPixmap(":/images/axonimg.png");
    if (ui->lblLogo && !logoPixmap.isNull())
        ui->lblLogo->setPixmap(logoPixmap.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (ui->message)
        ui->message->setText(QString("Welcome back, %1!").arg(currentAdminName));

    if (ui->stackedWidget) ui->stackedWidget->setCurrentIndex(0);
    ui->btnOverview->setChecked(true);

    initDashboardGraphs();
    setupPatientHeader();
    loadPatientRowsFromBackend();
    setupStaffPage();
    setupSchedulingPage();
}

adminwindow::~adminwindow()
{
    delete ui;
    delete staffMgr;
    delete patientMgr;
    delete apptMgr;
    delete billMgr;
    if (adminBackend) delete adminBackend;
}




void adminwindow::updateDateTime()
{
    if (ui->lblDateTime)
        ui->lblDateTime->setText(
            QDateTime::currentDateTime().toString("ddd MMM d yyyy,  hh:mm AP"));
}




void adminwindow::on_btnMenu_clicked()
{
    QMenu menu(this);
    QAction *logoutAction = menu.addAction("Logout");
    QPoint pos = ui->btnMenu->mapToGlobal(QPoint(0, ui->btnMenu->height()));
    if (menu.exec(pos) == logoutAction) {
        MainWindow *loginScreen = new MainWindow();
        loginScreen->show();
        this->close();
    }
}

void adminwindow::on_btnOverview_clicked()
{
    if (ui->stackedWidget) ui->stackedWidget->setCurrentIndex(0);
    staffMgr->reload();
    patientMgr->reload();
    billMgr->reload();
    initDashboardGraphs();
    loadPatientRowsFromBackend();
}

void adminwindow::on_btnStaffManager_clicked()
{
    if (ui->stackedWidget) ui->stackedWidget->setCurrentIndex(1);
    staffMgr->reload();
    refreshStaffTable();
}

void adminwindow::on_btnScheduling_clicked()
{
    apptMgr->reload();
    staffMgr->reload();
    refreshSchedulingTable();
    if (schedulingPage) ui->stackedWidget->setCurrentWidget(schedulingPage);
}


//Dashboard charts

void adminwindow::initDashboardGraphs()
{
    int totalStaff    = staffMgr->getTotalCount();
    int totalPatients = patientMgr->getTotalCount();

    int activeStaff = 0, inactiveStaff = 0;
    for (const auto &s : staffMgr->getAllStaff()) {
        if (s.status.toLower().trimmed() == "on leave") inactiveStaff++;
        else                                             activeStaff++;
    }

    if (ui->lblValueStaff)
        ui->lblValueStaff->setText(QString("%1 / %2").arg(activeStaff).arg(totalStaff));
    if (ui->lblValuePatients)
        ui->lblValuePatients->setText(QString::number(totalPatients));

    // Staff pie
    QPieSeries *staffSeries = new QPieSeries();
    staffSeries->setPieSize(1.0);
    staffSeries->append("Active",   activeStaff)->setBrush(QColor(0x166534));
    staffSeries->append("Inactive", inactiveStaff)->setBrush(QColor(0xbbf7d0));
    QChart *staffChart = new QChart();
    staffChart->addSeries(staffSeries);
    if (ui->widgetGraphStaff) embedChart(ui->widgetGraphStaff, staffChart);
    if (ui->lblSubtextStaff)
        ui->lblSubtextStaff->setText("<span style='color:#166534;'>●</span> Active &nbsp;"
                                     "<span style='color:#bbf7d0;'>●</span> Inactive");


    int activeDoctors = 0, inactiveDoctors = 0;
    for (const auto &s : staffMgr->getAllStaff()) {
        if (s.role.toLower().trimmed() == "doctor") {
            if (s.status.toLower().trimmed() == "on leave") inactiveDoctors++;
            else                                             activeDoctors++;
        }
    }
    QPieSeries *docSeries = new QPieSeries();
    docSeries->setHoleSize(0.75);
    docSeries->setPieSize(0.95);
    docSeries->append("Active",   activeDoctors)->setBrush(QColor(0x0284c7));
    docSeries->append("On Leave", inactiveDoctors)->setBrush(QColor(0xe0f2fe));
    QChart *docChart = new QChart();
    docChart->addSeries(docSeries);
    if (ui->widgetGraphDoctors) embedChart(ui->widgetGraphDoctors, docChart);
    if (ui->lblValueDoctors)
        ui->lblValueDoctors->setText(
            QString("%1 / %2").arg(activeDoctors).arg(activeDoctors + inactiveDoctors));
    if (ui->lblSubtextDoctors)
        ui->lblSubtextDoctors->setText("<span style='color:#0284c7;'>●</span> Active &nbsp;"
                                       "<span style='color:#e0f2fe;'>●</span> On Leave");

    // Patient gender breakdown
    int male = 0, female = 0, other = 0;
    for (const auto &p : patientMgr->getAllPatients()) {
        QString g = p.gender.toLower();
        if (g == "male")        male++;
        else if (g == "female") female++;
        else                    other++;
    }
    QPieSeries *patSeries = new QPieSeries();
    patSeries->setHoleSize(0.35);
    if (male)   patSeries->append("Male",   male)->setBrush(QColor(0x6366f1));
    if (female) patSeries->append("Female", female)->setBrush(QColor(0xec4899));
    if (other)  patSeries->append("Other",  other)->setBrush(QColor(0xf59e0b));
    QChart *patChart = new QChart();
    patChart->addSeries(patSeries);
    if (ui->widgetGraphPatients) embedChart(ui->widgetGraphPatients, patChart);
    if (ui->lblSubtextPatients)
        ui->lblSubtextPatients->setText(
            "<span style='color:#6366f1;'>●</span> Male &nbsp;"
            "<span style='color:#ec4899;'>●</span> Female &nbsp;"
            "<span style='color:#f59e0b;'>●</span> Other");

    // ===== Revenue trend — FIX: was 100% hardcoded mock data. Now computed
    // live from BillingManager / billing_database.csv. "Today" is defined
    // as bills GENERATED today (the only date the billing model tracks),
    // summed by amountToPay (money actually collected via processPayment).
    QString todayStr = QDate::currentDate().toString("yyyy-MM-dd");
    double revenueToday = 0.0;
    for (const auto &b : billMgr->getAllBills())
        if (b.date == todayStr) revenueToday += b.amountToPay;

    if (ui->lblValueRevenue)
        ui->lblValueRevenue->setText(QString("Rs. %1").arg(revenueToday, 0, 'f', 2));

    QLineSeries *revSeries = new QLineSeries();
    QDate startDate = QDate::currentDate().addDays(-3);
    for (int i = 0; i < 4; ++i) {
        QDate d = startDate.addDays(i);
        QString dStr = d.toString("yyyy-MM-dd");
        double dayTotal = 0.0;
        for (const auto &b : billMgr->getAllBills())
            if (b.date == dStr) dayTotal += b.amountToPay;
        revSeries->append(i, dayTotal);
    }
    revSeries->setPen(QPen(QColor(0x0d9488), 4));
    QChart *revChart = new QChart();
    revChart->addSeries(revSeries);
    revChart->createDefaultAxes();
    if (ui->widgetGraphRevenue) embedChart(ui->widgetGraphRevenue, revChart);
    if (ui->lblSubtextRevenue)
        ui->lblSubtextRevenue->setText("<span style='color:#0d9488;'>●</span> Revenue &nbsp;"
                                       "<span style='color:#880808;'>●</span> Expense");
}



void adminwindow::setupPatientHeader()
{
    if (!ui->webContainer) return;

    QWidget *headerWidget = new QWidget();
    QHBoxLayout *hl = new QHBoxLayout(headerWidget);
    hl->setContentsMargins(15, 5, 15, 5);
    hl->setSpacing(10);

    const QString hStyle =
        "font-weight:bold;font-size:11px;color:#94a3b8;"
        "border:none;background:transparent;background-color:transparent;";
    auto makeH = [&](const QString &txt, int stretch) {
        QLabel *l = new QLabel(txt);
        l->setStyleSheet(hStyle);
        hl->addWidget(l, stretch);
    };
    makeH("PATIENT ID", 1); makeH("NAME", 2); makeH("GENDER", 1);
    makeH("PROBLEM / DIAGNOSIS", 2); makeH("ASSIGNED DOCTOR", 2);
    makeH("STATUS", 1); makeH("ACTION", 1);
    headerWidget->setLayout(hl);

    QVBoxLayout *cardLayout = qobject_cast<QVBoxLayout*>(ui->webContainer->layout());
    if (!cardLayout) {
        cardLayout = new QVBoxLayout(ui->webContainer);
        cardLayout->setContentsMargins(15, 15, 15, 15);
        cardLayout->setSpacing(8);
        ui->webContainer->setLayout(cardLayout);
    }
    cardLayout->insertWidget(0, headerWidget);
}

void adminwindow::loadPatientRowsFromBackend()
{
    // Clear any previously-built rows below the header (index 0) before
    // repainting, otherwise reload() would duplicate rows on every refresh.
    if (QVBoxLayout *cardLayout = qobject_cast<QVBoxLayout*>(ui->webContainer->layout())) {
        while (cardLayout->count() > 1) {
            QLayoutItem *item = cardLayout->takeAt(1);
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
    }

    auto patients     = patientMgr->getAllPatients();
    int totalPatients = patients.size();
    int displayCount  = std::min(5, totalPatients);

    for (int i = 0; i < displayCount; ++i) {
        const auto &p = patients[totalPatients - 1 - i];
        addPatientRow(p.id, p.name, p.gender, p.diagnosisTreatment,
                      p.assignedDoctor, p.status);
    }

    if (patients.isEmpty()) {
        addPatientRow("PT-0001", "Aditya Poudel", "Male",   "Cardiac Checkup",   "Dr. Rijal",   "Admitted");
        addPatientRow("PT-0002", "Mira Gurung",   "Female", "Migraine Treatment", "Dr. Subedi",  "Discharged");
        addPatientRow("PT-0003", "Hari Rana",     "Male",   "Fracture Follow-up", "Dr. Baral",   "Admitted");
        addPatientRow("PT-0004", "Sita Wagle",    "Female", "General Physical",   "Dr. Acharya", "Completed");
    }
}

void adminwindow::addPatientRow(const QString &id,     const QString &name,
                                const QString &gender, const QString &problem,
                                const QString &doctor, const QString &status)
{
    if (!ui->webContainer) return;

    QFrame *row = new QFrame();
    row->setObjectName("patientRow");
    row->setStyleSheet(
        "QFrame#patientRow{background-color:#ffffff;border-bottom:1px solid #f1f5f9;}"
        "QFrame#patientRow:hover{background-color:#f8fafc;}");

    QHBoxLayout *rl = new QHBoxLayout(row);
    rl->setContentsMargins(15, 10, 15, 10);
    rl->setSpacing(10);

    QLabel *lblId      = new QLabel(id);
    QLabel *lblName    = new QLabel(name);
    QLabel *lblGender  = new QLabel(gender);
    QLabel *lblProblem = new QLabel(problem);
    QLabel *lblDoctor  = new QLabel(doctor);
    QLabel *lblStatus  = new QLabel();
    QPushButton *btnEdit = new QPushButton("Edit");

    lblId->setStyleSheet("font-weight:bold;color:#64748b;" + kPlainLabel);
    lblName->setStyleSheet("font-weight:bold;color:#1e293b;" + kPlainLabel);
    lblGender->setStyleSheet("color:#334155;" + kPlainLabel);
    lblProblem->setStyleSheet("color:#334155;" + kPlainLabel);
    lblDoctor->setStyleSheet("color:#334155;" + kPlainLabel);
    applyStatusBadge(lblStatus, status);

    btnEdit->setStyleSheet(
        "QPushButton{background-color:#f1f5f9;color:#0f172a;border:1px solid #e2e8f0;"
        "border-radius:6px;font-weight:bold;font-size:12px;padding:4px 12px;max-width:60px;}"
        "QPushButton:hover{background-color:#e2e8f0;}"
        "QPushButton:pressed{background-color:#cbd5e1;}");

    connect(btnEdit, &QPushButton::clicked, this, [=]() {
        // Out-parameter widgets — factory fills these in
        QLineEdit *nameEdit    = nullptr;
        QComboBox *genderBox   = nullptr;
        QLineEdit *problemEdit = nullptr;
        QLineEdit *doctorEdit  = nullptr;
        QComboBox *statusBox   = nullptr;

        QDialog *dlg = createEditPatientDialog(
            lblId->text(), lblName->text(), lblGender->text(),
            lblProblem->text(), lblDoctor->text(), lblStatus->text(),
            this,
            nameEdit, genderBox, problemEdit, doctorEdit, statusBox);

        if (dlg->exec() == QDialog::Accepted) {
            lblName->setText(nameEdit->text());
            lblGender->setText(genderBox->currentText());
            lblProblem->setText(problemEdit->text());
            lblDoctor->setText(doctorEdit->text());
            applyStatusBadge(lblStatus, statusBox->currentText());

            Patient updated        = patientMgr->searchPatient(lblId->text());
            updated.name               = nameEdit->text();
            updated.gender             = genderBox->currentText();
            updated.diagnosisTreatment = problemEdit->text();
            updated.assignedDoctor     = doctorEdit->text();
            updated.status             = statusBox->currentText();
            patientMgr->removePatient(lblId->text());
            patientMgr->addPatient(updated);
        }
        dlg->deleteLater();
    });

    rl->addWidget(lblId,      1);
    rl->addWidget(lblName,    2);
    rl->addWidget(lblGender,  1);
    rl->addWidget(lblProblem, 2);
    rl->addWidget(lblDoctor,  2);
    rl->addWidget(lblStatus,  1);
    rl->addWidget(btnEdit,    1);
    row->setLayout(rl);

    QVBoxLayout *cl = qobject_cast<QVBoxLayout*>(ui->webContainer->layout());
    if (cl) cl->addWidget(row);
}


//Staff Manager page

void adminwindow::setupStaffPage()
{
    QWidget *page2 = ui->stackedWidget->widget(1);
    if (!page2) return;

    if (page2->layout()) {
        QLayoutItem *item;
        while ((item = page2->layout()->takeAt(0))) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete page2->layout();
    }

    QVBoxLayout *pageLayout = new QVBoxLayout(page2);
    pageLayout->setContentsMargins(20, 16, 20, 16);
    pageLayout->setSpacing(14);

    // Header row
    QHBoxLayout *headerRow = new QHBoxLayout();
    QLabel *title = new QLabel("Staff Manager");
    title->setStyleSheet(
        "font-size:22px;font-weight:700;color:#0f172a;border:none;background:transparent;");

    QPushButton *btnAdd = new QPushButton("＋  Add Staff");
    btnAdd->setFixedHeight(38);
    btnAdd->setStyleSheet(
        "QPushButton{background-color:#0284c7;color:#ffffff;border:none;"
        "border-radius:8px;font-weight:700;font-size:13px;padding:0 18px;}"
        "QPushButton:hover{background-color:#0369a1;}"
        "QPushButton:pressed{background-color:#075985;}");

    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(btnAdd);
    pageLayout->addLayout(headerRow);

    staffCountLabel = new QLabel();
    staffCountLabel->setStyleSheet(
        "font-size:13px;color:#64748b;border:none;background:transparent;");
    updateStaffCountLabel();
    pageLayout->addWidget(staffCountLabel);

    // Column header bar
    QWidget *colHeader = new QWidget();
    colHeader->setStyleSheet(
        "background-color:#f8fafc;border-bottom:2px solid #e2e8f0;border-radius:0px;");
    QHBoxLayout *chLayout = new QHBoxLayout(colHeader);
    chLayout->setContentsMargins(16, 8, 16, 8);
    chLayout->setSpacing(10);
    const QString chStyle =
        "font-weight:700;font-size:11px;color:#94a3b8;"
        "border:none;background:transparent;background-color:transparent;";
    auto makeCol = [&](const QString &txt, int s) {
        QLabel *l = new QLabel(txt); l->setStyleSheet(chStyle);
        chLayout->addWidget(l, s);
    };
    makeCol("STAFF ID", 1); makeCol("FULL NAME", 2); makeCol("ROLE",    1);
    makeCol("AGE",      1); makeCol("GENDER",    1); makeCol("PHONE",   2);
    makeCol("USERNAME", 2); makeCol("STATUS",    2); makeCol("ACTIONS", 2);
    pageLayout->addWidget(colHeader);

    // Scrollable rows area
    staffScrollArea = new QScrollArea();
    staffScrollArea->setWidgetResizable(true);
    staffScrollArea->setFrameShape(QFrame::NoFrame);
    staffScrollArea->setStyleSheet("QScrollArea{border:none;background:transparent;}");

    staffRowContainer = new QWidget();
    staffRowContainer->setStyleSheet("background:transparent;");
    staffRowsLayout = new QVBoxLayout(staffRowContainer);
    staffRowsLayout->setContentsMargins(0, 0, 0, 0);
    staffRowsLayout->setSpacing(0);
    staffRowsLayout->addStretch();

    staffScrollArea->setWidget(staffRowContainer);
    pageLayout->addWidget(staffScrollArea, 1);

    refreshStaffTable();
    initDashboardGraphs();

    connect(btnAdd, &QPushButton::clicked, this, &adminwindow::onAddStaffClicked);
}

void adminwindow::updateStaffCountLabel()
{
    if (!staffCountLabel) return;
    int total = staffMgr->getTotalCount();
    int docs = 0, admins = 0, recs = 0;
    for (const auto &s : staffMgr->getAllStaff()) {
        if      (s.role == "Doctor")       docs++;
        else if (s.role == "Admin")        admins++;
        else                               recs++;
    }
    staffCountLabel->setText(
        QString("Total Staff: <b>%1</b> &nbsp;|&nbsp; "
                "Doctors: <b>%2</b> &nbsp;|&nbsp; "
                "Admins: <b>%3</b> &nbsp;|&nbsp; "
                "Receptionists: <b>%4</b>")
        .arg(total).arg(docs).arg(admins).arg(recs));
}

void adminwindow::refreshStaffTable()
{
    while (staffRowsLayout->count() > 1) {
        QLayoutItem *item = staffRowsLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    for (const auto &s : staffMgr->getAllStaff())
        addStaffRow(s);
    updateStaffCountLabel();
}

void adminwindow::addStaffRow(const StaffData &s)
{
    QFrame *row = new QFrame();
    row->setObjectName("staffRow");
    row->setStyleSheet(
        "QFrame#staffRow{background-color:#ffffff;border-bottom:1px solid #f1f5f9;}"
        "QFrame#staffRow:hover{background-color:#f8fafc;}");

    QHBoxLayout *rl = new QHBoxLayout(row);
    rl->setContentsMargins(16, 10, 16, 10);
    rl->setSpacing(10);

    QLabel *lblId     = new QLabel(s.id);
    QLabel *lblName   = new QLabel(s.name);
    QLabel *lblRole   = new QLabel();
    QLabel *lblAge    = new QLabel(s.age);
    QLabel *lblGender = new QLabel(s.gender);
    QLabel *lblPhone  = new QLabel(s.phone);
    QLabel *lblUser   = new QLabel(s.username);
    QLabel *lblStatus = new QLabel();

    lblId->setStyleSheet("font-weight:bold;color:#64748b;" + kPlainLabel);
    lblName->setStyleSheet("font-weight:bold;color:#1e293b;" + kPlainLabel);
    lblAge->setStyleSheet("color:#334155;" + kPlainLabel);
    lblGender->setStyleSheet("color:#334155;" + kPlainLabel);
    lblPhone->setStyleSheet("color:#334155;" + kPlainLabel);
    lblUser->setStyleSheet("font-family:monospace;color:#0369a1;" + kPlainLabel);
    applyRoleBadge(lblRole, s.role);
    applyStatusBadge(lblStatus, s.status.isEmpty() ? "On Duty" : s.status);

    QWidget *actionWidget = new QWidget();
    actionWidget->setStyleSheet("background:transparent;border:none;");
    QHBoxLayout *al = new QHBoxLayout(actionWidget);
    al->setContentsMargins(0, 0, 0, 0);
    al->setSpacing(6);

    QPushButton *btnEdit   = new QPushButton("Edit");
    QPushButton *btnRemove = new QPushButton("Remove");
    btnEdit->setFixedSize(52, 28);
    btnRemove->setFixedSize(68, 28);
    btnEdit->setStyleSheet(
        "QPushButton{background:#f1f5f9;color:#0f172a;border:1px solid #e2e8f0;"
        "border-radius:6px;font-weight:bold;font-size:11px;}"
        "QPushButton:hover{background:#e2e8f0;}");
    btnRemove->setStyleSheet(
        "QPushButton{background:#fee2e2;color:#b91c1c;border:1px solid #fecaca;"
        "border-radius:6px;font-weight:bold;font-size:11px;}"
        "QPushButton:hover{background:#fecaca;}");
    al->addWidget(btnEdit);
    al->addWidget(btnRemove);
    actionWidget->setLayout(al);

    QString staffIdCopy = s.id;

    connect(btnEdit, &QPushButton::clicked, this, [=]() {
        StaffData current = staffMgr->searchStaff(staffIdCopy);

        // Out-parameter widgets — factory fills these in
        QLineEdit *usernameEdit = nullptr;
        QLineEdit *passwordEdit = nullptr;
        QComboBox *roleBox      = nullptr;
        QLineEdit *nameEdit     = nullptr;
        QLineEdit *ageEdit      = nullptr;
        QComboBox *genderBox    = nullptr;
        QLineEdit *phoneEdit    = nullptr;
        QComboBox *statusBox    = nullptr;

        QDialog *dlg = createEditStaffDialog(
            current, this,
            usernameEdit, passwordEdit, roleBox,
            nameEdit, ageEdit, genderBox,
            phoneEdit, statusBox);

        if (dlg->exec() == QDialog::Accepted) {
            current.username = usernameEdit->text().trimmed();
            current.password = passwordEdit->text().trimmed();
            current.role     = roleBox->currentText();
            current.name     = nameEdit->text().trimmed();
            current.age      = ageEdit->text().trimmed();
            current.gender   = genderBox->currentText();
            current.phone    = phoneEdit->text().trimmed();
            current.status   = statusBox->currentText();

            if (staffMgr->updateStaff(current)) {
                refreshStaffTable();
                initDashboardGraphs();
            }
        }
        dlg->deleteLater();
    });

    connect(btnRemove, &QPushButton::clicked, this, [=]() {
        StaffData s2 = staffMgr->searchStaff(staffIdCopy);
        auto reply = QMessageBox::question(this,
            "Confirm Removal",
            QString("Remove staff member <b>%1</b> (%2)?<br>"
                    "This will also delete their login credentials.")
            .arg(s2.name, s2.id),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            staffMgr->removeStaff(staffIdCopy);
            refreshStaffTable();
            initDashboardGraphs();
        }
    });

    rl->addWidget(lblId,       1);
    rl->addWidget(lblName,     2);
    rl->addWidget(lblRole,     1);
    rl->addWidget(lblAge,      1);
    rl->addWidget(lblGender,   1);
    rl->addWidget(lblPhone,    2);
    rl->addWidget(lblUser,     2);
    rl->addWidget(lblStatus,   2);
    rl->addWidget(actionWidget,2);
    row->setLayout(rl);

    staffRowsLayout->insertWidget(staffRowsLayout->count() - 1, row);
}

void adminwindow::onAddStaffClicked()
{
    // Out-parameter widgets — factory fills these in
    QLineEdit *usernameEdit = nullptr;
    QLineEdit *passwordEdit = nullptr;
    QComboBox *roleBox      = nullptr;
    QLineEdit *idEdit       = nullptr;
    QLineEdit *nameEdit     = nullptr;
    QLineEdit *ageEdit      = nullptr;
    QComboBox *genderBox    = nullptr;
    QLineEdit *phoneEdit    = nullptr;
    QComboBox *statusBox    = nullptr;

    QDialog *dlg = createAddStaffDialog(this,
        usernameEdit, passwordEdit, roleBox, idEdit,
        nameEdit, ageEdit, genderBox, phoneEdit, statusBox);

    if (dlg->exec() != QDialog::Accepted) {
        dlg->deleteLater();
        return;
    }

    StaffData newStaff;
    newStaff.username = usernameEdit->text().trimmed();
    newStaff.password = passwordEdit->text().trimmed();
    newStaff.role     = roleBox->currentText();
    newStaff.id       = idEdit->text().trimmed();
    newStaff.name     = nameEdit->text().trimmed();
    newStaff.age      = ageEdit->text().trimmed();
    newStaff.gender   = genderBox->currentText();
    newStaff.phone    = phoneEdit->text().trimmed();
    newStaff.status   = statusBox->currentText();

    dlg->deleteLater();

    // Duplicate checks
    for (const auto &s : staffMgr->getAllStaff()) {
        if (s.username == newStaff.username) {
            QMessageBox::warning(this, "Duplicate Username",
                "A staff member with that username already exists.");
            return;
        }
        if (s.id == newStaff.id) {
            QMessageBox::warning(this, "Duplicate Staff ID",
                "A staff member with that ID already exists.");
            return;
        }
    }

    staffMgr->addStaff(newStaff);
    refreshStaffTable();
    initDashboardGraphs();

    QMessageBox::information(this, "Staff Added",
        QString("Staff member <b>%1</b> added successfully.<br>"
                "Login: <b>%2</b> / <b>%3</b>")
        .arg(newStaff.name, newStaff.username, newStaff.password));
}


// Scheduling page

void adminwindow::setupSchedulingPage()
{
    schedulingPage = new QWidget();

    QVBoxLayout *pageLayout = new QVBoxLayout(schedulingPage);
    pageLayout->setContentsMargins(20, 16, 20, 16);
    pageLayout->setSpacing(14);

    QHBoxLayout *headerRow = new QHBoxLayout();
    QLabel *title = new QLabel("Scheduling");
    title->setStyleSheet(
        "font-size:22px;font-weight:700;color:#0f172a;border:none;background:transparent;");
    headerRow->addWidget(title);
    headerRow->addStretch();
    pageLayout->addLayout(headerRow);

    scheduleCountLabel = new QLabel();
    scheduleCountLabel->setStyleSheet(
        "font-size:13px;color:#64748b;border:none;background:transparent;");
    pageLayout->addWidget(scheduleCountLabel);


    QWidget *colHeader = new QWidget();
    colHeader->setStyleSheet(
        "background-color:#f8fafc;border-bottom:2px solid #e2e8f0;");
    QHBoxLayout *chLayout = new QHBoxLayout(colHeader);
    chLayout->setContentsMargins(16, 8, 16, 8);
    chLayout->setSpacing(10);
    const QString chStyle =
        "font-weight:700;font-size:11px;color:#94a3b8;"
        "border:none;background:transparent;background-color:transparent;";
    auto makeCol = [&](const QString &txt, int s) {
        QLabel *l = new QLabel(txt); l->setStyleSheet(chStyle);
        chLayout->addWidget(l, s);
    };
    makeCol("APPT ID", 1); makeCol("DATE", 1); makeCol("TIME", 1);
    makeCol("PATIENT", 2); makeCol("DOCTOR", 2); makeCol("DEPARTMENT", 2);
    makeCol("STATUS", 1);
    pageLayout->addWidget(colHeader);


    scheduleScrollArea = new QScrollArea();
    scheduleScrollArea->setWidgetResizable(true);
    scheduleScrollArea->setFrameShape(QFrame::NoFrame);
    scheduleScrollArea->setStyleSheet("QScrollArea{border:none;background:transparent;}");

    scheduleRowContainer = new QWidget();
    scheduleRowContainer->setStyleSheet("background:transparent;");
    scheduleRowsLayout = new QVBoxLayout(scheduleRowContainer);
    scheduleRowsLayout->setContentsMargins(0, 0, 0, 0);
    scheduleRowsLayout->setSpacing(0);
    scheduleRowsLayout->addStretch();

    scheduleScrollArea->setWidget(scheduleRowContainer);
    pageLayout->addWidget(scheduleScrollArea, 1);

    ui->stackedWidget->addWidget(schedulingPage);

    refreshSchedulingTable();
}

void adminwindow::refreshSchedulingTable()
{
    if (!scheduleRowsLayout) return;

    while (scheduleRowsLayout->count() > 1) {
        QLayoutItem *item = scheduleRowsLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    QVector<Appointment> all = apptMgr->getAllAppointments();
    for (const auto &a : all)
        addSchedulingRow(a);

    if (scheduleCountLabel)
        scheduleCountLabel->setText(QString("Total Appointments: <b>%1</b>").arg(all.size()));
}

void adminwindow::addSchedulingRow(const Appointment &a)
{
    QFrame *row = new QFrame();
    row->setObjectName("scheduleRow");
    row->setStyleSheet(
        "QFrame#scheduleRow{background-color:#ffffff;border-bottom:1px solid #f1f5f9;}"
        "QFrame#scheduleRow:hover{background-color:#f8fafc;}");

    QHBoxLayout *rl = new QHBoxLayout(row);
    rl->setContentsMargins(16, 10, 16, 10);
    rl->setSpacing(10);

    QLabel *lblId     = new QLabel(a.id);
    QLabel *lblDate   = new QLabel(a.date);
    QLabel *lblTime   = new QLabel(a.time);
    QLabel *lblPat    = new QLabel(a.patientName);
    QLabel *lblDoc    = new QLabel(a.doctorName);
    QLabel *lblDept   = new QLabel(a.department);
    QLabel *lblStatus = new QLabel();

    lblId->setStyleSheet("font-weight:bold;color:#64748b;" + kPlainLabel);
    lblDate->setStyleSheet("color:#334155;" + kPlainLabel);
    lblTime->setStyleSheet("color:#334155;" + kPlainLabel);
    lblPat->setStyleSheet("font-weight:bold;color:#1e293b;" + kPlainLabel);
    lblDoc->setStyleSheet("color:#334155;" + kPlainLabel);
    lblDept->setStyleSheet("color:#334155;" + kPlainLabel);
    applyStatusBadge(lblStatus, a.status);

    rl->addWidget(lblId,     1);
    rl->addWidget(lblDate,   1);
    rl->addWidget(lblTime,   1);
    rl->addWidget(lblPat,    2);
    rl->addWidget(lblDoc,    2);
    rl->addWidget(lblDept,   2);
    rl->addWidget(lblStatus, 1);
    row->setLayout(rl);

    scheduleRowsLayout->insertWidget(scheduleRowsLayout->count() - 1, row);
}
