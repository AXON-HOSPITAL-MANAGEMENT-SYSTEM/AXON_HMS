#ifndef PATIENT_H
#define PATIENT_H

#include <QString>

struct Patient {
    QString id;
    QString name;
    QString age;
    QString gender;
    QString contact;
    QString bloodGroup;
    QString diagnosisTreatment; // Used for "Reason for Visit"
    QString assignedDoctor;     // Used for "Assigned Doctor"
    QString status;
    QString bedNumber;
};

#endif // PATIENT_H