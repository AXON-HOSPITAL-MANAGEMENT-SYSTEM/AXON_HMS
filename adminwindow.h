#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include "admin.h"
#include "staffmanager.h"
#include "patientmanager.h"
#include "appointmentmanager.h"
#include "billingmanager.h"
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QTimer>
#include <QDateTime>


namespace Ui {
class AXON_ADMIN;
}

class adminwindow : public QWidget
{
    Q_OBJECT

public:
    explicit adminwindow(const QString &employeeName, QWidget *parent = nullptr);
    ~adminwindow();

signals:
    void logoutRequested();

private slots:
    void initDashboardGraphs();
    void updateDateTime();
    void on_btnMenu_clicked();
    void on_btnOverview_clicked();
    void on_btnStaffManager_clicked();
    void on_btnScheduling_clicked();
    void onAddStaffClicked();


private:
    Ui::AXON_ADMIN *ui;
    Admin          *adminBackend;
    StaffManager   *staffMgr;
    PatientManager *patientMgr;
    AppointmentManager *apptMgr;
    BillingManager *billMgr;        // NEW: backs the "Total Revenue (Today)" card
    QTimer         *timer;
    QString         currentAdminName;


    void setupPatientHeader();
    void loadPatientRowsFromBackend();
    void addPatientRow(const QString &id,     const QString &name,
                       const QString &gender, const QString &problem,
                       const QString &doctor, const QString &status);


    void setupStaffPage();
    void refreshStaffTable();
    void addStaffRow(const StaffData &s);
    void updateStaffCountLabel();


    QScrollArea *staffScrollArea   = nullptr;
    QWidget     *staffRowContainer = nullptr;
    QVBoxLayout *staffRowsLayout   = nullptr;
    QLabel      *staffCountLabel   = nullptr;


    void setupSchedulingPage();
    void refreshSchedulingTable();
    void addSchedulingRow(const Appointment &a);

    QWidget     *schedulingPage       = nullptr;
    QScrollArea *scheduleScrollArea   = nullptr;
    QWidget     *scheduleRowContainer = nullptr;
    QVBoxLayout *scheduleRowsLayout   = nullptr;
    QLabel      *scheduleCountLabel   = nullptr;
};

#endif // ADMINWINDOW_H
