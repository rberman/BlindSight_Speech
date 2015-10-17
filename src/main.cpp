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


// Global Var: sentence to read

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
			wprintf_s(L"Yes master, y'all are smiling.");
			break;
		case 2:
			wprintf_s(L"Today is Saturday.");
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
	pxcCHAR *cmds[3] = { L"Read text for me", L"Recognize face", L"What is the weather" };
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

int speak_aloud(PXCSession *session, const std::string text) {
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


int main() {
	// Create and retrieve a session
	PXCSession *session = PXCSession::CreateInstance();
	if (session == NULL) {
		wprintf_s(L"Session not created by PXCSession\n");
		return 1;
	}

	listen(session);
	std::string text = "I'm saying this text";
	speak_aloud(session, text);

	//Release the session instance
	session->Release();
	return 0;
	return 0;
}