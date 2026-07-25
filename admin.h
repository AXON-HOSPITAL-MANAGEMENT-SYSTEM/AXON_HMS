#ifndef ADMIN_H
#define ADMIN_H

#include <QString>
#include "staffmanager.h"

class Admin
{
public:
    Admin();
    ~Admin();


    void runMainInterface();


    void manageStaff();
    void getOverview();
    void scheduling();

private:

    QString id;
    QString name;
    QString age;
    QString gender;
    QString phone;


    StaffManager *staffMgr;
};

#endif // ADMIN_H
