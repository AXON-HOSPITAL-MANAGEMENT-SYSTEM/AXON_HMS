#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "doctorwindow.h"
#include "adminwindow.h"
#include "receptionistwindow.h"
#include "staffmanager.h"
#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>



#include "adminwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Enter key navigation updates
    connect(ui->userInput, &QLineEdit::returnPressed, this, [this]() {
        ui->passInput->setFocus();
    });
    connect(ui->passInput, &QLineEdit::returnPressed, this, &MainWindow::handleLogin);
    connect(ui->loginButton, &QPushButton::clicked, this, &MainWindow::handleLogin);

    // Password visibility action setup
    eyeAction = new QAction(this);
    eyeAction->setIcon(QIcon(":/images/ihide.png"));
    ui->passInput->addAction(eyeAction, QLineEdit::TrailingPosition);
    connect(eyeAction, &QAction::triggered, this, &MainWindow::togglePasswordVisibility);

    //rolecombobox dropdown arrow
    ui->roleComboBox->setStyleSheet(
        "QComboBox {"
        "    background-color: white;"
        "    color: black;"
        "    padding-left: 5px;"
        "    padding-right: 25px;"
        "}"
        "QComboBox::drop-down {"
        "    subcontrol-position: top right;"
        "    width: 25px;"
        "    border: none;"
        "}"
        /* Default state: Pointing Right (Scaled Down) */
        "QComboBox::down-arrow {"
        "    image: url(:/images/right-arrow-icon.png);"
        "    width: 12px;"  /* Adjust this value to make it smaller or larger */
        "    height: 12px;" /* Keep width and height equal to maintain aspect ratio */
        "}"
        /* Clicked/Open state: Pointing Down (Scaled Down) */
        "QComboBox::down-arrow:on {"
        "    image: url(:/images/down-arrow-icon.png);"
        "    width: 12px;"  /* Make sure this matches the width above */
        "    height: 12px;" /* Make sure this matches the height above */
        "}"

        "QAbstractItemView {"
        "    background-color: white;"
        "    color: black;"
        "    selection-background-color: #e0e0e0;"
        "    selection-color: black;"
        "}"
        );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::togglePasswordVisibility()
{
    if (ui->passInput->echoMode() == QLineEdit::Password) {
        ui->passInput->setEchoMode(QLineEdit::Normal);
        eyeAction->setIcon(QIcon(":/images/ishow.png"));
    } else {
        ui->passInput->setEchoMode(QLineEdit::Password);
        eyeAction->setIcon(QIcon(":/images/ihide.png"));
    }
}

void MainWindow::handleLogin() {
    qDebug() << "Handling login via handleLogin()... Redirecting to button logic.";

    // Call the button click slot directly
    on_loginButton_clicked();
}

// FIX: previously this hand-parsed "staff_database.csv" directly with its
// own QFile/QTextStream logic, while StaffManager (used by AdminWindow to
// add/edit staff) read/wrote a different path ("database/staff_database.csv").
// Any staff Admin added or edited was therefore invisible here. Login now
// goes through the SAME StaffManager class, so it always sees current data.
void MainWindow::on_loginButton_clicked()
{
    QString enteredUser = ui->userInput->text().trimmed();
    QString enteredPass = ui->passInput->text().trimmed();
    QString selectedRole = ui->roleComboBox->currentText().trimmed();

    if (enteredUser.isEmpty() || enteredPass.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please fill in all fields.");
        return;
    }

    StaffManager staffMgr;   // reads the SAME staff_database.csv Admin writes to
    bool authenticated = false;

    for (const StaffData &s : staffMgr.getAllStaff()) {
        if (enteredUser == s.username &&
            enteredPass == s.password &&
            selectedRole.compare(s.role, Qt::CaseInsensitive) == 0) {

            authenticated = true;
            QString fullName = s.name.isEmpty() ? s.username : s.name;

            if (selectedRole.compare("Admin", Qt::CaseInsensitive) == 0) {
                adminwindow *adminWin = new adminwindow(fullName);
                adminWin->setAttribute(Qt::WA_DeleteOnClose);
                adminWin->show();
                this->hide();
            } else if (selectedRole.compare("Doctor", Qt::CaseInsensitive) == 0) {
                qDebug() << "Launching Doctor Window for: " << fullName;
                doctorwindow *doctorWin = new doctorwindow(fullName);
                doctorWin->setAttribute(Qt::WA_DeleteOnClose);
                doctorWin->show();
                this->hide();
            } else if (selectedRole.compare("Receptionist", Qt::CaseInsensitive) == 0) {
                qDebug() << "Launching Receptionist Window...";
                ReceptionistWindow *receptionistWin = new ReceptionistWindow();
                receptionistWin->setAttribute(Qt::WA_DeleteOnClose);
                receptionistWin->show();
                this->hide();
            }

            break; // Found the matching staff member — stop searching
        }
    }

    // If no matching record was found, show the failure message
    if (!authenticated) {
        QMessageBox::warning(this, "Login Failed",
                             "Invalid username, password, or role.");
    }
}
