#include "staffmanager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

StaffManager::StaffManager() {
    loadAll();
}

void StaffManager::reload() {
    loadAll();
}

void StaffManager::loadAll() {
    staffList.clear();

    QFile file("database/staff_database.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Fallback: try the embedded resource (read-only, used as seed)
        QFile res(":/database/staff_database.csv");
        if (!res.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "StaffManager: staff_database.csv not found anywhere.";
            return;
        }
        QTextStream in(&res);
        _parseStream(in);
        res.close();
        saveAll(); // write out to working directory so future edits persist
        return;
    }

    QTextStream in(&file);
    _parseStream(in);
    file.close();
}

void StaffManager::_parseStream(QTextStream &in) {
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        QStringList f = line.split(",");
        // Format: username,password,Role,staff_id,full_name,age,gender,phone,status
        if (f.size() >= 8) {
            StaffData s;
            s.username = f[0].trimmed();
            s.password = f[1].trimmed();
            s.role     = f[2].trimmed();
            s.id       = f[3].trimmed();
            s.name     = f[4].trimmed();
            s.age      = f[5].trimmed();
            s.gender   = f[6].trimmed();
            s.phone    = f[7].trimmed();
            s.status   = (f.size() >= 9) ? f[8].trimmed() : "On Duty";
            staffList.append(s);
        }
    }
}

void StaffManager::saveAll() {
    QFile file("database/staff_database.csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "StaffManager: Cannot open staff_database.csv for writing.";
        return;
    }
    QTextStream out(&file);
    out << "# username,password,Role,staff_id,full_name,age,gender,phone,status\n";
    for (const auto &s : staffList) {
        out << s.username << ","
            << s.password << ","
            << s.role     << ","
            << s.id       << ","
            << s.name     << ","
            << s.age      << ","
            << s.gender   << ","
            << s.phone    << ","
            << s.status   << "\n";
    }
    file.close();
}

// +addStaff() -> Appends a new staff record
void StaffManager::addStaff(const StaffData &newStaff) {
    staffList.append(newStaff);
    saveAll();
}

// +removeStaff() -> Removes by staff ID and rewrites the database
void StaffManager::removeStaff(const QString &staffId) {
    for (int i = 0; i < staffList.size(); ++i) {
        if (staffList[i].id == staffId) {
            staffList.removeAt(i);
            saveAll();
            return;
        }
    }
    qDebug() << "StaffManager: Staff" << staffId << "not found for removal.";
}

// +searchStaff() -> Returns matched staff by ID
StaffData StaffManager::searchStaff(const QString &staffId) {
    for (const auto &s : staffList) {
        if (s.id == staffId) return s;
    }
    return StaffData();
}

bool StaffManager::updateStaff(const StaffData &updated) {
    for (auto &s : staffList) {
        if (s.id == updated.id) {
            s = updated;
            saveAll();
            return true;
        }
    }
    return false;
}

QVector<StaffData> StaffManager::getAllStaff() const {
    return staffList;
}

int StaffManager::getTotalCount() const {
    return staffList.size();
}





QDialog *createAddStaffDialog(QWidget *parent,
                               QLineEdit *&outUsernameEdit, QLineEdit *&outPasswordEdit,
                               QComboBox *&outRoleBox, QLineEdit *&outIdEdit,
                               QLineEdit *&outNameEdit, QLineEdit *&outAgeEdit,
                               QComboBox *&outGenderBox, QLineEdit *&outPhoneEdit,
                               QComboBox *&outStatusBox)
{
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("Add New Staff Member");
    dlg->setMinimumWidth(400);
    dlg->setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QLabel { font-weight: 600; color: #334155; font-size: 12px; border:none; background:transparent; }"
        "QLineEdit, QComboBox { padding: 7px 10px; border: 1px solid #cbd5e1; border-radius: 8px;"
        "  background: #f8fafc; color: #0f172a; font-size: 12px; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #0284c7; background: #fff; }"
        "QPushButton { padding: 8px 16px; font-weight: bold; border-radius: 8px; font-size: 12px; }"
    );

    QFormLayout *form = new QFormLayout(dlg);
    form->setContentsMargins(28, 28, 28, 28);
    form->setSpacing(14);

    // Credentials section header
    QLabel *credHeader = new QLabel("── Login Credentials ──", dlg);
    credHeader->setStyleSheet("color:#0284c7; font-size:11px; font-weight:700; border:none; background:transparent;");
    form->addRow(credHeader);

    QLineEdit *usernameEdit = new QLineEdit(dlg);
    usernameEdit->setPlaceholderText("e.g. drsmith");
    QLineEdit *passwordEdit = new QLineEdit(dlg);
    passwordEdit->setPlaceholderText("Set initial password");
    passwordEdit->setEchoMode(QLineEdit::Password);

    QComboBox *roleBox = new QComboBox(dlg);
    roleBox->addItems({"Admin", "Doctor", "Receptionist"});

    form->addRow("Username:",  usernameEdit);
    form->addRow("Password:",  passwordEdit);
    form->addRow("Role:",      roleBox);

    // Profile section header
    QLabel *profileHeader = new QLabel("── Staff Profile ──", dlg);
    profileHeader->setStyleSheet("color:#0284c7; font-size:11px; font-weight:700; border:none; background:transparent;");
    form->addRow(profileHeader);

    QLineEdit *idEdit = new QLineEdit(dlg);
    idEdit->setPlaceholderText("e.g. STF_004");
    QLineEdit *nameEdit = new QLineEdit(dlg);
    nameEdit->setPlaceholderText("Full Name");
    QLineEdit *ageEdit = new QLineEdit(dlg);
    ageEdit->setPlaceholderText("Age");
    QComboBox *genderBox = new QComboBox(dlg);
    genderBox->addItems({"Male", "Female", "Other"});
    QLineEdit *phoneEdit = new QLineEdit(dlg);
    phoneEdit->setPlaceholderText("Phone Number");
    QComboBox *statusBox = new QComboBox(dlg);
    statusBox->addItems({"On Duty", "On Leave"});

    form->addRow("Staff ID:",  idEdit);
    form->addRow("Full Name:", nameEdit);
    form->addRow("Age:",       ageEdit);
    form->addRow("Gender:",    genderBox);
    form->addRow("Phone:",     phoneEdit);
    form->addRow("Status:",    statusBox);

    // Buttons
    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *cancel = new QPushButton("Cancel", dlg);
    QPushButton *addBtn = new QPushButton("Add Staff", dlg);
    cancel->setStyleSheet("background:#f1f5f9;color:#475569;border:1px solid #e2e8f0;");
    addBtn->setStyleSheet("background:#0284c7;color:#ffffff;border:none;");
    btns->addStretch();
    btns->addWidget(cancel);
    btns->addWidget(addBtn);
    form->addRow(btns);

    QObject::connect(cancel, &QPushButton::clicked, dlg, &QDialog::reject);
    QObject::connect(addBtn, &QPushButton::clicked, dlg, [=]() {
        if (usernameEdit->text().trimmed().isEmpty() ||
            passwordEdit->text().trimmed().isEmpty() ||
            idEdit->text().trimmed().isEmpty()       ||
            nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(dlg, "Incomplete", "Username, Password, Staff ID, and Full Name are required.");
            return;
        }
        dlg->accept();
    });

    outUsernameEdit = usernameEdit;
    outPasswordEdit = passwordEdit;
    outRoleBox      = roleBox;
    outIdEdit       = idEdit;
    outNameEdit     = nameEdit;
    outAgeEdit      = ageEdit;
    outGenderBox    = genderBox;
    outPhoneEdit    = phoneEdit;
    outStatusBox    = statusBox;

    return dlg;
}

QDialog *createEditStaffDialog(const StaffData &s, QWidget *parent,
                                QLineEdit *&outUsernameEdit, QLineEdit *&outPasswordEdit,
                                QComboBox *&outRoleBox, QLineEdit *&outNameEdit,
                                QLineEdit *&outAgeEdit, QComboBox *&outGenderBox,
                                QLineEdit *&outPhoneEdit, QComboBox *&outStatusBox)
{
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("Edit Staff — " + s.id);
    dlg->setMinimumWidth(380);
    dlg->setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QLabel { font-weight: 600; color: #334155; font-size: 12px; border:none; background:transparent; }"
        "QLineEdit, QComboBox { padding: 7px 10px; border: 1px solid #cbd5e1; border-radius: 8px;"
        "  background: #f8fafc; color: #0f172a; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #0284c7; background: #fff; }"
        "QPushButton { padding: 8px 16px; font-weight: bold; border-radius: 8px; }"
    );

    QFormLayout *form = new QFormLayout(dlg);
    form->setContentsMargins(24, 24, 24, 24);
    form->setSpacing(12);

    QLineEdit *usernameEdit = new QLineEdit(s.username, dlg);
    QLineEdit *passwordEdit = new QLineEdit(s.password, dlg);
    passwordEdit->setEchoMode(QLineEdit::Password);
    QComboBox *roleBox      = new QComboBox(dlg);
    roleBox->addItems({"Admin", "Doctor", "Receptionist"});
    roleBox->setCurrentText(s.role);
    QLineEdit *nameEdit  = new QLineEdit(s.name,   dlg);
    QLineEdit *ageEdit   = new QLineEdit(s.age,    dlg);
    QComboBox *genderBox = new QComboBox(dlg);
    genderBox->addItems({"Male", "Female", "Other"});
    genderBox->setCurrentText(s.gender);
    QLineEdit *phoneEdit = new QLineEdit(s.phone,  dlg);
    QComboBox *statusBox = new QComboBox(dlg);
    statusBox->addItems({"On Duty", "On Leave"});
    statusBox->setCurrentText(s.status.isEmpty() ? "On Duty" : s.status);

    form->addRow("Username:", usernameEdit);
    form->addRow("Password:", passwordEdit);
    form->addRow("Role:",     roleBox);
    form->addRow("Name:",     nameEdit);
    form->addRow("Age:",      ageEdit);
    form->addRow("Gender:",   genderBox);
    form->addRow("Phone:",    phoneEdit);
    form->addRow("Status:",   statusBox);

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *cancel = new QPushButton("Cancel", dlg);
    QPushButton *save   = new QPushButton("Save", dlg);
    cancel->setStyleSheet("background:#f1f5f9;color:#475569;border:1px solid #e2e8f0;");
    save->setStyleSheet("background:#0284c7;color:#ffffff;border:none;");
    btns->addStretch();
    btns->addWidget(cancel);
    btns->addWidget(save);
    form->addRow(btns);

    QObject::connect(cancel, &QPushButton::clicked, dlg, &QDialog::reject);
    QObject::connect(save,   &QPushButton::clicked, dlg, &QDialog::accept);

    outUsernameEdit = usernameEdit;
    outPasswordEdit = passwordEdit;
    outRoleBox      = roleBox;
    outNameEdit     = nameEdit;
    outAgeEdit      = ageEdit;
    outGenderBox    = genderBox;
    outPhoneEdit    = phoneEdit;
    outStatusBox    = statusBox;

    return dlg;
}
