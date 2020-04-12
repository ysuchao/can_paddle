#include "can_paddle.h"
#include "./ui_can_paddle.h"


const QString CanPaddle::byteOrderTable[2] = {"Motorola", "Intel"};
const QString CanPaddle::valueTypeTable[2] = {"Unsigned", "Signed"};

CanPaddle::CanPaddle(QWidget *parent)
    : QMainWindow(parent)
    , dbc1(nullptr)
    , ui(new Ui::CanPaddle)
{
    ui->setupUi(this);
    ui->actionshow_grid->setChecked(true);
    ui->actionshow_hex->setChecked(true);
    ui->tableWidget->setShowGrid(true);
    ui->tableWidget->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    QObject::connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(on_actiontableWidget_sort(int)));
}

CanPaddle::~CanPaddle()
{
    if(dbc1) dbc_free(dbc1);
    delete ui;
}

void CanPaddle::on_actionOpen_triggered()
{
    QString homePath = QDir::homePath();
    QString windowTitle = "Select dbc file";
    QString filter = "Vector DBC File (*.dbc)";
    dbcName = QFileDialog::getOpenFileName(this, windowTitle, homePath, filter);
    if(dbcName == QString("")) { return; }

    ui->tableWidget->clear();
    ui->tableWidget->clearContents();
    ui->tableWidget->setColumnCount(0);
    ui->tableWidget->setRowCount(0);

    /* old */
    if(dbc1) dbc_free(dbc1);

    /* new */
    dbc1 = dbc_read_file((char *)dbcName.toStdString().c_str());
    if(nullptr == dbc1)
    {
        QString title = "Error";
        QString info = "Failed to read this dbc file";
        QMessageBox::critical(this, title, info);
        return;
    }

    string_t network_name;
    network_name = basename(dbc1->filename);
    strtok(network_name, ".");
    QTreeWidgetItem * itemNetwors = new QTreeWidgetItem(treeItemType::typeNetwork);
    itemNetwors->setText(0, QString(network_name));
    auto networkRoot = ui->treeWidget->topLevelItem(treeRootIndex::networkIndex);
    networkRoot->addChild(itemNetwors);
    showNetworks(dbc1);

    auto ecuRoot = ui->treeWidget->topLevelItem(treeRootIndex::ecuIndex);
    for(auto e=dbc1->node_list; e; e=e->next)
    {
        QTreeWidgetItem * itemEcu = new QTreeWidgetItem(treeItemType::typeEcu);
        itemEcu->setText(0, QString::fromStdString(e->node->name));
        ecuRoot->addChild(itemEcu);
    }

    auto nodeRoot = ui->treeWidget->topLevelItem(treeRootIndex::nodeIndex);
    for(auto n=dbc1->node_list; n; n=n->next)
    {
        QTreeWidgetItem * itemNode = new QTreeWidgetItem(treeItemType::typeNode);
        itemNode->setText(0, QString::fromStdString(n->node->name));
        /* item tx messages */
        QTreeWidgetItem * itemMappedTxMessages = new QTreeWidgetItem(treeItemType::typeMessageList);
        itemMappedTxMessages->setText(0, QString("Tx Messages"));
        /* add messages to it */
        for(auto m=dbc1->message_list; m; m=m->next)
        {
            bool match = false;
            if(!qstrcmp(m->message->sender, n->node->name))
            {
                match = true;
            }
            for(auto t=m->message->transmitter_list; t && !match; t=t->next)
            {
                if(!qstrcmp(t->string, n->node->name))
                {
                    match = true;
                }
            }
            if(!match) continue;
            /* show */
            QTreeWidgetItem * itemMappedTxMessage = new QTreeWidgetItem(treeItemType::typeMessage);
            itemMappedTxMessage->setText(0, QString::fromStdString(m->message->name));
            itemMappedTxMessages->addChild(itemMappedTxMessage);
        }
        itemNode->addChild(itemMappedTxMessages);
        /* item rx signals */
        QTreeWidgetItem * itemMappedRxSignals = new QTreeWidgetItem(treeItemType::typeSignalList);
        itemMappedRxSignals->setText(0, QString("Rx Signals"));
        /* add messages to it */
        for(auto m=dbc1->message_list; m; m=m->next)
        {
            for(auto s=m->message->signal_list; s; s=s->next)
            {
                for(auto r=s->signal->receiver_list; r; r=r->next)
                {
                    if(!qstrcmp(r->string, n->node->name))
                    {
                        QTreeWidgetItem * itemMappedRxSignal = new QTreeWidgetItem(treeItemType::typeSignal);
                        itemMappedRxSignal->setText(0, QString::fromStdString(s->signal->name));
                        itemMappedRxSignals->addChild(itemMappedRxSignal);
                        break;
                    }
                }
            }
        }
        itemNode->addChild(itemMappedRxSignals);
        /* add node to root */
        nodeRoot->addChild(itemNode);
    }

    for(auto m=dbc1->message_list; m; m=m->next)
    {
        QTreeWidgetItem * itemMessage = new QTreeWidgetItem(treeItemType::typeMessage);
        itemMessage->setText(0, QString::fromStdString(m->message->name));
        auto messageRoot = ui->treeWidget->topLevelItem(treeRootIndex::messageIndex);
        messageRoot->addChild(itemMessage);
        for(auto s=m->message->signal_list; s; s=s->next)
        {
            /* add to this */
            do {
                QTreeWidgetItem * itemSignal = new QTreeWidgetItem(treeItemType::typeSignal);
                itemSignal->setText(0, QString::fromStdString(s->signal->name));
                itemMessage->addChild(itemSignal);
            } while(0);
            /* add to global signal list */
            do {
                QTreeWidgetItem * itemSignal = new QTreeWidgetItem(treeItemType::typeSignal);
                itemSignal->setText(0, QString::fromStdString(s->signal->name));
                auto signalRoot = ui->treeWidget->topLevelItem(treeRootIndex::signalIndex);
                signalRoot->addChild(itemSignal);
            } while (0);
        }
    }
}

void CanPaddle::showNetworks(const dbc_t * dbc)
{
    string_t network_name;
    network_name = basename(dbc->filename);
    strtok(network_name, ".");
    auto labelList = QStringList();
    labelList.append(QString("Name"));
    labelList.append(QString("Comemnt"));
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setHorizontalHeaderLabels(labelList);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget->setRowCount(1);
    auto itemName = new QTableWidgetItem(QString::fromStdString(network_name));
    std::ostringstream comment("");
    if(dbc->network->comment)
    {
        comment << std::string(dbc->network->comment);
    }
    auto itemComment = new QTableWidgetItem(QString::fromStdString(comment.str()));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 0, itemName);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 1, itemComment);
}

void CanPaddle::showNetworkNodeList(const dbc_t * dbc)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_nodeHeader(ui->tableWidget);
    for(auto n=dbc->node_list; n; n=n->next)
    {
        tableWidget_add_node(ui->tableWidget, n->node);
    }
}

void CanPaddle::showNetworkNode(const dbc_t * dbc, const char * node_name)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_nodeHeader(ui->tableWidget);
    for(auto n=dbc->node_list; n; n=n->next)
    {
        if(qstrcmp(n->node->name, node_name)) continue;
        tableWidget_add_node(ui->tableWidget, n->node);
        break;
    }
}

void CanPaddle::showNetworkNodeMessageList(const dbc_t * dbc, const char * node_name)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_messageHeader(ui->tableWidget);
    for(auto m=dbc->message_list; m; m=m->next)
    {
        bool match = false;
        if(!qstrcmp(m->message->sender, node_name))
        {
            match = true;
        }
        for(auto t=m->message->transmitter_list; t && !match; t=t->next)
        {
            if(!qstrcmp(t->string, node_name))
            {
                match = true;
            }
        }
        if(!match) continue;
        tableWidget_add_message(ui->tableWidget, m->message);
    }
}

void CanPaddle::showNetworkNodeSignalList(const dbc_t * dbc, const char * node_name)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_signalHeader(ui->tableWidget);
    for(auto m=dbc->message_list; m; m=m->next)
    {
        for(auto s=m->message->signal_list; s; s=s->next)
        {
            for(auto r=s->signal->receiver_list; r; r=r->next)
            {
                if(qstrcmp(r->string, node_name)) continue;
                tableWidget_add_signal(ui->tableWidget, s->signal);
                break;
            }
        }
    }
}

void CanPaddle::showMessages(const dbc_t * dbc)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_messageHeader(ui->tableWidget);
    for(auto m=dbc->message_list; m; m=m->next)
    {
        tableWidget_add_message(ui->tableWidget, m->message);
    }
}

void CanPaddle::showSignalsList(const dbc_t * dbc)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_signalHeader(ui->tableWidget);
    for(auto m=dbc->message_list; m; m=m->next)
    {
        for(auto s=m->message->signal_list; s; s=s->next)
        {
            tableWidget_add_signal(ui->tableWidget, s->signal);
        }
    }
}

void CanPaddle::showSignals(const dbc_t * dbc, const char * message_name)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_signalHeader(ui->tableWidget);
    for(auto m=dbc->message_list; m; m=m->next)
    {
        if(qstrcmp(m->message->name, message_name)) continue;
        for(auto s=m->message->signal_list; s; s=s->next)
        {
            tableWidget_add_signal(ui->tableWidget, s->signal);
        }
        break;
    }
}

void CanPaddle::showSignal(const dbc_t * dbc, const char * signal_name)
{
    tableWidget_clear(ui->tableWidget);
    tableWidget_add_signalHeader(ui->tableWidget);
    for(auto m=dbc->message_list; m; m=m->next)
    {
        for(auto s=m->message->signal_list; s; s=s->next)
        {
            if(qstrcmp(s->signal->name, signal_name)) continue;
            tableWidget_add_signal(ui->tableWidget, s->signal);
            break;
        }
    }
}

void CanPaddle::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    if(!dbc1) { return; }

    //std::ostringstream debugString;
    //if(item->parent())
    //{
    //    debugString << item->parent()->type() << "," << item->parent()->text(0).toStdString() << " : ";
    //}
    //debugString << item->type() << "," << item->text(column).toStdString();
    //qDebug() << QString::fromStdString(debugString.str());

    tableWidget_clear(ui->tableWidget);
    if((item->type() == treeItemType::typeRoot && item->text(column) == QString("Networks")) ||
       (item->parent() && item->parent()->type() == treeItemType::typeRoot && item->parent()->text(column) == QString("Networks")))
    {
        showNetworks(dbc1);
    }
    else if(item->type() == treeItemType::typeRoot && item->text(column) == QString("ECUs"))
    {
        showNetworkNodeList(dbc1);
    }
    else if(item->type() == treeItemType::typeRoot && item->text(column) == QString("Network nodes"))
    {
        showNetworkNodeList(dbc1);
    }
    else if(item->type() == treeItemType::typeNode)
    {
        showNetworkNode(dbc1, item->text(0).toStdString().c_str());
    }
    else if(item->parent() && item->parent()->type() == treeItemType::typeNode && item->type() == treeItemType::typeMessageList)
    {
        showNetworkNodeMessageList(dbc1, item->parent()->text(0).toStdString().c_str());
    }
    else if(item->parent() && item->parent()->type() == treeItemType::typeNode && item->type() == treeItemType::typeSignalList)
    {
        showNetworkNodeSignalList(dbc1, item->parent()->text(0).toStdString().c_str());
    }
    else if(item->type() == treeItemType::typeMessage)
    {
        showSignals(dbc1, item->text(0).toStdString().c_str());
    }
    else if(item->type() == treeItemType::typeRoot && item->text(column) == QString("Messages"))
    {
        showMessages(dbc1);
    }
    else if(item->type() == treeItemType::typeRoot && item->text(column) == QString("Signals"))
    {
        showSignalsList(dbc1);
    }
    else if(item->type() == treeItemType::typeSignal)
    {
        showSignal(dbc1, item->text(0).toStdString().c_str());
    }
    else
    {
        /* do nothing */
    }
}

void CanPaddle::on_actionAbout_triggered()
{
    QString title = "About";
    std::ostringstream info;
    info << "CanPaddle V1.0\n"
         << "Vector CAN bus data base file reader\n"
         << "All Rights Reserved by\n"
         << "ysuchao@126.com";
    QMessageBox::about(this, title, QString::fromStdString(info.str()));
}

void CanPaddle::tableWidget_clear(QTableWidget * widget)
{
    widget->clear();
    widget->setColumnCount(0);
    widget->setRowCount(0);
}

void CanPaddle::tableWidget_add_nodeHeader(QTableWidget * widget)
{
    auto labelList = QStringList();
    labelList.append(QString("Name"));
    labelList.append(QString("Comemnt"));
    widget->setColumnCount(2);
    widget->setHorizontalHeaderLabels(labelList);
    widget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

}

void CanPaddle::tableWidget_add_node(QTableWidget * widget, const node_t * node)
{
    widget->setRowCount(widget->rowCount() + 1);
    auto itemName = new QTableWidgetItem(QString::fromStdString(node->name));
    std::ostringstream comment("");
    if(node->comment)
    {
        comment << std::string(node->comment);
    }
    auto itemComment = new QTableWidgetItem(QString::fromStdString(comment.str()));
    widget->setItem(widget->rowCount()-1, 0, itemName);
    widget->setItem(widget->rowCount()-1, 1, itemComment);
}

void CanPaddle::tableWidget_add_messageHeader(QTableWidget * widget)
{
    auto labelList = QStringList();
    labelList.append(QString("Name"));
    labelList.append(QString("Id"));
    labelList.append(QString("Id Format"));
    labelList.append(QString("DLC"));
    labelList.append(QString("Tx Method"));
    labelList.append(QString("Cycle Time"));
    labelList.append(QString("Transmitter"));
    labelList.append(QString("Comemnt"));
    widget->setColumnCount(9);
    widget->setColumnHidden(8, true);
    widget->setHorizontalHeaderLabels(labelList);
    widget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

void CanPaddle::tableWidget_add_message(QTableWidget * widget, const message_t * message)
{
    ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
    auto itemName        = new QTableWidgetItem(QString(message->name));
    QString id("");
    if(ui->actionshow_hex->isChecked())
    {
        id += QString("0x");
        id += QString("%1").arg((message->id & 0x1FFFFFFF), (message->id & 0x80000000)?8:3, 16, QChar('0')).toUpper();
    }
    else
    {
        id += QString::number(message->id & 0x1FFFFFFF);
    }
    auto itemId          = new QTableWidgetItem(id);
    auto itemIdFormat    = new QTableWidgetItem(QString((message->id&0x80000000)?"Extended":"Standard"));
    auto itemDlc         = new QTableWidgetItem(QString::number(message->len));
    auto itemTxMethod    = new QTableWidgetItem(QString(""));
    auto itemCycleTime   = new QTableWidgetItem(QString(""));
    auto itemId2         = new QTableWidgetItem();
    itemId2->setData(Qt::EditRole, message->id & 0x1FFFFFFF);
    std::ostringstream transmitters("");
    string_list_t * tl = message->transmitter_list;
    while((void *)tl != nullptr)
    {
        transmitters << std::string(tl->string) << ";";
        tl = tl->next;
    }
    auto itemTransmitter = new QTableWidgetItem(QString(transmitters.str().c_str()));
    std::ostringstream comment("");
    if(message->comment)
    {
        comment << std::string(message->comment);
    }
    auto itemComment = new QTableWidgetItem(QString(comment.str().c_str()));
    widget->setItem(widget->rowCount()-1, 0, itemName);
    widget->setItem(widget->rowCount()-1, 1, itemId);
    widget->setItem(widget->rowCount()-1, 2, itemIdFormat);
    widget->setItem(widget->rowCount()-1, 3, itemDlc);
    widget->setItem(widget->rowCount()-1, 4, itemTxMethod);
    widget->setItem(widget->rowCount()-1, 5, itemCycleTime);
    widget->setItem(widget->rowCount()-1, 6, itemTransmitter);
    widget->setItem(widget->rowCount()-1, 7, itemComment);
    widget->setItem(widget->rowCount()-1, 8, itemId2);
}

void CanPaddle::tableWidget_add_signalHeader(QTableWidget * widget)
{
    auto labelList = QStringList();
    labelList.append(QString("Name"));
    labelList.append(QString("Length"));
    labelList.append(QString("Byte Order"));
    labelList.append(QString("Value Type"));
    labelList.append(QString("Initial Value"));
    labelList.append(QString("Factor"));
    labelList.append(QString("Offset"));
    labelList.append(QString("Minimum"));
    labelList.append(QString("Maximum"));
    labelList.append(QString("Unit"));
    labelList.append(QString("Value Table"));
    labelList.append(QString("Comment"));
    widget->setColumnCount(12);
    widget->setHorizontalHeaderLabels(labelList);
    widget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

void CanPaddle::tableWidget_add_signal(QTableWidget * widget, const signal_t * signal)
{
    widget->setRowCount(widget->rowCount() + 1);
    auto itemName = new QTableWidgetItem(QString::fromStdString(signal->name));
    auto itemByteOrder = new QTableWidgetItem(byteOrderTable[signal->endianess]);
    auto itemValueType = new QTableWidgetItem(valueTypeTable[signal->signedness]);
    auto itemLength = new QTableWidgetItem(QString::fromStdString(std::to_string(signal->bit_len)));
    auto itemInitialValue = new QTableWidgetItem(QString::fromStdString(std::to_string(0)));
    auto itemFactor = new QTableWidgetItem(QString::number(signal->scale));
    auto itemOffset = new QTableWidgetItem( QString::number(signal->offset));
    auto itemMinimum = new QTableWidgetItem(QString::number(signal->min));
    auto itemMaximum = new QTableWidgetItem(QString::number(signal->max));
    std::ostringstream unit("");
    if(signal->unit)
    {
        unit << std::string(signal->unit);
    }
    auto itemUnit = new QTableWidgetItem(QString::fromStdString(unit.str()));
    auto itemValueTable = new QTableWidgetItem(QString::fromStdString(""));
    std::ostringstream comment("");
    if(signal->comment)
    {
        comment << std::string(signal->comment);
    }
    auto itemComment = new QTableWidgetItem(QString::fromStdString(comment.str()));
    widget->setItem(widget->rowCount()-1, 0, itemName);
    widget->setItem(widget->rowCount()-1, 1, itemLength);
    widget->setItem(widget->rowCount()-1, 2, itemByteOrder);
    widget->setItem(widget->rowCount()-1, 3, itemValueType);
    widget->setItem(widget->rowCount()-1, 4, itemInitialValue);
    widget->setItem(widget->rowCount()-1, 5, itemFactor);
    widget->setItem(widget->rowCount()-1, 6, itemOffset);
    widget->setItem(widget->rowCount()-1, 7, itemMinimum);
    widget->setItem(widget->rowCount()-1, 8, itemMaximum);
    widget->setItem(widget->rowCount()-1, 9, itemUnit);
    widget->setItem(widget->rowCount()-1, 10, itemValueTable);
    widget->setItem(widget->rowCount()-1, 11, itemComment);
}


void CanPaddle::on_actiontableWidget_sort(int col)
{
    if(tableWidgetSort == Qt::SortOrder::AscendingOrder)
    {
        tableWidgetSort = Qt::SortOrder::DescendingOrder;
    }
    else
    {
        tableWidgetSort = Qt::SortOrder::AscendingOrder;
    }
    if(col == 1 && ui->tableWidget->horizontalHeaderItem(col)->text() == QString("Id"))
    {
        col = 8;
    }
    ui->tableWidget->sortItems(col, tableWidgetSort);
}

void CanPaddle::on_actionshow_hex_triggered()
{
    if(ui->tableWidget->horizontalHeaderItem(1)->text() == QString("Id"))
    {
        for(int k=0; k<ui->tableWidget->rowCount(); k++)
        {
            uint32_t id = ui->tableWidget->item(k, 1)->text().toUInt(0, ui->actionshow_hex->isChecked()?10:16);
            QString text("");
            if(ui->actionshow_hex->isChecked())
            {
                text += "0x";
                text += QString("%1").arg(id, (ui->tableWidget->item(k, 2)->text() == QString("Extended"))?8:3, 16, QChar('0')).toUpper();
            }
            else
            {
                text += QString::number(id & 0x1FFFFFFF);
            }
            ui->tableWidget->item(k, 1)->setText(text);
        }
    }
}
