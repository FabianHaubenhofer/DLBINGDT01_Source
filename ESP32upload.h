#ifndef UPLOAD_H_
#define UPLOAD_H_

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <map>

struct MultipartMessage
{
public:
	std::map<const char *, const char *> otherData; //list containing key-value pairs for message parts other than the file
	const char *filename = "";
	const char *name = "";
	const char *contentType = "image/jpeg";
	File *file;
};

class Uploader
{
private:
	const char *_URL;

	FS *_tempfileFS = &SPIFFS;
	bool _https;
	const char *_cert;

public:
	Uploader(const char *URL);
	Uploader(const char *URL, FS *tempfileFS);
	Uploader(const char *URL, const char *cert);
	Uploader(const char *URL, const char *cert, FS *tempfileFS);
	int send(MultipartMessage message);
	void set(const char *URL);
};

#endif