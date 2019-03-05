#ifndef EMAILWITHCURL_H
#define EMAILWITHCURL_H

#include "pch.h"

// Enable one of the below depending on your operating system
#ifdef __linux__ 
#define LINUX_H
#elif _WIN32
#define WINDOWS_H
#endif

class EmailWithCurl
{
public:
	// default constructor
	EmailWithCurl()= default;
	// copy constructor deleted
	EmailWithCurl(const EmailWithCurl& emailWithCurl) = delete;
	// copy assignment operator deleted
	EmailWithCurl operator=(const EmailWithCurl& emailWithCurl) = delete;
	// move constructor deleted
	EmailWithCurl(EmailWithCurl&& emailWithCurl) = delete;
	// move assignment operator deleted
	EmailWithCurl operator=(EmailWithCurl&& emailWithCurl) = delete;

	// sets who the email is going to
	void setTo(const std::string &to);

	// sets who the email came from
	void setFrom(const std::string &from);

	// sets the cc
	void setCc(const std::string &to);

	// sets the subject of the email
	void setSubject(const std::string &subject);

	// set the body of the email
	void setBody(const std::string &body);

	// sets the smtp username 
	void setSmtpUsername(const std::string &username);

	// sets the smtp password 
	void setSmtpPassword(const std::string &password);

	// sets the SMTP HOST
	void setSmtpHost(const std::string &host);

	// adds a binary attachment to the email
	void addAttachment(const std::string &filepath);

	// removes an attachment from the email (Not implemented yet)
	void removeAttachment(const std::string &filepath);

	// removes all attachments
	void removeAllAttachments();

	// contructs the final email
	void constructEmail();

	// clears the contents of the email
	void clearEmailContents();

	// void send email
	int send() const; // returns a CURL error code

	// dumps the email contents for debugging
	void dump() const;

private:
	// Smtp Information
	std::string smtpUser;
	std::string smtpPassword;
	std::string smtpHost;

	// email Data
	std::string to;
	std::string from;
	std::string cc;
	std::string subject;
	std::string body;

	// vector which stores the email Data
	std::vector<std::string> email_contents;
	std::vector<std::string> attachments;

	// length of the above vector
	int numberOfLines;
};

#endif	// EMAILWITHCURL_H