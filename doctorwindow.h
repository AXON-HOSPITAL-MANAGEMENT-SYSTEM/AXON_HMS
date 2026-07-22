#ifndef DOCTORWINDOW_H
#define DOCTORWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QTableWidget>
#include "doctor.h"
#include "patientmanager.h"
#include "appointmentmanager.h"

namespace Ui
{
class doctorwindow;
};

class doctorwindow : public QWidget
{
    Q_OBJECT

public:
    explicit doctorwindow(const QString &employeeName, QWidget *parent = nullptr);
    ~doctorwindow();

private slots:
    void updateDateTime();
    void switchPage();
    void logout();
    void setupOverviewLayout();
    void setupStatsSection();
    void populatePatientList();
    void populateAppointmentsTable();
    void viewPatientDetails(const QString &patientId);

    //Treatment functions
    void openTreatmentDialog(const QString &patientId);


private:
    Ui::doctorwindow *ui;
    Doctor *doctorBackend;
    QString currentUserName;
    QTimer *timer;

    PatientManager     *patientMgr;
    AppointmentManager *apptMgr;

    // Patient List page
    void setupPatientListPage();
    void refreshPatientList();
    void addPatientRow(const Patient &p);
    void addPatientRowWithTreat(const Patient &p);    //For patient list with Treat button
    void refreshPatientTable(QTableWidget *table);

    QVBoxLayout *patientRowsLayout = nullptr;
    QWidget     *patientRowContainer = nullptr;
    QLabel      *patientCountLabel = nullptr;

    // Scheduling page
    void setupSchedulingPage();
    void refreshSchedulingTable();
    void addScheduleRow(const Appointment &a);


    QVBoxLayout *scheduleRowsLayout = nullptr;
    QWidget     *scheduleRowContainer = nullptr;
    QLabel      *scheduleCountLabel = nullptr;
};

#endif // DOCTORWINDOW_H