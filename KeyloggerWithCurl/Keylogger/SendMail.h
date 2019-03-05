#ifndef SENDMAIL_H
#define SENDMAIL_H

#include "pch.h"
#include "EmailWithCurl.h"





namespace Mail
{	
	
	
	// Both emails can be the same
	#define X_EM_TO "ToEmailDestination@gmail.com"
	#define X_EM_FROM "FromEmailSource@gmail.com"
	#define X_EM_PASS "Password"


	int SendMail(std::string headMessage, std::string bodyMessage)
	{
		EmailWithCurl email;
		int curlError = 0;
		// e.dump();

		email.setTo(X_EM_TO);
		email.setFrom(X_EM_FROM);
		email.setSubject(headMessage);
		//e.setCc("");
		email.setBody(bodyMessage);

		email.setSmtpHost("smtps://smtp.gmail.com:465");
		email.setSmtpUsername(X_EM_FROM);
		email.setSmtpPassword(X_EM_PASS);


		email.constructEmail();
		email.dump();

		curlError = email.send();

		return curlError;
	}

}

#endif // SENDMAIL_H
