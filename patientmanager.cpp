#include "patientmanager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

PatientManager::PatientManager() {
    loadAll();
}

void PatientManager::reload() {
    loadAll();
}

// Loads all patient records from the CSV database into memory
// FIX: was reading "database/patient_database.csv" — a subfolder no other
// part of the app used, so Admin/Doctor/Receptionist all ended up creating
// and reading DIFFERENT files depending on the working directory. Now reads
// the same root-level file everywhere.
void PatientManager::loadAll() {
    patientList.clear();

    QFile file("patient_database.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Fallback: try the embedded resource (read-only, used as seed)
        QFile res(":/database/patient_database.csv");
        if (!res.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "PatientManager: patient_database.csv not found anywhere.";
            return;
        }
        QTextStream in(&res);
        _parseStream(in);
        res.close();
        saveAll();
        return;
    }

    QTextStream in(&file);
    _parseStream(in);
    file.close();
}

// FIX: contact and bloodGroup were declared on the Patient struct and
// captured by the Receptionist's registration form, but this parser only
// ever read 8 fields, so both values were silently discarded on every
// save/reload cycle. Now supports the full 10-column schema:
//   id,name,age,gender,contact,blood_group,diagnosis,doctor,status,bed
// Legacy 8-column rows (from before this fix) still load fine — contact and
// bloodGroup just come back empty for those rows.
void PatientManager::_parseStream(QTextStream &in) {
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        QStringList fields = line.split(",");

        if (fields.size() >= 10) {
            Patient p;
            p.id                 = fields[0].trimmed();
            p.name               = fields[1].trimmed();
            p.age                = fields[2].trimmed();
            p.gender             = fields[3].trimmed();
            p.contact            = fields[4].trimmed();
            p.bloodGroup         = fields[5].trimmed();
            p.diagnosisTreatment = fields[6].trimmed();
            p.assignedDoctor     = fields[7].trimmed();
            p.status             = fields[8].trimmed();
            p.bedNumber          = fields[9].trimmed();
            patientList.append(p);
        } else if (fields.size() >= 8) {
            // Legacy rows without contact/blood group columns
            Patient p;
            p.id                 = fields[0].trimmed();
            p.name               = fields[1].trimmed();
            p.age                = fields[2].trimmed();
            p.gender             = fields[3].trimmed();
            p.diagnosisTreatment = fields[4].trimmed();
            p.assignedDoctor     = fields[5].trimmed();
            p.status             = fields[6].trimmed();
            p.bedNumber          = fields[7].trimmed();
            patientList.append(p);
        }
    }
}

// Rewrites the entire CSV from the in-memory vector (used after any mutation)
void PatientManager::saveAll() {
    QFile file("patient_database.csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "PatientManager: Cannot open patient_database.csv for writing.";
        return;
    }

    QTextStream out(&file);
    out << "# patient_id, name, age, gender, contact, blood_group, diagnosis/treatment, assigned_doctor, status, bed_number\n";
    for (const auto &p : patientList) {
        out << p.id << ","
            << p.name << ","
            << p.age << ","
            << p.gender << ","
            << p.contact << ","
            << p.bloodGroup << ","
            << p.diagnosisTreatment << ","
            << p.assignedDoctor << ","
            << p.status << ","
            << p.bedNumber << "\n";
    }
    file.close();
}

// +addPatient() -> Appends a new patient record to the file database
void PatientManager::addPatient(const Patient &newPatient) {
    patientList.append(newPatient);

    QFile file("patient_database.csv");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << newPatient.id << ","
            << newPatient.name << ","
            << newPatient.age << ","
            << newPatient.gender << ","
            << newPatient.contact << ","
            << newPatient.bloodGroup << ","
            << newPatient.diagnosisTreatment << ","
            << newPatient.assignedDoctor << ","
            << newPatient.status << ","
            << newPatient.bedNumber << "\n";
        file.close();
    }
}

// +removePatient() -> Deletes a record from memory and rewrites the database
void PatientManager::removePatient(const QString &patientId) {
    for (int i = 0; i < patientList.size(); ++i) {
        if (patientList[i].id == patientId) {
            patientList.removeAt(i);
            saveAll();
            return;
        }
    }
    qDebug() << "PatientManager: Patient" << patientId << "not found for removal.";
}

// +searchPatient() -> Returns the matched patient object by ID
Patient PatientManager::searchPatient(const QString &patientId) {
    for (const auto &p : patientList) {
        if (p.id == patientId) {
            return p;
        }
    }
    return Patient(); // Returns empty struct if not found
}

// +getPatientById() -> Identical helper method for lookup
Patient PatientManager::getPatientById(const QString &patientId) {
    return searchPatient(patientId);
}

// +updatePatient() -> Finds patient by ID, updates fields, and persists changes
void PatientManager::updatePatient(const Patient &updatedPatient) {
    for (int i = 0; i < patientList.size(); ++i) {
        if (patientList[i].id == updatedPatient.id) {
            patientList[i] = updatedPatient;
            saveAll();
            return;
        }
    }
    qDebug() << "PatientManager: Patient" << updatedPatient.id << "not found for update.";
}

// +diagnose() -> Updates diagnosis field and persists
void PatientManager::diagnose(const QString &patientId, const QString &diagnosis) {
    for (auto &p : patientList) {
        if (p.id == patientId) {
            p.diagnosisTreatment = diagnosis;
            saveAll();
            return;
        }
    }
}

// +treat() -> Updates treatment info (appends to diagnosis field) and persists
void PatientManager::treat(const QString &patientId, const QString &treatment) {
    for (auto &p : patientList) {
        if (p.id == patientId) {
            p.diagnosisTreatment = treatment;
            saveAll();
            return;
        }
    }
}

// +discharge() -> Sets patient status to Discharged and persists
void PatientManager::discharge(const QString &patientId) {
    for (auto &p : patientList) {
        if (p.id == patientId) {
            p.status = "Discharged";
            saveAll();
            return;
        }
    }
}

QVector<Patient> PatientManager::getAllPatients() const {
    return patientList;
}

int PatientManager::getTotalCount() const {
    return patientList.size();
}

void PatientManager::setStatus(const QString &patientId, const QString &newStatus) {
    for (auto &p : patientList) {
        if (p.id == patientId) {
            p.status = newStatus;
            saveAll();
            return;
        }
    }
}

// Generates the next sequential, collision-free Patient ID (PT-0001, PT-0002, ...)
QString PatientManager::generateNextId() const {
    int n = patientList.size() + 1;
    QString candidate;
    bool exists;
    do {
        candidate = QString("PT-%1").arg(n, 4, 10, QChar('0'));
        exists = false;
        for (const auto &p : patientList) {
            if (p.id == candidate) { exists = true; break; }
        }
        ++n;
    } while (exists);
    return candidate;
}

QDialog *createEditPatientDialog(const QString &id, const QString &name, const QString &gender,
                                 const QString &problem, const QString &doctor, const QString &status,
                                 QWidget *parent,
                                 QLineEdit *&outNameEdit, QComboBox *&outGenderBox,
                                 QLineEdit *&outProblemEdit, QLineEdit *&outDoctorEdit,
                                 QComboBox *&outStatusBox)
{
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("Modify Patient Record — " + id);
    dlg->setMinimumWidth(380);
    dlg->setStyleSheet(
        "QDialog { background-color: #ffffff; border-radius: 8px; }"
        "QLabel { font-weight: bold; color: #334155; font-size: 12px; border:none; background:transparent; }"
        "QLineEdit, QComboBox { padding: 6px; border: 1px solid #cbd5e1; border-radius: 6px;"
        "  background: #f8fafc; color: #0f172a; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #6366f1; background: #ffffff; }"
        "QPushButton { padding: 6px 14px; font-weight: bold; border-radius: 6px; font-size: 12px; }"
        );

    QFormLayout *form = new QFormLayout(dlg);
    form->setContentsMargins(24, 24, 24, 24);
    form->setSpacing(14);

    QLineEdit *nameEdit    = new QLineEdit(name, dlg);
    QComboBox *genderBox   = new QComboBox(dlg);
    genderBox->addItems({"Male", "Female", "Other"});
    genderBox->setCurrentText(gender);
    QLineEdit *problemEdit = new QLineEdit(problem, dlg);
    QLineEdit *doctorEdit  = new QLineEdit(doctor, dlg);
    QComboBox *statusBox   = new QComboBox(dlg);
    statusBox->addItems({"Admitted", "Discharged", "Checked In", "No Show", "Completed"});
    statusBox->setCurrentText(status.trimmed());

    form->addRow("Patient Name:",       nameEdit);
    form->addRow("Gender:",             genderBox);
    form->addRow("Diagnosis/Problem:", problemEdit);
    form->addRow("Assigned Doctor:",    doctorEdit);
    form->addRow("Status:",             statusBox);

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *cancel = new QPushButton("Cancel", dlg);
    QPushButton *save   = new QPushButton("Save Changes", dlg);
    cancel->setStyleSheet("background-color:#f1f5f9;color:#475569;border:1px solid #e2e8f0;");
    save->setStyleSheet("background-color:#0284c7;color:#ffffff;border:none;");
    btns->addStretch();
    btns->addWidget(cancel);
    btns->addWidget(save);
    form->addRow(btns);

    QObject::connect(cancel, &QPushButton::clicked, dlg, &QDialog::reject);
    QObject::connect(save,   &QPushButton::clicked, dlg, &QDialog::accept);

    outNameEdit    = nameEdit;
    outGenderBox   = genderBox;
    outProblemEdit = problemEdit;
    outDoctorEdit  = doctorEdit;
    outStatusBox   = statusBox;

    return dlg;
}
