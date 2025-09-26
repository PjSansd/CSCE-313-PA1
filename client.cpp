/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	

	Name: Haihua Pei
	UIN: 333005716
	Date: 09/23/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <fstream>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}

	// Start fork so that server is running from client

	int status; 
	pid_t pid = fork();

	if (pid == -1){
		cout << "Fork Failed. \n";
		return 1;
	}

	if (pid == 0) {

		cout << "Fork Successful. \n";

		// You can add "-m buffer_size" to increase buffer size
		execl("server", "server", NULL);

		cout << "this shouldn't print.";
	}



    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	
	// example data point request
    // char buf[MAX_MESSAGE]; // 256
    // datamsg x(1, 0.0, 1);
	
	// memcpy(buf, &x, sizeof(datamsg));
	// chan.cwrite(buf, sizeof(datamsg)); // question
	// double reply;
	// chan.cread(&reply, sizeof(double)); //answer
	// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

	// Data Point Request
	char dataBuffer[MAX_MESSAGE]; // 256

	// If requesting filename: 
	if (filename != "") {
		cout << " Starting File Transfer. " << endl;

		// Send initial request/handshake:

		filemsg fm(0, 0);
		
		int len = sizeof(filemsg) + (filename.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), filename.c_str());
		chan.cwrite(buf2, len);  // I want the file length;

		delete[] buf2;

		__int64_t response; 
		chan.cread(&response, sizeof(__int64_t));

		cout << "file length:" << response;

		// Gotten file length



		// Prepare file to write
		fstream ofs;
		ofs.open( "received/" + filename, ios::out | ios::trunc );

		ofstream writeFile("received/" + filename);

		if (!writeFile.is_open()) {
			cout << "invalid file name." << endl;
			return 1;
		}



		//start downloading the file through bits and bytes
		for ( __int64_t offsetStart = 0; offsetStart < response; offsetStart += MAX_MESSAGE) {

			filemsg fileRequestMsg(offsetStart, (int) min( (__int64_t) MAX_MESSAGE, response - offsetStart) );

			// Create a message that sends the filename AFTER everything aswell
			int len = sizeof(filemsg) + (filename.size() + 1);
			char* buf2 = new char[len];
			memcpy(buf2, &fileRequestMsg, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), filename.c_str());
			chan.cwrite(buf2, len);		
			delete[] buf2;
			// This should work now

			char* fileResponse[MAX_MESSAGE];
			chan.cread(&fileResponse, sizeof(fileResponse));

			// Start writing to file 

			writeFile << fileResponse;
			
		}

		writeFile.close();

	} else { // If not requesting full file

		
		if (p < 1 || p > 15) {
			cout << " P flag invalid or not entered." << endl;
		} else if (t == -1 || e == -1) { 
			cout << " t and or e flag is not entered. Sending entire p file. " << endl;

			fstream ofs;
			ofs.open( "received/x1.csv", ios::out | ios::trunc );

			ofstream writeFile("received/x1.csv");

			if (! writeFile.is_open()) {
				cout << "unable to write to received." << endl;
				return 1;
			}

			for ( double i = 0; i < 0.004 * 1000; i+= 0.004 ){ 
				//ecg1: 
				datamsg x1(p, i, 1);

				memcpy(dataBuffer, &x1, sizeof(datamsg));
				chan.cwrite(dataBuffer, sizeof(datamsg));

				double dataReply1; 
				chan.cread(&dataReply1, sizeof(double));

				//ecg2: 
				datamsg x2(p, i, 2);
				memcpy(dataBuffer, &x2, sizeof(datamsg));
				chan.cwrite(dataBuffer, sizeof(datamsg));

				double dataReply2; 
				chan.cread(&dataReply2, sizeof(double));

				writeFile << i << "," << dataReply1 << "," << dataReply2 << "\n";

			}

			writeFile.close();
			

		} else { 

			cout << "Proceeding as normal. " << endl;
			datamsg y(p, t, e); // Person, Seconds, Eno

			memcpy(dataBuffer, &y, sizeof(datamsg));
			chan.cwrite(dataBuffer, sizeof(datamsg));

			double dataReply; 
			chan.cread(&dataReply, sizeof(double));
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << dataReply << endl;
		}
	}
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	// cout << "cwritten" << endl;

	wait(&status); 
}
