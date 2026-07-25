#ifndef PATIENTMANAGER_H
#define PATIENTMANAGER_H

#include <QVector>
#include <QString>
#include "patient.h"
#include <QTextStream>
#include <QDialog>

class QWidget;
class QLineEdit;
class QComboBox;

class PatientManager
{
public:
    PatientManager();

    void addPatient(const Patient &newPatient);
    void removePatient(const QString &patientId);
    Patient searchPatient(const QString &patientId);

    // --- ADDED MISSING LOOKUP & UPDATE METHODS ---
    Patient getPatientById(const QString &patientId);
    void updatePatient(const Patient &updatedPatient);

    void diagnose(const QString &patientId, const QString &diagnosis);
    void treat(const QString &patientId, const QString &treatment);
    void discharge(const QString &patientId);

    QVector<Patient> getAllPatients() const;
    int getTotalCount() const;

    QString generateNextId() const;

    void reload();

    void setStatus(const QString &patientId, const QString &newStatus);

private:
    QVector<Patient> patientList;
    void saveAll(); // rewrites patient_database.csv from patientList
    void loadAll(); // reads patient_database.csv into patientList
    void _parseStream(QTextStream &in);
};

// Builds a "Modify Patient Record" dialog and hands back pointers to the
// editable fields via the out-parameters so the caller can read the values
// once dlg->exec() == QDialog::Accepted. Caller owns / deletes the dialog
// (or uses a stack-allocated QDialog and passes &dlg as parent owner).
QDialog *createEditPatientDialog(const QString &id, const QString &name, const QString &gender,
                                 const QString &problem, const QString &doctor, const QString &status,
                                 QWidget *parent,
                                 QLineEdit *&outNameEdit, QComboBox *&outGenderBox,
                                 QLineEdit *&outProblemEdit, QLineEdit *&outDoctorEdit,
                                 QComboBox *&outStatusBox);

#endif // PATIENTMANAGER_H
