#pragma once

#include <QDialog>
#include "ui_WNetDialog.h"

class WNetDialog : public QDialog
{
	Q_OBJECT

public:
	WNetDialog(QWidget *parent = nullptr);
	~WNetDialog();

public:
	QString getNetPath();

private slots:
	void slotOk();

	void slotCancel();

private:
	Ui::WNetDialogClass ui;
};
