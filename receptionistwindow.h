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
#include <QRadioButton>
#include <QButtonGroup>
#include <QTextEdit>
#include <QLabel>
#include <QStackedWidget>
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
    void onSearchBillingPatientClicked();   // NEW: looks up patient + loads their open bill
    void onPaymentModeChanged();            // NEW: switches the stacked widget page
    void updateChangeDue();                 // NEW: recomputes Change Due on the Cash page
    void onGenerateBillClicked();           // NEW: replaces onCreateInvoiceClicked()
    void onProcessPaymentClicked();         // NEW: collects payment on the loaded bill
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
    // UI Controls: Billing Page — Patient Lookup / Information
    QLineEdit   *txtBillSearch{nullptr};
    QPushButton *btnBillSearch{nullptr};
    QLabel      *lblBillPatientId{nullptr};
    QLabel      *lblBillPatientName{nullptr};
    QLabel      *lblBillPatientAge{nullptr};
    QLabel      *lblBillPatientGender{nullptr};
    // UI Controls: Billing Page — Current Bill Summary
    QTableWidget *tblCurrentBillSummary{nullptr};
    // UI Controls: Billing Page — Payment Details
    QRadioButton  *radCash{nullptr};
    QRadioButton  *radCard{nullptr};
    QRadioButton  *radOnline{nullptr};
    QRadioButton  *radInsurance{nullptr};
    QButtonGroup  *paymentModeGroup{nullptr};
    QDoubleSpinBox *spnAmountToPay{nullptr};
    QDoubleSpinBox *spnDiscount{nullptr};
    QTextEdit      *txtBillNotes{nullptr};
    QPushButton    *btnGenerateBill{nullptr};
    QPushButton    *btnProcessPayment{nullptr};
    // UI Controls: Billing Page — Payment Details — mode-specific stacked pages
    QStackedWidget *stackedPaymentDetails{nullptr};
    // Cash page
    QWidget        *pageCash{nullptr};
    QDoubleSpinBox *spnCashReceived{nullptr};
    QLabel         *lblChangeDue{nullptr};
    // Card page
    QWidget     *pageCard{nullptr};
    QComboBox   *cmbCardType{nullptr};
    QLineEdit   *txtCardLast4{nullptr};
    QLineEdit   *txtCardAuthCode{nullptr};
    // Online page
    QWidget     *pageOnline{nullptr};
    QComboBox   *cmbOnlineMethod{nullptr};
    QLineEdit   *txtOnlineTxnId{nullptr};
    QLineEdit   *txtOnlinePayerRef{nullptr};
    // Insurance page
    QWidget        *pageInsurance{nullptr};
    QComboBox      *cmbInsuranceProvider{nullptr};
    QLineEdit      *txtPolicyMemberId{nullptr};
    QLineEdit      *txtPreAuthCode{nullptr};
    QDoubleSpinBox *spnCoveredAmount{nullptr};
    // Tracks which patient/bill the billing page is currently working with
    QString currentLookupPatientId;
    QString currentGeneratedBillId;
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
    void populatePatientDropdowns();
    void populateDoctorDropdowns();   // NEW: pulls real doctor names from StaffManager
    void updateSidebarSelection(QPushButton *activeBtn);
    void openEditPatientDialog(const QString &patientId);
};
#endif // RECEPTIONISTWINDOW_H
