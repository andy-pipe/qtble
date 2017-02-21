#include "qtble.h"
#include <fcntl.h>
#include <string>
#include <sstream>
#include <iostream>
#include <system_error>
#include <curl/curl.h>
#include <curl/easy.h>
#include <credentials.h>

#define MAX_BUF 1024
#define fifoout "/tmp/ausgang"
#define fifoin  "/tmp/eingang"
#define handleTemp "0x0021"
#define handleHum "0x0029"
#define handlePres "0x0031"
#define handleLight "0x0041"
#define handleKey "0x0049"
#define loop_ms 90		//Loop time to check for new feedback
#define send_ms 1333    //Loop time to send data to cloud
#define wd_ms 5000      //Watchdog timeout

const std::string apiKeyStream(cr_apiKeyStream);
const std::string apiKeyReadOnly(cr_apiKeyReadOnly);
const std::string apiKeyFull(cr_apiKeyFull);
const std::string device(cr_device);

QFile fdi(fifoin);
int fdo;
bool connected = false;
bool initialized = false;
bool cloud = false;
QTextStream out(&fdi);

float sAmb, sObj, sLux, sPres, sHum;
int sKey1, sKey2, sKey3;

QTimer *sectimer = new QTimer();
QTimer *sendtimer = new QTimer();
QTimer *wdtimer = new QTimer();

QNetworkConfigurationManager ncm;

Qtble::Qtble(QWidget *parent) :
	QMainWindow(parent)
{

	_ui.setupUi(this);
#ifdef __arm__
	setWindowState(windowState() | Qt::WindowFullScreen);
#endif
	_ui.BLEconnected->setText("<font color='red'>BLE offline</font>");
	_ui.pushButtonCloud->setText("Cloud off");

	//Initialize connection to command pipe
	fdi.open((QIODevice::WriteOnly | QIODevice::Text));

	//Initialize connection to feedback pipe
    fdo = open(fifoout, O_RDONLY | O_NONBLOCK);

	//Initialize timer to periodically read feedback pipe
    sectimer->start(loop_ms);
	connect(sectimer, SIGNAL(timeout()), this, SLOT(readPipe()));

	//Initialize connection to periodically send data to cloud method
	connect(sendtimer, SIGNAL(timeout()), this, SLOT(sendData()));

	//Initialize button to initiate send data to cloud
	connect(_ui.pushButtonCloud, SIGNAL(clicked()), this, SLOT(button_clickedCloud()));

	//Initialize timer to peridically update date/time status line
	QTimer *datetimer = new QTimer();
	datetimer->start(1000);
	connect(datetimer, SIGNAL(timeout()), this, SLOT(showDateTime()));

	//Initialize watchdog
	QObject::connect(wdtimer, SIGNAL(timeout()), this, SLOT(watchdog()));

	connect(&ncm, SIGNAL(onlineStateChanged(bool)), this, SLOT(onNetworkStateChanged(bool)));
}


Qtble::~Qtble()
{
}


void Qtble::readPipe()
{


    char buf[MAX_BUF];
    int nindex;
    ssize_t len = 0;
	//qDebug() << "tick";
	if (! connected) {
	    out << "connect" << endl; //Initiate connection
	    out.flush();
	}
	if ((len = read(fdo, buf, MAX_BUF)) > 0) {
        buf[len] = 0;
        //printf("Length:   %d\n", (int)len);
        //qDebug() << "Buffer [" << len << "]: " << endl;
		//printf("%s", buf);
		//qDebug() << endl;
		QString input = QString(buf);
		//qDebug() << "Input: " << endl;
		qDebug() << input << endl;
        QStringList list = input.split('\n');
		//qDebug() << "List: " << endl;
    	for(int i = 0; i < (list.length()) ;i++) {
    		QString line = list.at(i);
    		//qDebug() << i << " .. " << endl << line << endl;
			if (line.contains("quit")) {
				qApp->exit();
			}
			if (line.contains("Connection successful")) {
				qDebug() << "Connected";
				_ui.BLEconnected->setText("<font color='green'>BLE connected</font>");
				connected = true;
			}
			if (line.contains("Characteristic value")) {
				qDebug() << " ... still alive" << endl;
				wdtimer->start(wd_ms);
			}
			if (line.contains("Disconnected")) {
				qDebug() << "Disconnected";
				_ui.BLEconnected->setText("<font color='red'>BLE offline</font>");
				wdtimer->stop();
				connected = false;
				initialized = false;
			}
			if (connected && (! initialized)) {
				out << "char-write-req 0x004A 0100" << endl; //Keys enabled
				out.flush();
				out << "char-write-req 0x0046 65" << endl;   //LUXmeter period
				out.flush();
				out << "char-write-req 0x0042 0100" << endl; //LUXmeter notification
				out.flush();
				out << "char-write-req 0x0044 01" << endl;   //LUXmeter enabled
				out.flush();
				out << "char-write-req 0x0026 67" << endl;   //Temperature period
				out.flush();
				out << "char-write-req 0x0022 0100" << endl; //Temperature notification
				out.flush();
				out << "char-write-req 0x0024 01" << endl;   //Temperature enabled
				out.flush();
				out << "char-write-req 0x0036 69" << endl;   //Pressure period
				out.flush();
				out << "char-write-req 0x0032 0100" << endl; //Pressure notification
				out.flush();
				out << "char-write-req 0x0034 01" << endl;   //Pressure enabled
				out.flush();
				out << "char-write-req 0x002E 66" << endl;   //Humidity period
				out.flush();
				out << "char-write-req 0x002A 0100" << endl; //Humidity notification
				out.flush();
				out << "char-write-req 0x002C 01" << endl;   //Humidity enabled
				out.flush();
				initialized = true;
				wdtimer->start(wd_ms);
			}
			if (connected && ((nindex = line.indexOf("Notification handle = ")))  >= 0) {
				//qDebug() << "Notification!!" << endl;
				wdtimer->start(wd_ms);
				if (line.contains(handleTemp)) {
					//qDebug() << "Temperature!";
					//qDebug() << line;
					uint16_t tAmb, tObj;
					float ftAmb, ftObj;
					bool ok;
					int index = line.indexOf("value:") + 7;
					tAmb = line.mid(index,2).toInt(&ok, 16) + 256 * line.mid(index + 3,2).toInt(&ok, 16);
					tObj = line.mid(index + 6,2).toInt(&ok, 16) + 256 * line.mid(index + 9,2).toInt(&ok, 16);
					//qDebug() << "tAmb=" << tAmb << " tObj="<<tObj;
					sensorTmp007Convert(tAmb, tObj, &ftAmb, &ftObj);
					//qDebug() << "tAmb=" << ftAmb << "°C ... tObj="<<ftObj<<"°C";
					_ui.progressBarAmb->setValue(ftAmb);
					_ui.progressBarObj->setValue(ftObj);
					sAmb = round(ftAmb);
					sObj = round(ftObj);
				}
				if (line.contains(handleHum)) {
					//qDebug() << "Humidity";
					//qDebug() << line;
					uint16_t Hum;
					float fHum, fTemp;
					bool ok;
					int index = line.indexOf("value:") + 7;
					Hum = line.mid(index + 6,2).toInt(&ok, 16) + 256 * line.mid(index + 9,2).toInt(&ok, 16);
					//qDebug() << "Hum=" << Hum;
					sensorHdc1000Convert(0, Hum, &fTemp, &fHum);
					//qDebug() << "Hum=" << fHum << "%r.h.";
					_ui.progressBarHum->setValue(fHum);
					sHum = round(fHum);
				}
				if (line.contains(handlePres)) {
					//qDebug() << "Pressure!";
					uint32_t tPres;
					//uint32_t tTemp;
					float fPres;
					//float fTemp;
					bool ok;
					//qDebug() << line;
					int index = line.indexOf("value:") + 7;
					//tTemp = line.mid(index,2).toInt(&ok, 16) +
					//256 * line.mid(index + 3,2).toInt(&ok, 16) +
					//65536 * line.mid(index + 6,2).toInt(&ok, 16);
					tPres = line.mid(index + 9,2).toInt(&ok, 16) +
							256 * line.mid(index + 12,2).toInt(&ok, 16) +
							65536 * line.mid(index + 15,2).toInt(&ok, 16);
					//qDebug() << "Temp=" << tTemp << " Pres="<<tPres;
					//fTemp = calcBmp280(tTemp);
					fPres = calcBmp280(tPres);
					//qDebug() << "Pres="<<fPres<<"hPa";
					_ui.progressBarPres->setValue(fPres);
					sPres = round(fPres);
				}
				if (line.contains(handleLight)) {
					//qDebug() << "Light!";
					uint16_t light;
					int iLUX;
					bool ok;
					//qDebug() << line;
					int index = line.indexOf("value:") + 7;
					light = line.mid(index + 0,2).toInt(&ok, 16) + 256 * line.mid(index + 3,2).toInt(&ok, 16);
					//qDebug() << "Light=" << light;
					iLUX = (int) sensorOpt3001Convert(light);
					//qDebug() << "LUX=" << iLUX;
					_ui.lcdNumberLUX->display(iLUX);
					sLux = iLUX;
				}
				if (line.contains(handleKey)) {
					//qDebug() << "Key!";
					int keys;
					bool ok;
					//qDebug() << line;
					int index = line.indexOf("value:") + 7;
					keys = line.mid(index,2).toInt(&ok, 16) & 0x07;
					//qDebug() << "Key=" << keys;
					sKey1 = keys & 0x01;
					_ui.progressBarKey1->setValue(sKey1);
					sKey2 = (keys & 0x02)>>1;
					_ui.progressBarKey2->setValue(sKey2);
					sKey3 = (keys & 0x04)>>2;
					_ui.progressBarReed->setValue(sKey3);
				}
	    	}
		}
    }
}


void Qtble::sendData()

{
	const char* url = "http://api.carriots.com/streams";
	const char* contentType = "Content-Type: application/json";

	struct entry {
		std::string item;
		std::string value;
	};

	QList<entry> dataList;

	//dataList.append((struct entry)("tAMB"; sAmb));

	/* QStringList dataList =

			input.split('\n');
			//qDebug() << "List: " << endl;
	    	for(int i = 0; i < (list.length()) ;i++) {
	    		QString line = list.at(i); */

	//qDebug() << "Sending";

	QDateTime dateTime = QDateTime::currentDateTime();
	long zeit = (dateTime.toMSecsSinceEpoch()/1000);

	std::string apiKeyHeader("carriots.apikey: ");
	apiKeyHeader += apiKeyStream;

	std::ostringstream jsonStream;
	jsonStream <<
		"{"
			"\"protocol\": \"v1\","
			"\"checksum\": \"\","
			"\"device\": \"" << device << "\","
			"\"at\": \"" << zeit << "\","
			"\"data\": { ";
	jsonStream << "\"tAmb\": " << sAmb << ",";
	jsonStream << "\"tObj\": " << sObj << ",";
	jsonStream << "\"tLux\": " << sLux << ",";
	jsonStream << "\"tHum\": " << sHum << ",";
	jsonStream << "\"tPres\": " << sPres << ",";
	jsonStream << "\"Key1\": " << sKey1 << ",";
	jsonStream << "\"Key2\": " << sKey2 << ",";
	jsonStream << "\"Key3\": " << sKey3 <<
				"}"
		"}";
	std::string json(jsonStream.str());

	//qDebug() << json.data();

	CURL* curl = curl_easy_init();
	if (!curl)
		throw std::system_error(CURLE_FAILED_INIT,
			std::generic_category(),
			"failed to initialize curl");

	curl_easy_setopt(curl, CURLOPT_URL, url);

	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, contentType);
	headers = curl_slist_append(headers, apiKeyHeader.c_str());

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.data());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json.size());

	CURLcode result = curl_easy_perform(curl);
	long httpStatus;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (result != CURLE_OK)
		throw std::system_error(result,
			std::generic_category(),
			"failed to send request");
	else if (httpStatus != 200)
		throw std::system_error(httpStatus,
			std::generic_category(),
			"server error");
}


void Qtble::showDateTime()
{
	QDateTime dateTime = QDateTime::currentDateTime();
	_ui.labelDateTime->setText(dateTime.toString("dddd d.M.yyyy hh:mm:ss"));
}


void Qtble::watchdog()
{
	qDebug() << "Watchdog ... " << endl;
	out << "char-read-hnd 0x01" << endl; //Check if connection alive
	out.flush();
}

void Qtble::onNetworkStateChanged(bool isOnline)
{
    qDebug() << "Network state changed, now" << isOnline;
}


float getNumberFromQString(const QString &xString)
{
  QRegExp xRegExp("(-?\\d+(?:[\\.,]\\d+(?:e\\d+)?)?)");
  xRegExp.indexIn(xString);
  QStringList xList = xRegExp.capturedTexts();
  if (true == xList.empty())
  {
    return 0.0;
  }
  return xList.begin()->toFloat();
}


void Qtble::button_clickedCloud()
{
	if (connected){
		if (cloud) {
			cloud = false;
			sendtimer->stop();
			_ui.pushButtonCloud->setText("Cloud off");
		}
		else {
			cloud = true;
			sendtimer->start(send_ms);
			_ui.pushButtonCloud->setText("CLOUD ON");
		}
	}
}


void sensorTmp007Convert(uint16_t rawAmbTemp, uint16_t rawObjTemp, float *tAmb, float *tObj)
{
  const float SCALE_LSB = 0.03125;
  float t;
  int it;

  it = (int)((rawObjTemp) >> 2);
  t = ((float)(it)) * SCALE_LSB;
  *tObj = t;

  it = (int)((rawAmbTemp) >> 2);
  t = (float)it;
  *tAmb = t * SCALE_LSB;
}

float sensorOpt3001Convert(uint16_t rawData)
{
  uint16_t e, m;

  m = rawData & 0x0FFF;
  e = (rawData & 0xF000) >> 12;

  return m * (0.01 * pow(2.0,e));
}

void sensorHdc1000Convert(uint16_t rawTemp, uint16_t rawHum,
                        float *temp, float *hum)
{
  //-- calculate temperature [°C]
  *temp = ((double)(int16_t)rawTemp / 65536)*165 - 40;

  //-- calculate relative humidity [%RH]
  *hum = ((double)rawHum / 65536)*100;
}


float calcBmp280(uint32_t rawValue)
{
  return rawValue / 100.0f;
}

