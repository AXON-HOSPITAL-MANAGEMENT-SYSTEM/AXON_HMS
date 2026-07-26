#ifndef STAFFMANAGER_H
#define STAFFMANAGER_H
#include <QString>
#include <QVector>
#include <QTextStream>
#include <QDialog>
class QWidget;
class QLineEdit;
class QComboBox;
// CSV format: username,password,Role,staff_id,full_name,age,gender,phone
struct StaffData {
    QString username;
    QString password;
    QString role;
    QString id;
    QString name;
    QString age;
    QString gender;
    QString phone;
    QString status;
};
class StaffManager
{
public:
    StaffManager();
    // Methods from the class diagram
    void addStaff(const StaffData &newStaff);
    void removeStaff(const QString &staffId);
    StaffData searchStaff(const QString &staffId);
    // Extra utilities needed by the Admin window
    QVector<StaffData> getAllStaff() const;
    bool updateStaff(const StaffData &updated);
    // NEW: overload used when the Staff ID itself may have changed in the
    // Edit dialog — looks the record up by its ORIGINAL id, then overwrites
    // the whole record (including the new id) and persists.
    bool updateStaff(const QString &oldStaffId, const StaffData &updated);
    int  getTotalCount() const;
    // Re-reads staff_database.csv from disk (e.g. so Receptionist's
    // "Assign Doctor" dropdown reflects doctors Admin has just added).
    void reload();
private:
    QVector<StaffData> staffList;
    void saveAll();
    void loadAll();
    void _parseStream(QTextStream &in);
};
// ── Dialog factories (replace AddStaffDialog / EditStaffDialog classes) ────
// Each builds a plain QDialog and returns it. Editable widgets are handed
// back via reference out-parameters so the caller can read values after
// dlg->exec() == QDialog::Accepted. The "Add Staff" button's own validation
// (required fields) is wired up inside the factory, same as before.
QDialog *createAddStaffDialog(QWidget *parent,
                              QLineEdit *&outUsernameEdit, QLineEdit *&outPasswordEdit,
                              QComboBox *&outRoleBox, QLineEdit *&outIdEdit,
                              QLineEdit *&outNameEdit, QLineEdit *&outAgeEdit,
                              QComboBox *&outGenderBox, QLineEdit *&outPhoneEdit,
                              QComboBox *&outStatusBox);

// NEW: outIdEdit added — Staff ID is now editable from the Edit dialog too.
QDialog *createEditStaffDialog(const StaffData &s, QWidget *parent,
                               QLineEdit *&outUsernameEdit, QLineEdit *&outPasswordEdit,
                               QComboBox *&outRoleBox, QLineEdit *&outIdEdit,
                               QLineEdit *&outNameEdit, QLineEdit *&outAgeEdit,
                               QComboBox *&outGenderBox, QLineEdit *&outPhoneEdit,
                               QComboBox *&outStatusBox);
#endif // STAFFMANAGER_H