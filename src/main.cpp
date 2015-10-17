#include <Windows.h>
#include <CommCtrl.h>
#include <vector>
#include "main.h"
#include "pxcspeechsynthesis.h"
#include "pxcsession.h"
#include "voice_out.h"
#include "resource.h"
#include <WindowsX.h>
#include <commctrl.h>
#include "pxcmetadata.h"
#include "service/pxcsessionservice.h"
#include <iostream>
#include <string>
#include <pxcsensemanager.h>
#include <pxcspeechrecognition.h>
#include <ctime>

#include <iostream>
using namespace std;
#include <omp.h>
#include <Windows.h>
#include <amp.h>
using namespace concurrency;

int speak_aloud(const std::string text) {
	// Create and retrieve a session
	PXCSession *session = PXCSession::CreateInstance();
	if (session == NULL) {
		wprintf_s(L"Session not created by PXCSession\n");
		return 1;
	}

	//Create an instance of the PXCSpeechSynthesis
	PXCSpeechSynthesis *tts = 0;
	pxcStatus sts = session->CreateImpl<PXCSpeechSynthesis>(&tts);
	if (sts != pxcStatus::PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to create an instance of the PXCSpeechSynthesis\n");
		return 2;
	}
	//Initialize the Module
	PXCSpeechSynthesis::ProfileInfo pinfo;
	tts->QueryProfile(0, &pinfo);
	pinfo.language = PXCSpeechSynthesis::LANGUAGE_US_ENGLISH;
	sts = tts->SetProfile(&pinfo);
	if (sts != pxcStatus::PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to Initialize the Module\n");
		return 3;
	}

	//Speak string
	pxcCHAR sentence[256];
	std::wstring widestr = std::wstring(text.begin(), text.end());
	wcscpy_s(sentence, widestr.c_str());
	// Synthesize the text string
	tts->BuildSentence(1, sentence);
	// Retrieve the synthesized speech
	int nbuffers = tts->QueryBufferNum(1);
	for (int i = 0;i<nbuffers;i++) {
		PXCAudio *audio = tts->QueryBuffer(1, i);
		// send audio to the audio output device
		VoiceOut vo(&pinfo);
		vo.RenderAudio(audio);

		// Clean up
		tts->ReleaseSentence(1);
	}
}

// Say the current date aloud
int readDate() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 80, "%d-%m-%Y", timeinfo);
	std::string str(buffer);

	std::cout << str;
	speak_aloud(str);

	return 0;
}

int readTime() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 80, "%I:%M", timeinfo);
	std::string str(buffer);

	std::cout << str;
	speak_aloud(str);

	return 0;
}

// Speech recognition handler object
class MyHandler : public PXCSpeechRecognition::Handler {
public:
	virtual void PXCAPI OnRecognition(const PXCSpeechRecognition::RecognitionData *data) {
		if (data->scores[0].label<0) {
			wprintf_s(L"Output: %s\n", data->scores[0].sentence);
			if (data->scores[0].tags[0])
				wprintf_s(L"Output: %s\n", data->scores[0].tags);
		}

		// Dispatch differnt work here
		switch (data->scores[0].label) {
		case 0:
			wprintf_s(L"Reading text for you, my pleasure.");
			break;
		case 1:
			readDate();
			break;
		case 2:
			readTime();
			break;
		default:
			break;
		}

		if (data->scores[0].tags[0])
			wprintf_s(L"Output: %s\n", data->scores[0].tags);
	}

	virtual void PXCAPI OnAlert(const PXCSpeechRecognition::AlertData *data) {
		if (data->label == PXCSpeechRecognition::ALERT_SPEECH_BEGIN)
			wprintf_s(L"Alert: SPEECH_BEGIN\n");
		else if (data->label == PXCSpeechRecognition::ALERT_SPEECH_END)
			wprintf_s(L"Alert: SPEECH_END\n");
	}
};

int listen(PXCSession *session) {
	//Create an instance of the PXCSpeechRecognition
	PXCSpeechRecognition *sr = 0;
	pxcStatus sts = session->CreateImpl<PXCSpeechRecognition>(&sr);
	if (sts != pxcStatus::PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to create an instance of the PXCSpeechRecognition\n");
		return -1;
	}
	//Initialize the Module
	PXCSpeechRecognition::ProfileInfo pinfo;
	sr->QueryProfile(0, &pinfo);
	pinfo.language = PXCSpeechRecognition::LANGUAGE_US_ENGLISH;
	sts = sr->SetProfile(&pinfo);
	if (sts != pxcStatus::PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to Configure the Module\n");
		return -1;
	}
	pxcCHAR *cmds[3] = { L"Read text for me", L"What day is it?", L"What time is it?" };
	sr->BuildGrammarFromStringList(1, cmds, 0, 3);
	sr->SetGrammar(1);
	

	//Set the Recognition mode command and control mode or dictation mode
	sts = sr->SetDictation();
	if (sts != pxcStatus::PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to Set the Recognition mode \n");
		return -1;
	}
	
	//Start the speech recognition with the event handler
	MyHandler handler; // handler for PXCSpeechRecognition
	sts = sr->StartRec(NULL, &handler);
	if (sts != pxcStatus::PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to Start the handler \n");
		return -1;
	}
	while (true) { if (GetAsyncKeyState(VK_ESCAPE)) break; } //looping infinitely until escape is pressed
															 //Stop the event handler that handles the speech recognition
	sr->StopRec();
	if (sts != pxcStatus::PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to Stop the handler \n");
		return -1;
	}


	return -1;
}

int main(int argc, char *argv[]) {

	// Create and retrieve a session
	PXCSession *session = PXCSession::CreateInstance();
	if (session == NULL) {
		wprintf_s(L"Session not created by PXCSession\n");
		return 1;
	}

	// Starts a new team with 4 threads
	#pragma omp parallel shared(session) num_threads(4)
	{
		#pragma omp sections
		{
			{
				listen(session);
				std::string text = "I'm saying this text";
			}
			#pragma omp section
			{ cout << "Blah Blah Blah " << '\n'; } // We can add other stuff here that will run in parallel
		}
	}

	//Release the session instance
	session->Release();
	return 0;
	return 0;
	return 0;
}