#include "nodedataviewer.h"
#include "ui_nodedataviewer.h"

NodeDataViewer::NodeDataViewer(CommonNode *node, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NodeDataViewer)
{
    m_node=node;
    ui->setupUi(this);
    connect(&m_timer1s,SIGNAL(timeout()),this,SLOT(Timer1sEvent()));
    connect(ui->closeButton,SIGNAL(clicked()),this,SLOT(close()));

    for(uint i=0;i<m_node->m_srv.num_float_tags;++i)
    {
        ui->listWidget->addItem(m_node->m_nameObject+"["+QString::number(i)+"] = "+QString::number(m_node->m_srv.buff[i]));
    }

    m_timer1s.start(1000);
}
//=====================================================
NodeDataViewer::~NodeDataViewer()
{
    delete ui;
}
//=====================================================
void NodeDataViewer::Timer1sEvent()
{
    for(uint i=0;i<m_node->m_srv.num_float_tags;++i)
    {
        ui->listWidget->item(i)->setText(m_node->m_nameObject+"["+QString::number(i)+"] = "+QString::number(m_node->m_srv.buff[i]));
    }

}
//=====================================================
void NodeDataViewer::closeEvent(QCloseEvent *event)
{
    m_timer1s.stop();
    //qDebug() << "del.....";
    deleteLater();
}
//=====================================================

