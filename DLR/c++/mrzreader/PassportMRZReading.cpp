#include <stdio.h>
#include <zmq.hpp>
#include <json/json.h>
#include <string>
#include <vector>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "../../include/DynamsoftLabelRecognizer.h"
#include <iostream>

using namespace std;
using namespace dynamsoft::dlr;

#if defined(_WIN32) || defined(_WIN64)
#ifdef _WIN64
#pragma comment(lib, "../../lib/windows/x64/DynamsoftLabelRecognizerx64.lib")
#else
#pragma comment(lib, "../../lib/windows/x86/DynamsoftLabelRecognizerx86.lib")
#endif
#endif


int main()
{
    char szErrorMsg[512] = { 0 };
    // 1.Initialize license.
    CLabelRecognizer::InitLicense("DLS2eyJvcmdhbml6YXRpb25JRCI6IjEwMDIyNzc2MyJ9", szErrorMsg, 512);
    // 2.Create an instance of Label Recognizer.
    CLabelRecognizer dlr;
    // 3. Append config by a template json file.
    int ret = dlr.AppendSettingsFromFile("wholeImgMRZTemplate.json");
    printf("License initialization: %s\n\n", szErrorMsg);
    zmq::context_t zmq_context(1);
    zmq::socket_t zmq_socket(zmq_context, ZMQ_REP);
    zmq_socket.bind("tcp://*:5558");
    while (true)
    {
        zmq::message_t recv_msg;
        zmq_socket.recv(recv_msg, zmq::recv_flags::none);
        std::string msg = recv_msg.to_string();
        std::cout << msg + "\n";
        try {
            int errorCode = 0;
            struct stat buffer;
            if (stat(msg.c_str(), &buffer) == 0) {


                clock_t start, finish;
                double duration;
                start = clock();

                errorCode = dlr.RecognizeByFile(msg.c_str(), "locr");
                
                finish = clock();
                duration = (double)(finish - start) / CLOCKS_PER_SEC * 1000;
                std::string s = "";
                Json::Value rootValue = Json::objectValue;
                rootValue["results"] = Json::arrayValue;
                rootValue["elapsedTime"] = duration;
                DLR_ResultArray* pDLRResults = NULL;
                dlr.GetAllResults(&pDLRResults);
                if (pDLRResults != NULL)
                {
                    // 5. Output the raw text of MRZ.
                    int rCount = pDLRResults->resultsCount;
                    printf("\nRecognized %d results\n", rCount);
                    for (int ri = 0; ri < rCount; ++ri)
                    {
                        printf("\nResult %d :\n", ri);
                        DLR_Result* result = pDLRResults->results[ri];
                        int lCount = result->lineResultsCount;
                        for (int li = 0; li < lCount; ++li)
                        {
                            printf("Line result %d: %s\n", li, result->lineResults[li]->text);
                            Json::Value box = Json::objectValue;
                            
                            box["text"] = result->lineResults[li]->text;
                            box["x1"] = result->lineResults[li]->location.points[0].x;
                            box["x2"] = result->lineResults[li]->location.points[1].x;
                            box["x3"] = result->lineResults[li]->location.points[2].x;
                            box["x4"] = result->lineResults[li]->location.points[3].x;
                            box["y1"] = result->lineResults[li]->location.points[0].y;
                            box["y2"] = result->lineResults[li]->location.points[1].y;
                            box["y3"] = result->lineResults[li]->location.points[2].y;
                            box["y4"] = result->lineResults[li]->location.points[3].y;
                            rootValue["boxes"].append(box);
                        }
                    }

                }
                Json::StreamWriterBuilder builder;
                std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
                std::ostringstream os;
                writer->write(rootValue, &os);
                s = os.str();
                //s = Json::FastWriter().write(rootValue);
                zmq::message_t response;
                response.rebuild(s.c_str(), s.length());
                zmq_socket.send(response, zmq::send_flags::dontwait);
                dlr.FreeResults(&pDLRResults);
            }
            else {
                zmq_socket.send(zmq::str_buffer("Received"), zmq::send_flags::dontwait);
            }
    }
        catch (...) {
            std::cout << "error";
            zmq_socket.send(zmq::str_buffer("Error"), zmq::send_flags::dontwait);
        }

        if (msg == "q") {
            break;
        }
}
}
