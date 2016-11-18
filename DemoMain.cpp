#include <opendnp3/LogToStdio.h>
#include <opendnp3/DNP3Manager.h>
#include <opendnp3/SlaveStackConfig.h>
#include <opendnp3/IChannel.h>
#include <opendnp3/IOutstation.h>
#include <opendnp3/SimpleCommandHandler.h>
#include <opendnp3/TimeTransaction.h>
#include <opendnp3/SlaveConfig.h>
#include <iostream>
#include <stdio.h> //printf
#include <unistd.h>  //close  sleep
#include <string.h>    //strlen  strncpy
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include "rapidjson/document.h" // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/filestream.h" // wrapper of C stream for prettywriter as output
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <cstdio>
# include <pthread.h>
#include <vector>

using namespace rapidjson;
using namespace std;
using namespace opendnp3;

//-----------------------------------Thread Buffer----------------

void *Producer(void*);
void *Consumer(void*);
int sockRec;
int BufferIndex=-1;
vector<string> BUFFER;

pthread_cond_t Buffer_Not_Full=PTHREAD_COND_INITIALIZER;
pthread_cond_t Buffer_Not_Empty=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mVar=PTHREAD_MUTEX_INITIALIZER;

//---------------------------Initial Structure----------------------------------

//{"command":1,"detail":{"values":[{"BufferSize":2048},{"ThreadBufferSize":100}]}}
struct Config{

//command 1   initial Configuration
    static int BufferSize;
    static int ThreadBufferSize;
    static string JsonIP;
    static string DNP3MasterIP;
    static int JsonPort;
    static int DNP3MasterPort;
    static int DNP3LocalAddress;
    static int DNP3RemoteAddress;
    static bool DisableUnsol;
    static bool AllowTimeSync;
    static int TimeSyncPeriod;
    static int IsSerialTCP;

//command 2   Database size
    static int MaxBinaryNum;
    static int MaxAnalogNum;
    static int MaxCounterNum;
    static int MaxControlNum;
    static int MaxSetpointNum;

//command 3   Class + Deadband
    static int AnalogClass;
    static int BinaryClass;
    static int CounterClass;
    static int AnalogDeadband;

//command 4   Event Grp/Var
    static int EventBinaryGroup;
    static int EventBinaryVariation;
    static int EventAnalogGroup;
    static int EventAnalogVariation;
    static int EventCoutnerGroup;
    static int EventCounterVariation;

//command 5   static Grp/Var
    static int StaticBinaryGroup;
    static int StaticBinaryVariation;
    static int StaticAnalogGroup;
    static int StaticAnalogVariation;
    static int StaticCounterGroup;
    static int StaticCounterVariation;
    static int StaticSetpointGroup;
    static int StaticSetpointVariation;

//command 6   serial
    static int BaudRate;
    static int DataBits;
    static string SerialDevice;
    static int FlowType;
    static int Parity;
    static int StopBits;
//command 7 exit
    static int exitConfig;

//command 8 IsAlive
    static bool IsAlive;
};

int Config::BufferSize = 2048;
int Config::ThreadBufferSize = 100;
string Config::JsonIP = "127.0.0.1";
string Config::DNP3MasterIP = "0.0.0.0";
int Config::JsonPort = 4800;
int Config::DNP3MasterPort = 20000;
int Config::DNP3LocalAddress = 1024;
int Config::DNP3RemoteAddress = 1;
bool Config::DisableUnsol = false;
bool Config::AllowTimeSync = true;
int Config::TimeSyncPeriod = 60000;
int Config::IsSerialTCP = 2;// 		(Serial : 1 , TCP : 2)

//command 2   Database size
int Config::MaxBinaryNum = 5;
int Config::MaxAnalogNum = 5;
int Config::MaxCounterNum = 5;
int Config::MaxControlNum = 5;
int Config::MaxSetpointNum = 5;

//command 3   Class + Deadband
int Config::AnalogClass = 2;//		(Class0 : 1 , Class1 : 2 , Class2 : 4 , Class3 : 8)
int Config::BinaryClass = 2;
int Config::CounterClass = 4;
int Config::AnalogDeadband = 2;

//command 4   Event Grp/Var
int Config::EventBinaryGroup = 2;//			(Grp : 2 , Var : 1,2)
int Config::EventBinaryVariation = 2;
int Config::EventAnalogGroup = 32;//		(Grp : 32 , Var : 1,2,3,4,5,6,7,8)
int Config::EventAnalogVariation = 4;
int Config::EventCoutnerGroup = 22;//		(Grp : 22 , Var : 1,2,5,6)
int Config::EventCounterVariation = 2;

//command 5   static Grp/Var
int Config::StaticBinaryGroup = 1;//		(Grp : 1 , Var: 2)
int Config::StaticBinaryVariation = 2;
int Config::StaticAnalogGroup = 30;//		(Grp : 30 , Var : 1,2,3,4,5,6)
int Config::StaticAnalogVariation = 1;
int Config::StaticCounterGroup = 20;//		(Grp : 20 , Var : 1,2,5,6)
int Config::StaticCounterVariation = 1;
int Config::StaticSetpointGroup = 40;//		(Grp : 40 , Var : 1,2,3,4)
int Config::StaticSetpointVariation = 1;

//command 6   serial
int Config::BaudRate = 9600;
int Config::DataBits = 8;
string Config::SerialDevice = "COM4";
int Config::FlowType = 0;//	(FLOW_NONE = 0,FLOW_HARDWARE = 1, FLOW_XONXOFF = 2)
int Config::Parity = 2;//	(PAR_NONE = 0,PAR_EVEN = 1,PAR_ODD = 2)
int Config::StopBits = 1;

//command 7 exit
int Config::exitConfig = 0; //(stay in Cinfig = 0 , exit from Config > 0)

//command 8 IsAlive
bool Config::IsAlive = true; //      (life :true , dead : false)

//------------------------------------------------------------------------------




class MyCommandHandler : public opendnp3::ICommandHandler {

private:

IDataObserver* mpObserver;

public:

//MyCommandHandler();
//~MyCommandHandler();

opendnp3::CommandStatus Select( const opendnp3::AnalogOutputInt16& arCommand, size_t aIndex );
opendnp3::CommandStatus Operate( const opendnp3::AnalogOutputInt16& arCommand, size_t aIndex );
//opendnp3::CommandStatus SelectAndOperate( const opendnp3::AnalogOutputInt16& arCommand, size_t aIndex );
opendnp3::CommandStatus DirectOperate(const opendnp3::AnalogOutputInt16& arCommand, size_t aIndex);

opendnp3::CommandStatus Select( const opendnp3::AnalogOutputInt32& arCommand, size_t aIndex );
opendnp3::CommandStatus Operate( const opendnp3::AnalogOutputInt32& arCommand, size_t aIndex );
//opendnp3::CommandStatus SelectAndOperate( const opendnp3::AnalogOutputInt32& arCommand, size_t aIndex );
opendnp3::CommandStatus DirectOperate(const opendnp3::AnalogOutputInt32& arCommand, size_t aIndex);

opendnp3::CommandStatus Select( const opendnp3::AnalogOutputFloat32& arCommand, size_t aIndex );
opendnp3::CommandStatus Operate( const opendnp3::AnalogOutputFloat32& arCommand, size_t aIndex );
//opendnp3::CommandStatus SelectAndOperate( const opendnp3::AnalogOutputFloat32& arCommand, size_t aIndex );
opendnp3::CommandStatus DirectOperate(const opendnp3::AnalogOutputFloat32& arCommand, size_t aIndex);

opendnp3::CommandStatus Select( const opendnp3::AnalogOutputDouble64& arCommand, size_t aIndex );
opendnp3::CommandStatus Operate( const opendnp3::AnalogOutputDouble64& arCommand, size_t aIndex );
//opendnp3::CommandStatus SelectAndOperate( const opendnp3::AnalogOutputDouble64& arCommand, size_t aIndex );
opendnp3::CommandStatus DirectOperate(const opendnp3::AnalogOutputDouble64& arCommand, size_t aIndex);

opendnp3::CommandStatus Select( const opendnp3::ControlRelayOutputBlock& arCommand, size_t aIndex );
opendnp3::CommandStatus Operate( const opendnp3::ControlRelayOutputBlock& arCommand, size_t aIndex );
//opendnp3::CommandStatus SelectAndOperate( const opendnp3::ControlRelayOutputBlock& arCommand, size_t aIndex );
opendnp3::CommandStatus DirectOperate(const opendnp3::ControlRelayOutputBlock& arCommand, size_t aIndex);

opendnp3::CommandStatus ValidateCROB(const opendnp3::ControlRelayOutputBlock& arCommand, size_t aIndex);
opendnp3::CommandStatus ValidateAnalogInt16(const opendnp3::AnalogOutputInt16& arCommand, size_t aIndex);
opendnp3::CommandStatus ValidateAnalogInt32(const opendnp3::AnalogOutputInt32& arCommand, size_t aIndex);
opendnp3::CommandStatus ValidateAnalogFloat32(const opendnp3::AnalogOutputFloat32& arCommand, size_t aIndex);
opendnp3::CommandStatus ValidateAnalogDouble64(const opendnp3::AnalogOutputDouble64& arCommand, size_t aIndex);

void DoOperate(const opendnp3::ControlRelayOutputBlock& arCommand, size_t aIndex);
void DoOperate(const opendnp3::AnalogOutputInt16& arCommand, size_t aIndex);
void DoOperate(const opendnp3::AnalogOutputInt32& arCommand, size_t aIndex);
void DoOperate(const opendnp3::AnalogOutputFloat32& arCommand, size_t aIndex);
void DoOperate(const opendnp3::AnalogOutputDouble64& arCommand, size_t aIndex);
void GetpObserver(IDataObserver* pObserver);
//void GetSock(int mSock);
// ... other virtual functions
};
void MyCommandHandler::GetpObserver(IDataObserver* pObserver){

    mpObserver = pObserver;

}
//void MyCommandHandler::GetSock(int mSock){

  //  sock = mSock;

//}
//////////////AnalogOutputInt16//////////
CommandStatus MyCommandHandler::Select( const AnalogOutputInt16& arCommand, size_t aIndex ) {
 // do select of 16bit analog output
 std::cout <<"CommandStatus = "<<ValidateAnalogInt16(arCommand, aIndex)<< std::endl;
	return ValidateAnalogInt16(arCommand, aIndex);
}
CommandStatus MyCommandHandler::Operate( const AnalogOutputInt16& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogInt16(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
CommandStatus MyCommandHandler::DirectOperate( const AnalogOutputInt16& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogInt16(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
///////////////AnalogOutputInt32/////////////
CommandStatus MyCommandHandler::Select( const AnalogOutputInt32& arCommand, size_t aIndex ) {
 // do select of 16bit analog output
 std::cout <<"CommandStatus = "<<ValidateAnalogInt32(arCommand, aIndex)<< std::endl;
	return ValidateAnalogInt32(arCommand, aIndex);
}
CommandStatus MyCommandHandler::Operate( const AnalogOutputInt32& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogInt32(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
CommandStatus MyCommandHandler::DirectOperate( const AnalogOutputInt32& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogInt32(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
/////////////AnalogOutputFloat32///////////
CommandStatus MyCommandHandler::Select( const AnalogOutputFloat32& arCommand, size_t aIndex ) {
 // do select of 16bit analog output
 std::cout <<"CommandStatus = "<<ValidateAnalogFloat32(arCommand, aIndex)<< std::endl;
	return ValidateAnalogFloat32(arCommand, aIndex);
}
CommandStatus MyCommandHandler::Operate( const AnalogOutputFloat32& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogFloat32(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
CommandStatus MyCommandHandler::DirectOperate( const AnalogOutputFloat32& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogFloat32(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
/////////////AnalogOutputDouble64//////////
CommandStatus MyCommandHandler::Select( const AnalogOutputDouble64& arCommand, size_t aIndex ) {
 // do select of 16bit analog output
 std::cout <<"CommandStatus = "<<ValidateAnalogDouble64(arCommand, aIndex)<< std::endl;
	return ValidateAnalogDouble64(arCommand, aIndex);
}
CommandStatus MyCommandHandler::Operate( const AnalogOutputDouble64& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogDouble64(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
CommandStatus MyCommandHandler::DirectOperate( const AnalogOutputDouble64& arCommand, size_t aIndex ) {
 // do operate part of 16bit analog output
 CommandStatus validation = ValidateAnalogDouble64(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
//////////////ControlRelayOutputBlock/////////////
CommandStatus MyCommandHandler::Select( const ControlRelayOutputBlock& arCommand, size_t aIndex ) {
	std::cout <<"CommandStatus = "<<ValidateCROB(arCommand, aIndex)<< std::endl;
	return ValidateCROB(arCommand, aIndex);
}
CommandStatus MyCommandHandler::Operate( const ControlRelayOutputBlock& arCommand, size_t aIndex ) {

	CommandStatus validation = ValidateCROB(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}

CommandStatus MyCommandHandler::DirectOperate(const ControlRelayOutputBlock& arCommand, size_t aIndex)
{
	CommandStatus validation = ValidateCROB(arCommand, aIndex);
	std::cout <<"CommandStatus = "<<validation<< std::endl;
	if(validation == CS_SUCCESS) DoOperate(arCommand, static_cast<char>(aIndex));
	return validation;
}
//////////////////////////////////
CommandStatus MyCommandHandler::ValidateCROB(const ControlRelayOutputBlock& arCommand, size_t aIndex)
{
	if(aIndex < Config::MaxControlNum) {
		if(arCommand.GetCode() == CC_LATCH_ON || arCommand.GetCode() == CC_LATCH_OFF || arCommand.GetCode() == CC_PULSE || arCommand.GetCode() == CC_PULSE_CLOSE || arCommand.GetCode() == CC_PULSE_TRIP ) return CS_SUCCESS;
		else {std::cout <<"CC_LATCH_ON/OFF Fail"<< std::endl;return CS_NOT_SUPPORTED;}
	}
	else {std::cout <<"aIndex <8 fail"<< std::endl;return CS_NOT_SUPPORTED;}
}

CommandStatus MyCommandHandler::ValidateAnalogInt16(const AnalogOutputInt16& arCommand, size_t aIndex)
{
	if(aIndex < Config::MaxSetpointNum) {
		if(arCommand.GetValue() >=0 ) return CS_SUCCESS;
		else {std::cout <<"Value AnalogOutputInt16 Fail"<< std::endl;return CS_NOT_SUPPORTED;}
	}
	else {std::cout <<"aIndex <8 fail"<< std::endl;return CS_NOT_SUPPORTED;}
}

CommandStatus MyCommandHandler::ValidateAnalogInt32(const AnalogOutputInt32& arCommand, size_t aIndex)
{
	if(aIndex < Config::MaxSetpointNum) {
		if(arCommand.GetValue() >=0 ) return CS_SUCCESS;
		else {std::cout <<"Value AnalogOutputInt32 Fail"<< std::endl;return CS_NOT_SUPPORTED;}
	}
	else {std::cout <<"aIndex <8 fail"<< std::endl;return CS_NOT_SUPPORTED;}
}

CommandStatus MyCommandHandler::ValidateAnalogFloat32(const AnalogOutputFloat32& arCommand, size_t aIndex)
{
	if(aIndex < Config::MaxSetpointNum) {
		if(arCommand.GetValue() >=0 ) return CS_SUCCESS;
		else {std::cout <<"Value AnalogOutputFloat32 Fail"<< std::endl;return CS_NOT_SUPPORTED;}
	}
	else {std::cout <<"aIndex <8 fail"<< std::endl;return CS_NOT_SUPPORTED;}
}

CommandStatus MyCommandHandler::ValidateAnalogDouble64(const AnalogOutputDouble64& arCommand, size_t aIndex)
{
	if(aIndex < Config::MaxSetpointNum) {
		if(arCommand.GetValue() >=0 ) return CS_SUCCESS;
		else {std::cout <<"Value AnalogOutputDouble64 Fail"<< std::endl;return CS_NOT_SUPPORTED;}
	}
	else {std::cout <<"aIndex <8 fail"<< std::endl;return CS_NOT_SUPPORTED;}
}
void MyCommandHandler::DoOperate(const ControlRelayOutputBlock& arCommand, size_t aIndex)
{
    std::cout <<"ControlRelayOutputBlock Value = "<<arCommand.ToString()<<"  at port : "<<aIndex<< std::endl;
	char value = 0;
	//LOG_BLOCK(LEV_INFO, "Received " << aControl.ToString() << " on index: " << aIndex);

//command 0 is update
//{"command":0, "detail":{"adresses":[{"group":23, "index":4}, {"group":12, "index":0}, {"group":12, "index":3}], "values":[{"kind":"int", "val":3}, {"kind":"pulse", "onTime":100, "offTime":100, "count":2}, {"kind":"latch_on"}]}}
    Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    Value addressArray(kArrayType);
    Value myValueArray(kArrayType);

    Value myAddressObject(kObjectType);
    myAddressObject.SetObject();

    Value myValueObject(kObjectType);
    myValueObject.SetObject();

    Value myDetailObject(kObjectType);
    myDetailObject.SetObject();

    myAddressObject.AddMember("group", 12, allocator);
    myAddressObject.AddMember("index", aIndex, allocator);
    addressArray.PushBack(myAddressObject, allocator);

    if(arCommand.GetCode() == CC_LATCH_ON)
        myValueObject.AddMember("val", "CC_LATCH_ON", allocator);
    else if(arCommand.GetCode() == CC_LATCH_OFF)
        myValueObject.AddMember("val", "CC_LATCH_OFF",allocator);
    else if(arCommand.GetCode() == CC_PULSE)
        myValueObject.AddMember("val", "CC_PULSE",allocator);
    else if(arCommand.GetCode() == CC_PULSE_CLOSE)
        myValueObject.AddMember("val", "CC_PULSE_CLOSE",allocator);
    else if(arCommand.GetCode() == CC_PULSE_TRIP)
        myValueObject.AddMember("val", "CC_PULSE_TRIP",allocator);
    myValueObject.AddMember("onTime", arCommand.mOnTimeMS, allocator);
    myValueObject.AddMember("offTime", arCommand.mOffTimeMS, allocator);
    myValueObject.AddMember("count", (int)arCommand.mCount, allocator);
    myValueArray.PushBack(myValueObject, allocator);

    myDetailObject.AddMember("addresses", addressArray, allocator);
    myDetailObject.AddMember("values", myValueArray, allocator);
    document.AddMember("command", 0, allocator);
    document.AddMember("detail", myDetailObject, allocator);

	StringBuffer f;
	Writer<StringBuffer> writer(f);
	document.Accept(writer);

    string ss= f.GetString();
    ss += "\n" ;

    //printf("\%s.\n",ss.c_str());
    if( send(sockRec , ss.c_str() , strlen(ss.c_str()), 0) < 0)
           puts("Send failed");
    //close(sock);
    TimeTransaction tx(mpObserver);
    tx.Update(SetpointStatus (arCommand.GetCode(),TQ_ONLINE), aIndex);

}

void MyCommandHandler::DoOperate(const AnalogOutputInt16& arCommand, size_t aIndex)
{

	std::cout <<arCommand.ToString()<<"   at port : "<<aIndex<< std::endl;
//command 0 is update
//{"command":0, "detail":{"adresses":[{"group":23, "index":4}, {"group":12, "index":0}, {"group":12, "index":3}], "values":[{"kind":"int", "val":3}, {"kind":"pulse", "onTime":100, "offTime":100, "count":2}, {"kind":"latch_on"}]}}

    Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    Value addressArray(kArrayType);
    Value myValueArray(kArrayType);

    Value myAddressObject(kObjectType);
    myAddressObject.SetObject();

    Value myValueObject(kObjectType);
    myValueObject.SetObject();

    Value myDetailObject(kObjectType);
    myDetailObject.SetObject();

    myAddressObject.AddMember("group", 41, allocator);
    myAddressObject.AddMember("index", aIndex, allocator);
    addressArray.PushBack(myAddressObject, allocator);

    myValueObject.AddMember("kind", "int",allocator);
    myValueObject.AddMember("val", arCommand.GetValue(), allocator);
    myValueArray.PushBack(myValueObject, allocator);

    myDetailObject.AddMember("addresses", addressArray, allocator);
    myDetailObject.AddMember("values", myValueArray, allocator);
    document.AddMember("command", 0, allocator);
    document.AddMember("detail", myDetailObject, allocator);

	StringBuffer f;
	Writer<StringBuffer> writer(f);
	document.Accept(writer);
	//memcpy(json, f, sizeof(f));

    string ss= f.GetString() ;
    ss += "\n" ;
    if( send(sockRec , ss.c_str() , strlen(ss.c_str()), 0) < 0)
            puts("Send failed");
    //std::cout << arCommand.ToString() << std::endl;
    TimeTransaction tx(mpObserver);
    tx.Update(SetpointStatus (arCommand.GetValue(),PQ_ONLINE), aIndex);

	//pCmdProcessor->SelectAndOperate(AnalogOutputInt32(10), 0, [&](CommandResponse cr) {selectResult.set_value(cr);  cout<<"\nDONE\n"; });
}

void MyCommandHandler::DoOperate(const AnalogOutputInt32& arCommand, size_t aIndex)
{

	std::cout <<"AnalogOutputInt32 Value = "<<arCommand.GetValue()<<"   at port : "<<aIndex<< std::endl;
//command 0 is update
//{"command":0, "detail":{"adresses":[{"group":23, "index":4}, {"group":12, "index":0}, {"group":12, "index":3}], "values":[{"kind":"int", "val":3}, {"kind":"pulse", "onTime":100, "offTime":100, "count":2}, {"kind":"latch_on"}]}}

    Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    Value addressArray(kArrayType);
    Value myValueArray(kArrayType);

    Value myAddressObject(kObjectType);
    myAddressObject.SetObject();

    Value myValueObject(kObjectType);
    myValueObject.SetObject();

    Value myDetailObject(kObjectType);
    myDetailObject.SetObject();

    myAddressObject.AddMember("group", 41, allocator);
    myAddressObject.AddMember("index", aIndex, allocator);
    addressArray.PushBack(myAddressObject, allocator);

    myValueObject.AddMember("kind", "int",allocator);
    myValueObject.AddMember("val", arCommand.GetValue(), allocator);
    myValueArray.PushBack(myValueObject, allocator);

    myDetailObject.AddMember("addresses", addressArray, allocator);
    myDetailObject.AddMember("values", myValueArray, allocator);
    document.AddMember("command", 0, allocator);
    document.AddMember("detail", myDetailObject, allocator);

	StringBuffer f;
	Writer<StringBuffer> writer(f);
	document.Accept(writer);
	//memcpy(json, f, sizeof(f));

    string ss= f.GetString() ;
    ss += "\n" ;
    if( send(sockRec , ss.c_str() , strlen(ss.c_str()), 0) < 0)
            puts("Send failed");
    //std::cout << arCommand.ToString() << std::endl;
    TimeTransaction tx(mpObserver);
    tx.Update(SetpointStatus (arCommand.GetValue(),PQ_ONLINE), aIndex);

}

void MyCommandHandler::DoOperate(const AnalogOutputFloat32& arCommand, size_t aIndex)
{

	std::cout <<"AnalogOutputFloat32 Value = "<<arCommand.GetValue()<<"   at port : "<<aIndex<< std::endl;
//command 0 is update
//{"command":0, "detail":{"adresses":[{"group":23, "index":4}, {"group":12, "index":0}, {"group":12, "index":3}], "values":[{"kind":"int", "val":3}, {"kind":"pulse", "onTime":100, "offTime":100, "count":2}, {"kind":"latch_on"}]}}

    Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    Value addressArray(kArrayType);
    Value myValueArray(kArrayType);

    Value myAddressObject(kObjectType);
    myAddressObject.SetObject();

    Value myValueObject(kObjectType);
    myValueObject.SetObject();

    Value myDetailObject(kObjectType);
    myDetailObject.SetObject();

    myAddressObject.AddMember("group", 41, allocator);
    myAddressObject.AddMember("index", aIndex, allocator);
    addressArray.PushBack(myAddressObject, allocator);

    myValueObject.AddMember("kind", "double",allocator);
    myValueObject.AddMember("val", arCommand.GetValue(), allocator);
    myValueArray.PushBack(myValueObject, allocator);

    myDetailObject.AddMember("addresses", addressArray, allocator);
    myDetailObject.AddMember("values", myValueArray, allocator);
    document.AddMember("command", 0, allocator);
    document.AddMember("detail", myDetailObject, allocator);

	StringBuffer f;
	Writer<StringBuffer> writer(f);
	document.Accept(writer);
	//memcpy(json, f, sizeof(f));

    string ss= f.GetString() ;
    ss += "\n" ;
    if( send(sockRec , ss.c_str() , strlen(ss.c_str()), 0) < 0)
            puts("Send failed");
    //std::cout << arCommand.ToString() << std::endl;
    TimeTransaction tx(mpObserver);
    tx.Update(SetpointStatus (arCommand.GetValue(),PQ_ONLINE), aIndex);

}

void MyCommandHandler::DoOperate(const AnalogOutputDouble64& arCommand, size_t aIndex)
{

	std::cout <<"AnalogOutputFloat32 Value = "<<arCommand.GetValue()<<"   at port : "<<aIndex<< std::endl;
//command 0 is update
//{"command":0, "detail":{"adresses":[{"group":23, "index":4}, {"group":12, "index":0}, {"group":12, "index":3}], "values":[{"kind":"int", "val":3}, {"kind":"pulse", "onTime":100, "offTime":100, "count":2}, {"kind":"latch_on"}]}}

    Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    Value addressArray(kArrayType);
    Value myValueArray(kArrayType);

    Value myAddressObject(kObjectType);
    myAddressObject.SetObject();

    Value myValueObject(kObjectType);
    myValueObject.SetObject();

    Value myDetailObject(kObjectType);
    myDetailObject.SetObject();

    myAddressObject.AddMember("group", 41, allocator);
    myAddressObject.AddMember("index", aIndex, allocator);
    addressArray.PushBack(myAddressObject, allocator);

    myValueObject.AddMember("kind", "double",allocator);
    myValueObject.AddMember("val", arCommand.GetValue(), allocator);
    myValueArray.PushBack(myValueObject, allocator);

    myDetailObject.AddMember("addresses", addressArray, allocator);
    myDetailObject.AddMember("values", myValueArray, allocator);
    document.AddMember("command", 0, allocator);
    document.AddMember("detail", myDetailObject, allocator);

	StringBuffer f;
	Writer<StringBuffer> writer(f);
	document.Accept(writer);
	//memcpy(json, f, sizeof(f));

    string ss= f.GetString() ;
    ss += "\n" ;
    if( send(sockRec , ss.c_str() , strlen(ss.c_str()), 0) < 0)
            puts("Send failed");
    //std::cout << arCommand.ToString() << std::endl;
    TimeTransaction tx(mpObserver);
    tx.Update(SetpointStatus (arCommand.GetValue(),PQ_ONLINE), aIndex);

}

int main(int argc, char* argv[])
{

	// Specify a FilterLevel for the stack/physical layer to use.
	// Log statements with a lower priority will not be logged.
	const FilterLevel LOG_LEVEL = LEV_EVENT;
	// This is the main point of interaction with the stack
	DNP3Manager mgr(1); // only 1 thread is needed for a single stack

	// You can optionally subcribe to log messages
	// This singleton logger just prints messages to the console
	mgr.AddLogSubscriber(LogToStdio::Inst());

    //Socket client part
    	//int sockRec;
    struct sockaddr_in serverRec;

    	//char message[Config::BufferSize];

   	 //Create socket
    sockRec = socket(AF_INET , SOCK_STREAM , 0);

    if (sockRec == -1)
       	printf("Could not create socket");

    puts("Socket created");
    serverRec.sin_addr.s_addr = inet_addr(Config::JsonIP.c_str());
    serverRec.sin_family = AF_INET;
    serverRec.sin_port = htons( Config::JsonPort );
    //Connect to remote server
    while (connect(sockRec , (struct sockaddr *)&serverRec , sizeof(serverRec)) < 0){
        perror("connect failed. Error");
		sleep(1);
        continue;
    }

    puts("Connected\n");
    char *server_reply = new char[Config::BufferSize];
    while(true){
        struct sockaddr_in serverRec;
        int byte=recv(sockRec , server_reply , Config::BufferSize , 0);
        if(  byte <= 0){
            puts("recv failed");
            close(sockRec);
            sockRec = socket(AF_INET , SOCK_STREAM , 0);
            if (sockRec == -1)
                printf("Could not create socket");
            puts("Socket created");
            serverRec.sin_addr.s_addr = inet_addr(Config::JsonIP.c_str());
            serverRec.sin_family = AF_INET;
            serverRec.sin_port = htons( Config::JsonPort );
            while (connect(sockRec , (struct sockaddr *)&serverRec , sizeof(serverRec)) < 0){

                perror("connect failed. Error");
                sleep(1);
            }

            puts("Connected\n");
        }
        else{
            server_reply[byte] = '\0';
            Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
            #if 0
                if (document.Parse(server_reply).HasParseError()){
                    printf("\nPARSING ERROR.\n");
                    return 1;
                }
            #else
// In-situ parsing, decode strings directly in the source string. Source must be string.
            {
                if (document.ParseInsitu(server_reply).HasParseError()){
                    printf("\nParsing Error.\n");
                    continue;
                }
            }
            #endif
            printf("\nParsing to document succeeded.\n");
            Value::MemberIterator req = document.FindMember("detail");
            if(req != document.MemberEnd()){
                printf("\n Read JSON \n");
                const Value& a = document["detail"]["values"];
                SizeType num = a.Size();
                for(SizeType j = 0; j < num; j++){
                    Value& v = document["detail"]["values"];
                    Value& ad = document["detail"]["addresses"];
                    SizeType addNum = ad.Size();
                    SizeType valueNum = v.Size();
                    if (valueNum > 0 && addNum > 0 && document["command"].GetInt() == 1){
                    //-----------------------------BufferSize--------------------------------------------------
                        cout<<"--- Reading Commmand 1 ----"<<endl;
                        if (document["detail"]["addresses"][j]["Name"].GetString() == string("BufferSize"))
                            Config::BufferSize = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------ThreadBufferSize--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("ThreadBufferSize"))
                            Config::ThreadBufferSize = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------JsonIP--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("JsonIP"))
                            Config::JsonIP = document["detail"]["values"][j]["val"].GetString();
                    //-----------------------------DNP3MasterIP--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("DNP3MasterIP"))
                            Config::DNP3MasterIP = document["detail"]["values"][j]["val"].GetString();
                    //-----------------------------JsonPort--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("JsonPort"))
                            Config::JsonPort = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------DNP3MasterPort--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("DNP3MasterPort"))
                            Config::DNP3MasterPort = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------DNP3LocalAddress--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("DNP3LocalAddress"))
                            Config::DNP3LocalAddress = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------DNP3RemoteAddress--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("DNP3RemoteAddress"))
                            Config::DNP3RemoteAddress = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------DisableUnsol--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("DisableUnsol"))
                            Config::DisableUnsol = document["detail"]["values"][j]["val"].GetBool();
                    //-----------------------------AllowTimeSync--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("AllowTimeSync"))
                            Config::AllowTimeSync = document["detail"]["values"][j]["val"].GetBool();
                    //-----------------------------TimeSyncPeriod--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("TimeSyncPeriod"))
                            Config::TimeSyncPeriod = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------IsSerialTCP--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("IsSerialTCP"))
                            Config::IsSerialTCP = document["detail"]["values"][j]["val"].GetInt();

                    }
                    else if (valueNum > 0 && document["command"].GetInt() == 2){
                   //-----------------------------MaxBinaryNum--------------------------------------------------
                        cout<<"--- Reading Commmand 2 ----"<<endl;
                        if (document["detail"]["addresses"][j]["Name"].GetString() == string("MaxBinaryNum"))
                            Config::MaxBinaryNum = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------MaxAnalogNum--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("MaxAnalogNum"))
                            Config::MaxAnalogNum = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------MaxCounterNum--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("MaxCounterNum"))
                            Config::MaxCounterNum = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------MaxControlNum--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("MaxControlNum"))
                            Config::MaxControlNum = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------MaxSetpointNum--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("MaxSetpointNum"))
                            Config::MaxSetpointNum = document["detail"]["values"][j]["val"].GetInt();
                    }
                    else if (valueNum > 0 && document["command"].GetInt() == 3){
                    //-----------------------------AnalogClass--------------------------------------------------
                        cout<<"--- Reading Commmand 3 ----"<<endl;
                        if (document["detail"]["addresses"][j]["Name"].GetString() == string("AnalogClass"))
                            Config::AnalogClass = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------BinaryClass--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("BinaryClass"))
                            Config::BinaryClass = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------CounterClass--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("CounterClass"))
                            Config::CounterClass = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------AnalogDeadband--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("AnalogDeadband"))
                            Config::AnalogDeadband = document["detail"]["values"][j]["val"].GetInt();
                    }
                    else if (valueNum > 0 && document["command"].GetInt() == 4){
                    //-----------------------------EventBinaryGroup--------------------------------------------------
                        cout<<"--- Reading Commmand 4 ----"<<endl;
                        if (document["detail"]["addresses"][j]["Name"].GetString() == string("EventBinaryGroup"))
                            Config::EventBinaryGroup = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------EventBinaryVariation--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("EventBinaryVariation"))
                            Config::EventBinaryVariation = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------EventAnalogGroup--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("EventAnalogGroup"))
                            Config::EventAnalogGroup = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------EventAnalogVariation--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("EventAnalogVariation"))
                            Config::EventAnalogVariation = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------EventCoutnerGroup--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("EventCoutnerGroup"))
                            Config::EventCoutnerGroup = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------EventCounterVariation--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("EventCounterVariation"))
                            Config::EventCounterVariation = document["detail"]["values"][j]["val"].GetInt();
                    }
                    else if (valueNum > 0 && document["command"].GetInt() == 5){
                    //-----------------------------StaticBinaryGroup--------------------------------------------------
                        cout<<"--- Reading Commmand 5 ----"<<endl;
                        if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticBinaryGroup"))
                            Config::StaticBinaryGroup = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StaticBinaryVariation--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticBinaryVariation"))
                            Config::StaticBinaryVariation = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StaticAnalogGroup--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticAnalogGroup"))
                            Config::StaticAnalogGroup = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StaticAnalogVariation--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticAnalogVariation"))
                            Config::StaticAnalogVariation = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StaticCounterGroup--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticCounterGroup"))
                            Config::StaticCounterGroup = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StaticCounterVariation--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticCounterVariation"))
                            Config::StaticCounterVariation = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StaticSetpointGroup--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticSetpointGroup"))
                            Config::StaticSetpointGroup = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StaticSetpointVariation--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StaticSetpointVariation"))
                            Config::StaticSetpointVariation = document["detail"]["values"][j]["val"].GetInt();
                    }
                    else if (valueNum > 0 && document["command"].GetInt() == 6){
                    //-----------------------------BaudRate--------------------------------------------------
                        cout<<"--- Reading Commmand 6 ----"<<endl;
                        if (document["detail"]["addresses"][j]["Name"].GetString() == string("BaudRate"))
                            Config::BaudRate = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------DataBits--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("DataBits"))
                            Config::DataBits = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------SerialDevice--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("SerialDevice"))
                            Config::SerialDevice = document["detail"]["values"][j]["val"].GetString();
                    //-----------------------------FlowType--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("FlowType"))
                            Config::FlowType = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------Parity--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("Parity"))
                            Config::Parity = document["detail"]["values"][j]["val"].GetInt();
                    //-----------------------------StopBits--------------------------------------------------
                        else if (document["detail"]["addresses"][j]["Name"].GetString() == string("StopBits"))
                            Config::StopBits = document["detail"]["values"][j]["val"].GetInt();
                    }
                    else if (document["command"].GetInt() == 7){
                        cout<<"--- Reading Commmand 7 ----"<<endl;
                        Config::exitConfig = 1;
                        break;
                    }
                    //-----------------------------IsAlive--------------------------------------------------
                    else if (document["command"].GetInt() == 8){
                            cout<<"--- Reading Commmand 8 ----"<<endl;
                            Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
                            document.SetObject();
                            Document::AllocatorType& allocator = document.GetAllocator();
                            Value addressArray(kArrayType);
                            Value myValueArray(kArrayType);

                            Value myAddressObject(kObjectType);
                            myAddressObject.SetObject();

                            Value myValueObject(kObjectType);
                            myValueObject.SetObject();

                            Value myDetailObject(kObjectType);
                            myDetailObject.SetObject();

                            myAddressObject.AddMember("Name", "IsAlive", allocator);
                            addressArray.PushBack(myAddressObject, allocator);

                            myValueObject.AddMember("val", true, allocator);
                            myValueArray.PushBack(myValueObject, allocator);

                            myDetailObject.AddMember("addresses", addressArray, allocator);
                            myDetailObject.AddMember("values", myValueArray, allocator);
                            document.AddMember("command", 8, allocator);
                            document.AddMember("detail", myDetailObject, allocator);

                            StringBuffer f;
                            Writer<StringBuffer> writer(f);
                            document.Accept(writer);
                                //memcpy(json, f, sizeof(f));

                            string ss= f.GetString() ;
                            ss += "\n" ;
                            if( send(sockRec , ss.c_str() , strlen(ss.c_str()), 0) < 0)
                                puts("Send failed");
                    }

               }//for JSON array
               if(Config::exitConfig > 0) break;
            }//if json Member
        }//else  recv
	}//while true
	cout<<"Configuring DNP3 has been finished ."<<endl;
	//if(Config::IsSerialTCP == 2){
	// Add a TCPSclose(sock);erver to the manager with the name "tcpserver".
	// The server will wait 3000 ms in between failed bind calls.
        auto pServer = mgr.AddTCPServer("tcpserver", LOG_LEVEL, 5000, Config::DNP3MasterIP.c_str(), Config::DNP3MasterPort);
        pServer->AddStateListener([](ChannelState state) {
            std::cout << "Server state: " << ConvertChannelStateToString(state) << std::endl;
        });
    //}

    if(Config::IsSerialTCP == 1){
        SerialSettings aSettings ;
	    aSettings.mBaud = Config::BaudRate;
	    aSettings.mDataBits = Config::DataBits;
	    aSettings.mDevice = Config::SerialDevice;   ///   dev/tty0
        if (Config::FlowType == 0)
            aSettings.mFlowType = FLOW_NONE ;
        else if (Config::FlowType == 1)
            aSettings.mFlowType = FLOW_HARDWARE  ;
        else if (Config::FlowType == 2)
            aSettings.mFlowType = FLOW_XONXOFF;

        if (Config::Parity == 0)
            aSettings.mParity = PAR_NONE ;
        else if (Config::Parity == 1)
            aSettings.mParity = PAR_EVEN  ;
        else if (Config::Parity == 2)
            aSettings.mParity = PAR_ODD   ;

        aSettings.mStopBits = Config::StopBits;
        pServer = mgr.AddSerial("serialconnect1", LOG_LEVEL, 5000, aSettings);

	// You can optionally add a listener to the channel. You can do this anytime and
	// you will receive a stream of all state changes
        pServer->AddStateListener([](ChannelState state) {
            std::cout << "Server state: " << ConvertChannelStateToString(state) << std::endl;
        });
    }
	// The master config object for a slave. The default are
	// useable, but understanding the options are important.
	SlaveStackConfig stackConfig;
	stackConfig.link.LocalAddr = Config::DNP3LocalAddress;
	stackConfig.link.RemoteAddr = Config::DNP3RemoteAddress;

	// The DeviceTemplate struct specifies the structure of the
	// slave's database

	DeviceTemplate device(Config::MaxBinaryNum , Config::MaxAnalogNum , Config::MaxCounterNum , Config::MaxControlNum , Config::MaxSetpointNum);
	stackConfig.device = device;
	stackConfig.device.mStartOnline = true;
	stackConfig.slave.mDisableUnsol = Config::DisableUnsol;
	stackConfig.slave.mAllowTimeSync = Config::AllowTimeSync;
	stackConfig.slave.mTimeSyncPeriod = Config::TimeSyncPeriod ; // every 60 seconds
//----------------------------------------------------------------------------------------------------------------------
	for (int i=0;i < Config::MaxBinaryNum ;i++)
		stackConfig.device.mBinary[i].EventClass = IntToPointClass(Config::BinaryClass);

	for (int i=0;i < Config::MaxAnalogNum ;i++){
		stackConfig.device.mAnalog[i].EventClass = IntToPointClass(Config::AnalogClass);
		stackConfig.device.mAnalog[i].Deadband = Config::AnalogDeadband;
    }
	for (int i=0;i < Config::MaxCounterNum ;i++)
		stackConfig.device.mCounter[i].EventClass = IntToPointClass(Config::CounterClass);

//-----------------------------------------------------------------------------------------------------------------------
	switch (Config::EventBinaryGroup){
	case 2:
		if (Config::EventBinaryVariation == 1)
			stackConfig.slave.mEventBinary=EBR_GROUP2_VAR1;     //Single bit binary input event without time
		else if(Config::EventBinaryVariation == 2)
			stackConfig.slave.mEventBinary=EBR_GROUP2_VAR2;     //Single bit binary input event with time
	break;
	}
	switch (Config::EventAnalogGroup){
	case 32:
		if (Config::EventAnalogVariation == 1)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR1;    //32 bit Analogue input event without time
		else if (Config::EventAnalogVariation == 2)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR2;    //16 bit Analogue input event without time
		else if (Config::EventAnalogVariation == 3)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR3;    //32 bit Analogue input event with time
		else if (Config::EventAnalogVariation == 4)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR4;    //16 bit Analogue input event with time
		else if (Config::EventAnalogVariation == 5)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR5;    //Float Analogue input event without time
		else if (Config::EventAnalogVariation == 6)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR6;    //Double Analogue input event without time
		else if (Config::EventAnalogVariation == 7)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR7;    //Float Analogue input event with time
		else if (Config::EventAnalogVariation == 8)
			stackConfig.slave.mEventAnalog=EAR_GROUP32_VAR8;    //Double Analogue input event with time
	break;
	}

	switch (Config::EventCoutnerGroup){
	case 22:
		if (Config::EventCounterVariation == 1)
			stackConfig.slave.mEventCounter=ECR_GROUP22_VAR1;   //32 bit binary counter event without time
		else if (Config::EventCounterVariation == 2)
			stackConfig.slave.mEventCounter=ECR_GROUP22_VAR2;   //16 bit binary counter event without time
		else if (Config::EventCounterVariation == 5)
			stackConfig.slave.mEventCounter=ECR_GROUP22_VAR5;   //32 bit binary counter event with time
		else if (Config::EventCounterVariation == 6)
			stackConfig.slave.mEventCounter=ECR_GROUP22_VAR6;   //16 bit binary counter event with time
	break;
	}

    switch (Config::StaticCounterGroup){
	case 20:
		if (Config::StaticCounterVariation == 1)
			stackConfig.slave.mStaticCounter=SCR_GROUP20_VAR1;  //32 bit binary counter
		else if (Config::StaticCounterVariation == 2)
			stackConfig.slave.mStaticCounter=SCR_GROUP20_VAR2;  //16 bit binary counter
		else if (Config::StaticCounterVariation == 5)
			stackConfig.slave.mStaticCounter=SCR_GROUP20_VAR5;  //32 bit binary counter without status
		else if (Config::StaticCounterVariation == 6)
			stackConfig.slave.mStaticCounter=SCR_GROUP20_VAR6;  //16 bit binary counter without status
	break;
	}

	switch (Config::StaticAnalogGroup){
	case 30:
		if (Config::StaticAnalogVariation == 1)
			stackConfig.slave.mStaticAnalog=SAR_GROUP30_VAR1;   //32 bit Analogue input
		else if (Config::StaticAnalogVariation == 2)
			stackConfig.slave.mStaticAnalog=SAR_GROUP30_VAR2;   //16 bit Analogue input
		else if (Config::StaticAnalogVariation == 3)
			stackConfig.slave.mStaticAnalog=SAR_GROUP30_VAR3;   //32 bit Analogue input without status
		else if (Config::StaticAnalogVariation == 4)
			stackConfig.slave.mStaticAnalog=SAR_GROUP30_VAR4;   //16 bit Analogue input without status
        else if (Config::StaticAnalogVariation == 5)
			stackConfig.slave.mStaticAnalog=SAR_GROUP30_VAR5;   //Float analogue input without status
		else if (Config::StaticAnalogVariation == 6)
			stackConfig.slave.mStaticAnalog=SAR_GROUP30_VAR6;   //Double Analogue input without status
	break;
	}

	switch (Config::StaticBinaryGroup){
	case 1:
		if (Config::StaticBinaryVariation == 2)
			stackConfig.slave.mStaticBinary=SBR_GROUP1_VAR2;    //Single bit binary input with status
	break;
	}

	switch (Config::StaticSetpointGroup){
	case 40:
		if (Config::StaticSetpointVariation == 1)
			stackConfig.slave.mStaticSetpointStatus=SSSR_GROUP40_VAR1;  //32 bit Analogue output status
		else if (Config::StaticSetpointVariation == 2)
			stackConfig.slave.mStaticSetpointStatus=SSSR_GROUP40_VAR2;  //16 bit Analogue output status
		else if (Config::StaticSetpointVariation == 3)
			stackConfig.slave.mStaticSetpointStatus=SSSR_GROUP40_VAR3;  //float
		else if (Config::StaticSetpointVariation == 4)
			stackConfig.slave.mStaticSetpointStatus=SSSR_GROUP40_VAR4;  //double
	break;
	}
//------------------------------------------------------------------------------------------------------------------------

	ClassMask classNum(true,true,true);
	stackConfig.slave.mUnsolMask=classNum;
	if(classNum.IsEnabled())
		std::cout <<"ClassMask Enable ................. "<< std::endl;

	// Create a new slave with a log level, command handler, and
	// config info this	returns a thread-safe interface used for
	//ICommandHandler* handlerIns=new MyCommandHandler();
	MyCommandHandler handlerIns;

	auto pOutstation = pServer->AddOutstation("outstation", LOG_LEVEL,  &handlerIns, stackConfig);

	// You can optionally add a listener to the stack to observer communicate health. You
	// can do this anytime and you will receive a stream of all state changes.
	pOutstation->AddStateListener([](StackState state) {
		std::cout << "outstation state: " << ConvertStackStateToString(state) << std::endl;
	});

	auto pDataObserver = pOutstation->GetDataObserver();
	handlerIns.GetpObserver(pDataObserver);
	//handlerIns.GetSock(sockRec);
    cout<<"Thread Start . . . ."<<endl;
    //---------------------------------------
    pthread_t ptid,ctid;

    pthread_create(&ptid,NULL,&Producer,(void *) &sockRec);
    pthread_create(&ctid,NULL,&Consumer,(void *) pDataObserver);
    cout<<"Thread Middle . . . ."<<endl;
    pthread_join(ptid,NULL);
    pthread_join(ctid,NULL);

   //----------------------------------------
    delete[] server_reply;
    return 0;
}

//---------------------------------------Thread Buffer Implement------------
void *Producer(void * sockR)
{
    int *sock = (int *)sockR;
    char *server_reply = new char[Config::BufferSize];
    for(;;)
    {
        struct sockaddr_in serverRec;
        cout<<"Thread in Producer . . . ."<<endl;
        int byte=recv(*sock , server_reply , Config::BufferSize , 0);
        if(  byte <= 0)
        {
            puts("recv failed");
            close(*sock);
            *sock = socket(AF_INET , SOCK_STREAM , 0);
    	    if (sockRec == -1)
                printf("Could not create socket");
    	    puts("Socket created");
    	    serverRec.sin_addr.s_addr = inet_addr(Config::JsonIP.c_str());
    	    serverRec.sin_family = AF_INET;
    	    serverRec.sin_port = htons( Config::JsonPort );
            while (connect(*sock , (struct sockaddr *)&serverRec , sizeof(serverRec)) < 0){

                perror("connect failed. Error");
                sleep(1);
            }

            puts("Connected\n");
        }
        else {
             server_reply[byte] = '\0';
             //printf("\nstrlen(server_reply) %d.\n",(int)strlen(server_reply));

             pthread_mutex_lock(&mVar);
             if(BufferIndex > Config::ThreadBufferSize)
             {
                 pthread_cond_wait(&Buffer_Not_Full,&mVar);
             }

            BufferIndex++;
            BUFFER.push_back(server_reply);
            //cout<<"Producer JSON Rec : "<<server_reply<<endl;
            pthread_mutex_unlock(&mVar);
            pthread_cond_signal(&Buffer_Not_Empty);
        }
    }
    delete[] server_reply;
}

void *Consumer(void * pDataObserver)
{
    string tempStr;
    char *server_reply = new char[Config::BufferSize];
    IDataObserver *mDataObserver= (IDataObserver *) pDataObserver;
    for(;;)
    {
        cout<<"Thread in Consumer . . . ."<<endl;
        if (pthread_mutex_trylock(&mVar) == 0){
        	if(BufferIndex<0)
            		pthread_cond_wait(&Buffer_Not_Empty,&mVar);

            tempStr=BUFFER[BufferIndex];
            strncpy (server_reply, BUFFER[BufferIndex].c_str(),tempStr.length());
            //printf("\nstrlen(server_reply) C %d.\n",(int)strlen(server_reply));
            server_reply[tempStr.length()]='\0';
            //cout<<"server_reply C : "<<server_reply<<endl;
            //cout<<"BUFFER C : "<<BUFFER[BufferIndex]<<endl;
            BUFFER.pop_back();
            //cout<<"Size : "<<BUFFER.size()<<endl;
            printf("Consume : %d \n",BufferIndex--);
            pthread_mutex_unlock(&mVar);
            pthread_cond_signal(&Buffer_Not_Full);

            Document document; // Default template parameter uses UTF8 and MemoryPoolAllocator.
            #if 0
            if (document.Parse(server_reply).HasParseError()){
                printf("\nPARSING ERROR.\n");
                return 1;
            }
            #else
// In-situ parsing, decode strings directly in the source string. Source must be string.
            {
                if (document.ParseInsitu(server_reply).HasParseError()){
                    printf("\nParsing Error.\n");
                    continue;
                }
            }
            #endif

            printf("\nParsing to document succeeded.\n");
//x={invokeID:1, request:{type:0, adresses:[{class:0, dataType:'analogInput', index:0},
//{class:1, dataType:'binaryOutput', index:2}], values:[2, 3]}}

//command 0 is update
//{"command":0, "detail":{"addresses":[{"group":23, "index":4}, {"group":12, "index":0}, {"group":12, "index":3}], "values":[{"kind":"int", "val":3}, {"kind":"pulse", "onTime":100, "offTime":100, "count":2}, {"kind":"latch_on"}]}}

            TimeTransaction tx(mDataObserver);
            Value::MemberIterator req = document.FindMember("detail");
            if(req != document.MemberEnd()){
                printf("\n Read JSON \n");
                const Value& a = document["detail"]["addresses"];
                SizeType num = a.Size();
                cout<< "num"<<num<<endl;
                for(SizeType j = 0; j < num; j++){
                    Value& v = document["detail"]["values"];
                    Value& ad = document["detail"]["addresses"];
                    SizeType valueNum = v.Size();
                    SizeType addNum = ad.Size();
                        if (valueNum > 0 && addNum > 0 && document["command"].GetInt() == 0){
                            cout<<"Get Type : "<<document["detail"]["values"][j]["val"].GetType()<<endl;
                                //-----------------Binary input -----------------------------------------------
                                if (document["detail"]["addresses"][j]["group"].GetInt() == 2){
                                    if(document["detail"]["values"][j]["val"].GetString() == string("CC_LATCH_ON")){
                                        cout<<"Enter Latch ON"<<endl;
                                        tx.Update(Binary(CC_LATCH_ON,BQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                        std::cout <<"iBinary = "<<"CC_LATCH_ON"<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                    }
                                    else if (document["detail"]["values"][j]["val"].GetString() == string("CC_LATCH_OFF")){
                                        tx.Update(Binary(CC_LATCH_OFF,BQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                        std::cout <<"iBinary = "<<"CC_LATCH_OFF"<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                    }
                                    else if (document["detail"]["values"][j]["val"].GetString() == string("CC_PULSE")){
                                        tx.Update(Binary(CC_PULSE,BQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                        std::cout <<"iBinary = "<<"CC_PULSE"<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                    }
                                    else if (document["detail"]["values"][j]["val"].GetString() == string("CC_PULSE_CLOSE")){
                                        tx.Update(Binary(CC_PULSE_CLOSE,BQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                        std::cout <<"iBinary = "<<"CC_PULSE_CLOSE"<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                    }
                                    else if (document["detail"]["values"][j]["val"].GetString() == string("CC_PULSE_TRIP")){
                                        tx.Update(Binary(CC_PULSE_TRIP,BQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                        std::cout <<"iBinary = "<<"CC_PULSE_TRIP"<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                    }
                                }
                                //---------------- Analog input -----------------------------------------------
                                else if (document["detail"]["addresses"][j]["group"].GetInt() == 32){

                                    if (document["detail"]["values"][j]["kind"].GetString() == string("int")){

                                        tx.Update(Analog(document["detail"]["values"][j]["val"].GetInt(),AQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                        std::cout <<"iAnalog = "<<document["detail"]["values"][j]["val"].GetInt()<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                    }
                                    else if (document["detail"]["values"][j]["kind"].GetString() == string("double")){
                                        tx.Update(Analog(document["detail"]["values"][j]["val"].GetDouble(),AQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                        std::cout <<"iAnalog = "<<document["detail"]["values"][j]["val"].GetDouble()<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                    }
                                }
                                //-----------------Counter --------------------------------------------------
                                else if (document["detail"]["addresses"][j]["group"].GetInt() == 22){
                                    tx.Update(Counter(document["detail"]["values"][j]["val"].GetInt(),CQ_ONLINE), document["detail"]["addresses"][j]["index"].GetInt());
                                    std::cout <<"iCounter = "<<document["detail"]["values"][j]["val"].GetInt()<<" at port "<<document["detail"]["addresses"][j]["index"].GetInt()<< std::endl;
                                }
                                //---------------------------------------------------------------------------
                        } //if num test and command 0
                        else if (document["command"].GetInt() == 8){
                                cout<<"--- Reading Commmand 8 ----"<<endl;
                                Document documentSend; // Default template parameter uses UTF8 and MemoryPoolAllocator.
                                documentSend.SetObject();
                                Document::AllocatorType& allocator = documentSend.GetAllocator();
                                Value addressArray(kArrayType);
                                Value myValueArray(kArrayType);

                                Value myAddressObject(kObjectType);
                                myAddressObject.SetObject();

                                Value myValueObject(kObjectType);
                                myValueObject.SetObject();

                                Value myDetailObject(kObjectType);
                                myDetailObject.SetObject();

                                myAddressObject.AddMember("Name", "IsAlive", allocator);
                                addressArray.PushBack(myAddressObject, allocator);

                                myValueObject.AddMember("val", true, allocator);
                                myValueArray.PushBack(myValueObject, allocator);

                                myDetailObject.AddMember("addresses", addressArray, allocator);
                                myDetailObject.AddMember("values", myValueArray, allocator);
                                documentSend.AddMember("command", 8, allocator);
                                documentSend.AddMember("detail", myDetailObject, allocator);

                                StringBuffer f;
                                Writer<StringBuffer> writer(f);
                                documentSend.Accept(writer);
                                //memcpy(json, f, sizeof(f));

                                string ss= f.GetString() ;
                                ss += "\n" ;
                                if( send(sockRec , ss.c_str() , strlen(ss.c_str()), 0) < 0)
                                    puts("Send failed");

                        }//if command 1 life signal
                }//for loop JSON
            }
        }//try_lock

		else{
		       sleep(2);
		       cout<<"BufferIndex <0"<<endl;
        	}

    }// for loop
    delete[] server_reply;
}
//--------------------------------------------------------------------------

//
