#ifndef CANPADDLE_H
#define CANPADDLE_H

#include <iostream>
#include <exception>
#include <memory>
#include <sstream>

#include <QHeaderView>
#include <QMainWindow>
#include <QTableWidget>
#include <QTreeWidget>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "dbcreader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CanPaddle; }
QT_END_NAMESPACE

class CanPaddle : public QMainWindow
{
    Q_OBJECT

public:
    CanPaddle(QWidget *parent = nullptr);
    ~CanPaddle();

public:
    enum treeItemType
    {
        typeRoot = QTreeWidgetItem::ItemType::Type,
        typeNetwork = QTreeWidgetItem::ItemType::UserType + 1,
        typeEcu,
        typeNode,
        typeMessage,
        typeMessageList,
        typeSignal,
        typeSignalList
    };
    enum treeRootIndex
    {
        networkIndex = 0,
        ecuIndex,
        nodeIndex,
        messageIndex,
        signalIndex
    };
    static const QString byteOrderTable[2];
    static const QString valueTypeTable[2];
    Qt::SortOrder tableWidgetSort;
    QString dbcName;
    dbc_t * dbc1;

private:
    void showNetworks(const dbc_t * dbc);
    void showNetworkNodeList(const dbc_t * dbc);
    void showNetworkNode(const dbc_t * dbc, const char * node_name);
    void showNetworkNodeMessageList(const dbc_t * dbc, const char * node_name);
    void showNetworkNodeSignalList(const dbc_t * dbc, const char * node_name);
    void showMessages(const dbc_t * dbc);
    void showSignalsList(const dbc_t * dbc);
    void showSignals(const dbc_t * dbc, const char * message_name);
    void showSignal(const dbc_t * dbc, const char * signal_name);

    void tableWidget_clear(QTableWidget * widget);
    void tableWidget_add_nodeHeader(QTableWidget * widget);
    void tableWidget_add_node(QTableWidget *widget, const node_t * node);
    void tableWidget_add_messageHeader(QTableWidget * widget);
    void tableWidget_add_message(QTableWidget * widget, const message_t * message);
    void tableWidget_add_signalHeader(QTableWidget * widget);
    void tableWidget_add_signal(QTableWidget * widget, const signal_t * signal);

private slots:
    void on_actionOpen_triggered();

    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_actionAbout_triggered();

    void on_actionshow_hex_triggered();

    void on_actiontableWidget_sort(int col);
private:
    Ui::CanPaddle *ui;
};
#endif // CANPADDLE_H
