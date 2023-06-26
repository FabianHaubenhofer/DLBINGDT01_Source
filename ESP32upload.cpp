#include "ESP32upload.h"

Uploader::Uploader(const char *URL) : _URL(URL), _https(false)
{
}

Uploader::Uploader(const char *URL, FS *tempfileFS) : _URL(URL), _tempfileFS(tempfileFS), _https(false)
{
}

Uploader::Uploader(const char *URL, const char *cert) : _URL(URL), _https(true), _cert(cert)
{
}

Uploader::Uploader(const char *URL, const char *cert, FS *tempfileFS) : _URL(URL),  _tempfileFS(tempfileFS), _https(true), _cert(cert)
{
}

void Uploader::set(const char *URL){
	_URL = URL;
}

int Uploader::send(MultipartMessage message)
{

	HTTPClient myClient;
	int responseCode = 0;
	if (_https)
	{
		myClient.begin(_URL, _cert);
	}
	else
	{
		myClient.begin(_URL);
	}

	File tempfile;
	if (_tempfileFS->exists("/tempfile.temp"))
	{
		_tempfileFS->remove("/tempfile.temp");
	}
	tempfile = _tempfileFS->open("/tempfile.temp", FILE_WRITE);
	if (tempfile)
	{

		tempfile.print("----abcdefg\r\n");
		std::map< const char*, const char * >::iterator it = message.otherData.begin();
		while (it != message.otherData.end())
		{

			tempfile.printf("Content-Disposition: form-data; name=\"%s\"\r\n", it->first);
			tempfile.print("\r\n");
			tempfile.printf("%s\r\n", it->second);
			tempfile.print("----abcdefg\r\n");

			it++;
		}

		tempfile.printf("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n", message.name, message.filename);
		tempfile.printf("Content-Type: %s\r\n", message.contentType);
		tempfile.print("\r\n");
		uint8_t buf[256];
		Serial.printf("File has length %u\n", message.file->size());
		while (message.file->available())
		{
			int read = message.file->read(buf, 256);
			tempfile.write(buf, read);
			vTaskDelay(1);
		}
		message.file->close();
		tempfile.print("\r\n");
		tempfile.print("----abcdefg--\r\n");
		tempfile.flush();
		tempfile.close();
		myClient.addHeader("Content-Type", "multipart/form-data; boundary=\"--abcdefg\"");
		tempfile = _tempfileFS->open("/tempfile.temp");

		if (tempfile)
		{
			Serial.printf("Sending tempfile of length %u\n", tempfile.size());
			responseCode = myClient.sendRequest("POST", &tempfile, tempfile.size());
			while (myClient.getStream().available())
			{
				Serial.write(myClient.getStream().read());
			}
		}
	}

	myClient.end();
	return responseCode;
}