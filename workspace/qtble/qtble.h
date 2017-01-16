#ifndef _QTBLE_H_
#define _QTBLE_H_

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <GESys/ge_persistent_value.h>

#include "ui_qtble.h"

class QLocalSocket;

class Qtble : public QMainWindow {
	Q_OBJECT
public:
	Qtble(QWidget *parent = nullptr);
	~Qtble();

private:
	Ui::QtbleClass _ui;

private slots:
	void readPipe();
	void sendData();
	void showDateTime();
	void button_clickedCloud();

};

float getNumberFromQString(const QString &xString);
void sensorTmp007Convert(uint16_t rawAmbTemp, uint16_t rawObjTemp, float *tAmb, float *tObj);
float sensorOpt3001Convert(uint16_t rawData);
void sensorHdc1000Convert(uint16_t rawTemp, uint16_t rawHum, float *temp, float *hum);
float calcBmp280(uint32_t rawValue);

#endif // _QTBLE_H_
