#ifndef RECEPTIONISTWINDOW_H
#define RECEPTIONISTWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QTableWidget>
#include <QTimer>
#include <QPushButton>

#include "patientmanager.h"
#include "staffmanager.h"
#include "billingmanager.h"
#include "appointmentmanager.h"

namespace Ui {
class ReceptionistWindow;
}

class ReceptionistWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ReceptionistWindow(QWidget *parent = nullptr);
    ~ReceptionistWindow();

private slots:
    void onDashboardClicked();
    void onRegisterPatientClicked();
    void onScheduleClicked();
    void onBillingClicked();
    void onMenuClicked();

    // Patient Form Handlers
    void onSavePatientClicked();
    void onClearPatientFormClicked();

    // Schedule Handlers
    void onBookAppointmentClicked();
    void onClearScheduleFormClicked();

    // Billing Handlers
    void onCreateInvoiceClicked();
    void onClearBillingFormClicked();

    void updateDateTime();

private:
    Ui::ReceptionistWindow *ui;

    // Backend Managers
    PatientManager     *patientMgr{nullptr};
    StaffManager       *staffMgr{nullptr};
    BillingManager     *billingMgr{nullptr};
    AppointmentManager *apptMgr{nullptr};   // NEW: "Book Appointment" now creates real records here

    // UI Controls: Register Patient
    QLineEdit   *txtPatientName{nullptr};
    QSpinBox    *spnPatientAge{nullptr};
    QComboBox   *cmbPatientGender{nullptr};
    QLineEdit   *txtPatientContact{nullptr};
    QComboBox   *cmbBloodGroup{nullptr};
    QLineEdit   *txtReasonForVisit{nullptr};
    QComboBox   *cmbAssignedDoctor{nullptr};

    // UI Controls: Schedule Page
    QComboBox   *cmbSchedPatient{nullptr};
    QComboBox   *cmbSchedDoctor{nullptr};
    QDateEdit   *dtSchedDate{nullptr};
    QTimeEdit   *tmSchedTime{nullptr};
    QLineEdit   *txtSchedReason{nullptr};
    QTableWidget *scheduleTable{nullptr};

    // UI Controls: Billing Page
    QComboBox      *cmbBillPatient{nullptr};
    QComboBox      *cmbBillType{nullptr};
    QDoubleSpinBox *spnBillAmount{nullptr};
    QComboBox      *cmbBillStatus{nullptr};
    QTableWidget   *billingTable{nullptr};

    // Dashboard UI
    QTableWidget *queueTable{nullptr};
    QTimer       *dateTimeTimer{nullptr};

    // Initialization & Setup Helpers
    void setupCardStyles();
    void setupDashboardBottomArea();
    void setupRegisterPatientPage();
    void setupSchedulePage();
    void setupBillingPage();
    void setupConnections();

    // Refresh & Helper Functions
    void refreshDashboardStats();
    void refreshScheduleTable();
    void refreshBillingTable();
    void populatePatientDropdowns();
    void populateDoctorDropdowns();   // NEW: pulls real doctor names from StaffManager
    void updateSidebarSelection(QPushButton *activeBtn);
    void openEditPatientDialog(const QString &patientId);
};

#endif // RECEPTIONISTWINDOW_H
