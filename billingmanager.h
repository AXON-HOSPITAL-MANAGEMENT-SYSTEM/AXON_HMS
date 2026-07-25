#ifndef BILLINGMANAGER_H
#define BILLINGMANAGER_H

#include <QString>
#include <QVector>
#include <QTextStream>

// One line item on a bill (a row in the "Current Bill Summary" table).
// Stored inside a BillingRecord's serialized_items column as
// "serviceCode:description:amount", multiple items joined with "|".
struct BillItem {
    QString serviceCode;
    QString description;
    double  amount = 0.0;
};

// One row of billing_database.csv.
//
// Model used by generateBill()/processPayment():
//   subtotal          = sum of item amounts, fixed at the time the bill is generated
//   depositPaid       = deposit collected at generation time (0 if none)
//   discount          = discount applied at generation time
//   remainingBalance  = subtotal - depositPaid - discount, then reduced by
//                       every processPayment() call
//   amountToPay       = running total actually PAID so far via processPayment()
//                       (despite the name inherited from the CSV column, this
//                       is cumulative amount paid, not a target amount)
struct BillingRecord {
    QString billId;
    QString patientId;
    QString date;
    double  subtotal = 0.0;
    double  depositPaid = 0.0;
    double  discount = 0.0;
    double  amountToPay = 0.0;
    double  remainingBalance = 0.0;
    QString paymentMode;
    QVector<BillItem> items;
    QString notes;
};

// Handles billing for a patient's stay. Referenced by Receptionist and PatientManager (dependency arrows on the diagram). Backed by
// billing_database.csv — the single shared billing database file.
class BillingManager
{
public:
    BillingManager();

    // Diagram-matching methods
    // Creates a brand-new bill for a patient from a set of line items,
    // an optional deposit collected up front, and a discount. Returns the
    // new bill's ID.
    QString generateBill(const QString &patientId, const QVector<BillItem> &items,
                         double depositPaid, double discount, const QString &notes);

    // Applies a payment against an existing bill: increases the cumulative
    // amountToPay, decreases remainingBalance (never below zero), and
    // records the payment mode used. extraNotes (optional) is appended to
    // the bill's notes log rather than overwriting it.
    bool processPayment(const QString &billId, double amountPaid,
                        const QString &paymentMode, const QString &extraNotes = QString());

    // Queries
    BillingRecord searchBill(const QString &billId) const;
    BillingRecord getLatestBillForPatient(const QString &patientId) const;
    QVector<BillingRecord> getAllBillsForPatient(const QString &patientId) const;
    QVector<BillingRecord> getAllBills() const;

    // Re-reads billing_database.csv from disk.
    void reload();

private:
    QVector<BillingRecord> billList;
    void loadAll();
    void saveAll();
    void _parseStream(QTextStream &in);
    QString generateNextId() const;

    static QString serializeItems(const QVector<BillItem> &items);
    static QVector<BillItem> deserializeItems(const QString &s);
};

#endif // BILLINGMANAGER_H
