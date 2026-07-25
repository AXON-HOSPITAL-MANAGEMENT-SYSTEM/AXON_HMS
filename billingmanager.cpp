#include "billingmanager.h"
#include <QFile>
#include <QDebug>
#include <QDate>
#include <algorithm>

BillingManager::BillingManager() {
    loadAll();
}

void BillingManager::reload() {
    loadAll();
}

// Loads all billing records from the CSV database into memory
void BillingManager::loadAll() {
    billList.clear();

    QFile file("billing_database.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Fallback: try the embedded resource (read-only, used as seed)
        QFile res(":/database/billing_database.csv");
        if (!res.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "BillingManager: billing_database.csv not found anywhere.";
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

void BillingManager::_parseStream(QTextStream &in) {
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        QStringList fields = line.split(",");
        if (fields.size() >= 11) {
            BillingRecord b;
            b.billId           = fields[0].trimmed();
            b.patientId        = fields[1].trimmed();
            b.date              = fields[2].trimmed();
            b.subtotal          = fields[3].trimmed().toDouble();
            b.depositPaid       = fields[4].trimmed().toDouble();
            b.discount          = fields[5].trimmed().toDouble();
            b.amountToPay       = fields[6].trimmed().toDouble();
            b.remainingBalance  = fields[7].trimmed().toDouble();
            b.paymentMode       = fields[8].trimmed();
            b.items             = deserializeItems(fields[9].trimmed());
            // Notes is always the final column; rejoin defensively in case
            // it ever ends up containing a comma despite sanitization.
            b.notes = fields.mid(10).join(",").trimmed();
            billList.append(b);
        }
    }
}

// Rewrites the entire CSV from the in-memory vector
void BillingManager::saveAll() {
    QFile file("billing_database.csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "BillingManager: Cannot open billing_database.csv for writing.";
        return;
    }

    QTextStream out(&file);
    out << "# bill_id,patient_id,date,subtotal,deposit_paid,discount,amount_to_pay,remaining_balance,payment_mode,serialized_items,notes\n";
    for (const auto &b : billList) {
        out << b.billId << ","
            << b.patientId << ","
            << b.date << ","
            << QString::number(b.subtotal, 'f', 2) << ","
            << QString::number(b.depositPaid, 'f', 2) << ","
            << QString::number(b.discount, 'f', 2) << ","
            << QString::number(b.amountToPay, 'f', 2) << ","
            << QString::number(b.remainingBalance, 'f', 2) << ","
            << b.paymentMode << ","
            << serializeItems(b.items) << ","
            << b.notes << "\n";
    }
    file.close();
}

QString BillingManager::serializeItems(const QVector<BillItem> &items) {
    QStringList parts;
    for (const auto &it : items) {
        QString code = it.serviceCode;
        code.replace(":", "-").replace("|", "-").replace(",", " ");
        QString desc = it.description;
        desc.replace(":", "-").replace("|", "-").replace(",", " ");
        parts << QString("%1:%2:%3").arg(code, desc, QString::number(it.amount, 'f', 2));
    }
    return parts.join("|");
}

QVector<BillItem> BillingManager::deserializeItems(const QString &s) {
    QVector<BillItem> items;
    if (s.trimmed().isEmpty()) return items;

    const QStringList parts = s.split("|", Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        QStringList f = p.split(":");
        if (f.size() >= 3) {
            BillItem it;
            it.serviceCode = f[0].trimmed();
            it.description = f[1].trimmed();
            it.amount      = f[2].trimmed().toDouble();
            items.append(it);
        }
    }
    return items;
}

QString BillingManager::generateNextId() const {
    int n = billList.size() + 1001;
    QString candidate;
    bool exists;
    do {
        candidate = QString("B-%1").arg(n);
        exists = false;
        for (const auto &b : billList) {
            if (b.billId == candidate) { exists = true; break; }
        }
        ++n;
    } while (exists);
    return candidate;
}

// +generateBill() -> Creates a brand-new bill record and persists it
QString BillingManager::generateBill(const QString &patientId, const QVector<BillItem> &items,
                                     double depositPaid, double discount, const QString &notes)
{
    double subtotal = 0.0;
    for (const auto &it : items) subtotal += it.amount;

    double remaining = subtotal - depositPaid - discount;
    if (remaining < 0.0) remaining = 0.0;

    BillingRecord b;
    b.billId           = generateNextId();
    b.patientId        = patientId;
    b.date              = QDate::currentDate().toString("yyyy-MM-dd");
    b.subtotal          = subtotal;
    b.depositPaid       = depositPaid;
    b.discount          = discount;
    b.amountToPay       = 0.0; // nothing paid yet — payments come via processPayment()
    b.remainingBalance  = remaining;
    b.paymentMode       = "";
    b.items             = items;

    QString cleanNotes = notes;
    cleanNotes.replace(",", " ");
    b.notes = cleanNotes;

    billList.append(b);
    saveAll();
    return b.billId;
}

// +processPayment() -> Applies a payment to an existing bill and persists
bool BillingManager::processPayment(const QString &billId, double amountPaid,
                                    const QString &paymentMode, const QString &extraNotes)
{
    for (auto &b : billList) {
        if (b.billId == billId) {
            b.amountToPay      += amountPaid;
            b.remainingBalance  = std::max(0.0, b.remainingBalance - amountPaid);
            b.paymentMode       = paymentMode;

            QString clean = extraNotes;
            clean.replace(",", " ");
            if (!clean.trimmed().isEmpty()) {
                if (!b.notes.trimmed().isEmpty()) b.notes += " | ";
                b.notes += clean.trimmed();
            }

            saveAll();
            return true;
        }
    }
    qDebug() << "BillingManager: Bill" << billId << "not found for payment.";
    return false;
}

BillingRecord BillingManager::searchBill(const QString &billId) const {
    for (const auto &b : billList) {
        if (b.billId == billId) return b;
    }
    return BillingRecord();
}

// Returns the most recently generated bill for a patient (bills are
// appended chronologically, so the last match is the most recent).
BillingRecord BillingManager::getLatestBillForPatient(const QString &patientId) const {
    BillingRecord latest;
    for (const auto &b : billList) {
        if (b.patientId.trimmed().compare(patientId.trimmed(), Qt::CaseInsensitive) == 0)
            latest = b;
    }
    return latest;
}

QVector<BillingRecord> BillingManager::getAllBillsForPatient(const QString &patientId) const {
    QVector<BillingRecord> out;
    for (const auto &b : billList) {
        if (b.patientId.trimmed().compare(patientId.trimmed(), Qt::CaseInsensitive) == 0)
            out.append(b);
    }
    return out;
}

QVector<BillingRecord> BillingManager::getAllBills() const {
    return billList;
}
