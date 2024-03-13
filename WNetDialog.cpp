#include "WNetDialog.h"
#include "global.h"

WNetDialog::WNetDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(ui.btnCancel, &QPushButton::clicked, this, &WNetDialog::slotCancel);
	connect(ui.btnOk, &QPushButton::clicked, this, &WNetDialog::slotOk);

	QString fileName = "rtmp://liteavapp.qcloud.com/live/liteavdemoplayerstreamid";
	ui.lineEdit->setText(fileName);

	this->setWindowTitle("´ò¿ªÍøÂçÁ÷");
}

QString WNetDialog::getNetPath()
{
	return ui.lineEdit->text();
}

WNetDialog::~WNetDialog()
{}

void WNetDialog::slotOk()
{
	accept();
}

void WNetDialog::slotCancel()
{
	reject();
}
