/*
* Implementation of the SMTPS email class with Curl
* Author: walid Abbassi [https://github.com/walidAbbassi]
* 2019
*
*/

#include "pch.h"
#include "EmailWithCurl.h"
//#include <string.h>

#define MAX_LEN 255 // this must be divisible by 3 otherwise the SMTP server won't be able to decode the attachment properly
#define ENCODED_LEN 341

// offsets into the email template 
#define BOUNDARY 5
#define END_LINE 8
#define ATTACH_TYPE 10
#define ATTACH_TRANSFER 11
#define ATTACH_DEPOSITION 12
#define END_OF_TRANSMISSION_BOUNDARY 15
#define END_OF_TRANSMISSION 16

#ifdef WINDOWS_H
#define DIR_CHAR '\\'
#else
#define DIR_CHAR '/'
#endif


// base64 encoding functions
void base64_encode(char *input_buf, char *output_buf, size_t input_size);
void encodeblock(char *in, char *out, int len);

// callback function
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp);

// base64 encoding table
const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// global vector this is TEMP
std::vector<std::string> global_vec;

// struct used by the CURL callback
struct upload_status {
	size_t lines_read;
};

static const char *email_template[] = {
	/* i = 0 */
	"User-Agent: DXF Viewer agent\r\n",
	"MIME-Version: 1.0\r\n",
	"Content-Type: multipart/mixed;\r\n",
	" boundary=\"------------030203080101020302070708\"\r\n",
	"\r\nThis is a multi-part message in MIME format.\r\n",
	"--------------030203080101020302070708\r\n", // use this type of boundary for subsequent attachments
	"Content-Type: text/plain; charset=utf-8; format=flowed\r\n",
	"Content-Transfer-Encoding: 7bit\r\n",
	/* i = 8 (BODY) */
	"\r\n", /* empty line to divide headers from body, see RFC5322 */
	"--------------030203080101020302070708\r\n", /* i = 9 */
	/* Template for binary attachments i = 10 */
	"Content-Type: application/octet-stream\r\n",
	"Content-Transfer-Encoding: base64\r\n",
	"Content-Disposition: attachment;\r\n",
	/* Filename here */
	"\r\n",
	/* Encoded base64 contents here */
	"\r\n",
	"--------------030203080101020302070708--\r\n", // this type of boundary indicates that there are no more parts i = 15
	"\r\n.\r\n",
	NULL
};

/*
*	setter who the email is going to
*	@name	: setTo
*	@param	: string
*	@return : void
*/
void EmailWithCurl::setTo(const std::string &to)
{
	this->to = to;
}

/*
*	setter who the email came from
*	@name	: setFrom
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::setFrom(const std::string &from)
{
	this->from = from;
}

/*
*	setter the cc of the email
*	@name	: setCc
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::setCc(const std::string &cc)
{
	this->cc = cc;
}

/*
*	setter the subject of the email
*	@name	: setSubject
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::setSubject(const std::string &subject)
{
	this->subject = subject;
}

/*
*	setter the body of the email
*	@name	: setBody
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::setBody(const std::string &body)
{
	this->body = body;
}

/*
*	email setter the smtp Username 
*	@name	: setSmtpUsername
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::setSmtpUsername(const std::string &user)
{
	this->smtpUser = user;
}

/*
*	email setter the smtp Password 
*	@name	: setSmtpPassword
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::setSmtpPassword(const std::string &pass)
{
	this->smtpPassword = pass;
}

/*
*	email setter the smtp Host
*	@name	: setSmtpHost
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::setSmtpHost(const std::string &hostname)
{
	this->smtpHost = hostname;
}

/*
*	adds a binary attachment to the email
*	@name	: addAttachment
*	@param	: string.
*	@return : void
*/
void EmailWithCurl::addAttachment(const std::string &file_path)
{
	FILE *file = NULL;
	char buffer[MAX_LEN + 1] = { 0 };
	char encodedBuffer[ENCODED_LEN] = { 0 };
	char tempBuffer[MAX_LEN] = { 0 };
	char *filename = NULL;
	unsigned int fileSize = 0;
	unsigned int bytesCopied = 0;
	int bytesToCopy = 0;
	int bytesRead = 0;

	file = fopen(file_path.c_str(), "rb");
	if (file) {
		// get the file size
		fseek(file, 0, SEEK_END);
		fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		// copy the attachment header
		std::string type(email_template[ATTACH_TYPE]);
		this->attachments.push_back(type);

		std::string encodingType(email_template[ATTACH_TRANSFER]);
		this->attachments.push_back(encodingType);

		std::string deposition(email_template[ATTACH_DEPOSITION]);
		this->attachments.push_back(deposition);

		// extract the filename from the path
		filename = (char *)strrchr(file_path.c_str(), DIR_CHAR);

		if (filename != NULL) {
			filename += 1;
			// push the filename
			snprintf(tempBuffer, MAX_LEN, "  filename=\"%s\"\r\n", filename);

			std::string filename(tempBuffer);
			this->attachments.push_back(filename);

			std::string endLine(email_template[END_LINE]);
			this->attachments.push_back(endLine);

			// copy the file MAX_LEN bytes at a time into the attachments vector
			while (bytesCopied < fileSize) {
				// determine how many bytes to read
				if (bytesCopied + MAX_LEN > fileSize) {
					bytesToCopy = fileSize - bytesCopied;
				}

				else {
					bytesToCopy = MAX_LEN;
				}

				// read from the file
				memset(buffer, 0, MAX_LEN + 1);
				bytesRead = fread(buffer, sizeof(char), bytesToCopy, file);

				// encoded the data read
				memset(encodedBuffer, 0, ENCODED_LEN);
				base64_encode(buffer, encodedBuffer, bytesRead);

				// setup the encoded string so that we can push it to attachments
				std::string line(encodedBuffer);
				line += endLine;

				this->attachments.push_back(line);

				// update the number of bytes we have copied
				bytesCopied += bytesToCopy;
			}

			this->attachments.push_back(endLine);

			std::string boundary(email_template[BOUNDARY]);
			this->attachments.push_back(boundary);

		}

		else {
			removeAllAttachments();
			std::cout << "Failed to extract filename!" << std::endl;
		}


		// close the file
		fclose(file);
	}

	else {
		std::cout << "Unable to open file." << std::endl;
	}
}

/*
*	Contructs the final email
*	@name	: constructEmail
*	@param	: no param.
*	@return : void
*/
void EmailWithCurl::constructEmail()
{
	char buffer[MAX_LEN];
	std::string boundary(email_template[BOUNDARY]);

	// time stuff
	time_t rawtime;
	struct tm * timeinfo;

	// store all the email contents in a vector
	// TO:
	snprintf(buffer, MAX_LEN, "To: %s\r\n", this->to.c_str());
	std::string line1(buffer);
	this->email_contents.push_back(line1);

	// FROM:
	memset(buffer, 0, MAX_LEN);
	snprintf(buffer, MAX_LEN, "From: %s\r\n", this->from.c_str());
	std::string line2(buffer);
	this->email_contents.push_back(line2);

	// CC:
	memset(buffer, 0, MAX_LEN);
	snprintf(buffer, MAX_LEN, "Cc: %s\r\n", this->cc.c_str());
	std::string line3(buffer);

	if (this->cc.length() > 0) {
		this->email_contents.push_back(line3);
	}

	// Subject:
	memset(buffer, 0, MAX_LEN);
	snprintf(buffer, MAX_LEN, "Subject: %s\r\n", this->subject.c_str());
	std::string line4(buffer);
	this->email_contents.push_back(line4);

	// time:
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	memset(buffer, 0, MAX_LEN);
	strftime(buffer, sizeof(buffer), "%d/%m/%Y %I:%M:%S +1100\r\n", timeinfo);
	std::string time_str(buffer);

	this->email_contents.push_back(time_str);

	std::cout << time_str << std::endl;

	for (size_t i = 0; i < END_LINE; i++) { // other stuff e.g user-agent etc
		std::string line(email_template[i]);
		this->email_contents.push_back(line);
	}

	// add in the body
	std::string endLine(email_template[END_LINE]);
	this->email_contents.push_back(endLine);

	memset(buffer, 0, MAX_LEN);
	snprintf(buffer, MAX_LEN, "%s", this->body.c_str()); // Body:
	std::string line5(buffer);

	this->email_contents.push_back(line5); // body
	this->email_contents.push_back(endLine); // \r\n
	this->email_contents.push_back(boundary); // boundary

	 //add in the attachments
	for (size_t i = 0; i < attachments.size(); i++) {
		this->email_contents.push_back(this->attachments[i]);
	}

	// add the last boundary with the two hyphens
	std::string lastBoundary(email_template[END_OF_TRANSMISSION_BOUNDARY]);
	this->email_contents.push_back(lastBoundary);

	// specify that we don't want to send any more data
	std::string endTransmission(email_template[END_OF_TRANSMISSION]);
	this->email_contents.push_back(endTransmission);
}

/*
	This function was taken and modified from:
	https://curl.haxx.se/libcurl/c/smtp-ssl.html
*/
int EmailWithCurl::send(void) const
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx;

	upload_ctx.lines_read = 0;

	curl = curl_easy_init();

	global_vec = this->email_contents;

	if (curl) {
		/* Set username and password */
		curl_easy_setopt(curl, CURLOPT_USERNAME, this->smtpUser.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, this->smtpPassword.c_str());

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // allows emails to be sent 

		/* This is the URL for your mailserver. Note the use of smtps:// rather
		* than smtp:// to request a SSL based connection. */
		curl_easy_setopt(curl, CURLOPT_URL, this->smtpHost.c_str());

		/* If you want to connect to a site who isn't using a certificate that is
		* signed by one of the certs in the CA bundle you have, you can skip the
		* verification of the server's certificate. This makes the connection
		* A LOT LESS SECURE.
		*
		* If you have a CA cert for the server stored someplace else than in the
		* default bundle, then the CURLOPT_CAPATH option might come handy for
		* you. */
#ifdef SKIP_PEER_VERIFICATION
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

		/* If the site you're connecting to uses a different host name that what
		* they have mentioned in their server certificate's commonName (or
		* subjectAltName) fields, libcurl will refuse to connect. You can skip
		* this check, but this will make the connection less secure. */
#ifdef SKIP_HOSTNAME_VERIFICATION
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

		/* Note that this option isn't strictly required, omitting it will result
		* in libcurl sending the MAIL FROM command with empty sender data. All
		* autoresponses should have an empty reverse-path, and should be directed
		* to the address in the reverse-path which triggered them. Otherwise,
		* they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
		* details.
		*/
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, this->from.c_str());

		/* Add two recipients, in this particular case they correspond to the
		* To: and Cc: addressees in the header, but they could be any kind of
		* recipient. */
		recipients = curl_slist_append(recipients, this->to.c_str());
		if (this->cc.length() > 0) {
			recipients = curl_slist_append(recipients, this->cc.c_str());
		}

		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		/* We're using a callback function to specify the payload (the headers and
		* body of the message). You could just use the CURLOPT_READDATA option to
		* specify a FILE pointer to read from. */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		/* Since the traffic will be encrypted, it is very useful to turn on debug
		* information within libcurl to see what is happening during the
		* transfer */
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		/* Send the message */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* Free the list of recipients */
		curl_slist_free_all(recipients);

		/* Always cleanup */
		curl_easy_cleanup(curl);
	}

	return (int)res;
}

/*
*	email removes all attachments
*	@name	: removeAllAttachments
*	@param	: no param.
*	@return : void
*/
void EmailWithCurl::removeAllAttachments()
{
	this->attachments.clear();
}

/*
*	clear email Contents
*	@name	: clearEmailContents
*	@param	: no param.
*	@return : void
*/
void EmailWithCurl::clearEmailContents()
{
	this->email_contents.clear();
	this->attachments.clear();
}

/*
*	dumps the email contents for debugging
*	@name	: dump
*	@param	: no param.
*	@return : void
*/
void EmailWithCurl::dump() const
{

	std::cout << "Email contents: " << std::endl;
	for (size_t i = 0; i < this->email_contents.size(); i++) {
		std::cout << this->email_contents[i];
		if (i == 20) {
			break;
		}
	}

	std::cout << "\n\nEmail attachments: " << std::endl;
	for (size_t i = 0; i < this->attachments.size(); i++) {
		std::cout << this->attachments[i];
		if (i == 5) {
			break;
		}
	}
}


void base64_encode(char *input_buf, char *output_buf, size_t input_size)
{
	char in[3];
	char out[4];
	size_t len = input_size;

	*output_buf = 0;
	for (size_t i = 0; i < len; )
	{
		int buf3_len = 0;
		for (int j = 0; j < 3; ++j)
		{
			in[j] = input_buf[i++];
			if (i > len)
				in[j] = 0;
			else
				buf3_len++;
		}
		if (len > 0)
		{
			encodeblock(in, out, buf3_len);
			strncat(output_buf, out, 4);
		}
	}
}

void encodeblock(char *in, char *out, int len)
{
	out[0] = (char)b64_table[(int)(in[0] >> 2)];
	out[1] = (char)b64_table[(int)(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4))];
	out[2] = (char)(len > 1 ? b64_table[(int)(((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6))] : '=');
	out[3] = (char)(len > 2 ? b64_table[(int)(in[2] & 0x3f)] : '=');
}

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
		return 0;
	}

	if (upload_ctx->lines_read >= 0 && upload_ctx->lines_read < global_vec.size())
	{
		data = global_vec[upload_ctx->lines_read].c_str();

		if (data) {
			size_t len = strlen(data);
			memcpy(ptr, data, len);
			upload_ctx->lines_read++;

			return len;
		}
	}

	return 0;
}
