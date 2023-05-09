#include "usb_protoc.hpp"
#include <nan.h>

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;

int currenthandle = -1;

// receiverawdata(buffer, isCommand, (status) => {});
void sendrawdata(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    if (currenthandle < 0)
    {
        Nan::ThrowError("No handle available");
        return;
    }
    if (!args[0]->IsUint8Array() && !args[0]->IsArrayBuffer())
    {
        Nan::ThrowError("Argument 1: is not an IsUint8Array or ArrayBuffer");
        return;
    }
    if (!args[1]->IsBoolean())
    {
        Nan::ThrowError("Argument 2: is not a boolean");
        return;
    }
    if (!args[2]->IsFunction())
    {
        Nan::ThrowError("Argument 3: is not a function (cb)");
        return;
    }
    unsigned char *buffer = (unsigned char *)node::Buffer::Data(args[0]);
    int stat = sendRawData(currenthandle, buffer, args[0].As<v8::Uint8Array>()->Length(), args[1]->IsTrue());

    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = {Nan::New(stat)};

    Nan::Callback cb(args[2].As<v8::Function>());
    Nan::AsyncResource resource("gt5-usb:sendrawdata-cb");
    cb.Call(argc, argv, &resource);
}

// receiverawdata(length, (status, buffer) => {});
void receiverawdata(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    if (currenthandle < 0)
    {
        Nan::ThrowError("No handle available");
        return;
    }
    if (!args[0]->IsNumber() || args[0].IsEmpty())
    {
        Nan::ThrowError("Argument must be a number");
        return;
    }
    
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    
    unsigned int size = (unsigned int)args[0]->NumberValue(context).FromJust();
    unsigned char *retval = new unsigned char[size];
    int stat = receiveRawData(currenthandle, retval, size);

    const unsigned argc = 2;
    v8::Local<v8::Value> argv[argc] = {Nan::New(stat), Nan::NewBuffer((char *)retval, size).ToLocalChecked()};

    Nan::Callback cb(args[1].As<v8::Function>());
    Nan::AsyncResource resource("gt5-usb:receiverawdata-cb");
    cb.Call(argc, argv, &resource);
}

void runcommand(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    if (currenthandle < 0)
    {
        Nan::ThrowError("No handle available");
        return;
    }
    if (!args[0]->IsUint32() || args[0].IsEmpty())
    {
        Nan::ThrowError("Argument 1: is not an unsigned integer");
        return;
    }
    if (!args[1]->IsInt32() || args[1].IsEmpty())
    {
        Nan::ThrowError("Argument 2: is not an 32-bit integer");
        return;
    }

    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();

    int cmd_id = (unsigned short)args[0]->NumberValue(context).FromJust();
    int parameter = (int)args[1]->NumberValue(context).FromJust();
    CmdPacket result;
    int stat = runCommand(currenthandle, cmd_id, parameter, result);

    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = {Nan::New(stat)};

    Nan::Callback cb(args[2].As<v8::Function>());
    Nan::AsyncResource resource("gt5-usb:runcommand-cb");
    cb.Call(argc, argv, &resource);
}

// initdevice(path, (handle) =>{});
void initdevice(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
    std::string val;

    if (!args[0]->IsString())
    {
        Nan::ThrowError("Argument 1: is not a string");
        return;
    }

    Nan::Utf8String tval(args[0]);
    val = *tval;

    if (!args[1]->IsFunction() || args[1].IsEmpty())
    {
        Nan::ThrowError("Argument 2: is not a function (cb)");
        return;
    }

    currenthandle = gethandle(val.c_str());
    if (currenthandle < 0)
    {
        Nan::ThrowError("Couldn't open device");
        return;
    }

    Nan::Callback cb(args[1].As<v8::Function>());
    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = {Nan::New(currenthandle)};
    Nan::AsyncResource resource("gt5-usb:initdevice-cb");
    cb.Call(argc, argv, &resource);
}

void Init(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, void*)
{
    Nan::SetMethod(exports, "initdevice", initdevice);
    Nan::SetMethod(exports, "receiverawdata", receiverawdata);
    Nan::SetMethod(exports, "sendrawdata", sendrawdata);
    Nan::SetMethod(exports, "RunCommand", runcommand);
}

NODE_MODULE(gt_usb, Init)