#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include <nan.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "rlink.h"
#include "helper.h"

using namespace v8;
using namespace std;

Nan::Persistent<FunctionTemplate> RWrap::constructor_template;

RWrap::RWrap() : q_(NULL) {
  q_ = new RInside();
  callbacks_ = new RWrapCallbacks(this);
  q_->set_callbacks(callbacks_);
}

RWrap::~RWrap() {
  delete q_;
  delete callbacks_;
}

void RWrapCallbacks::WriteConsole(const std::string& line, int type)
{
  Handle<Object> global = Nan::GetCurrentContext()->Global();
  Local<Value> args[1] = { String::NewFromUtf8(v8::Isolate::GetCurrent(), line.c_str() ) };
  Local<Function> cb = Nan::New(_parent->cb_WriteConsole);
  Nan::MakeCallback(global, cb, 1, args);
}

void RWrapCallbacks::ShowMessage(const char* message)
{
  Handle<Object> global = Nan::GetCurrentContext()->Global();
  Local<Value> args[1] = { String::NewFromUtf8(v8::Isolate::GetCurrent(), message ) };
  Local<Function> cb = Nan::New(_parent->cb_ShowMessage);
  Nan::MakeCallback(global, cb, 1, args);
}

void RWrapCallbacks::Suicide(const char* message)
{
  Handle<Object> global = Nan::GetCurrentContext()->Global();
  Local<Value> args[1] = { String::NewFromUtf8(v8::Isolate::GetCurrent(), message ) };
  Local<Function> cb = Nan::New(_parent->cb_Suicide);
  Nan::MakeCallback(global, cb, 1, args);
}

NAN_MODULE_INIT(RWrap::Initialize) {
  Nan::HandleScope scope;

   // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("session").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "setCallbackShowMessage", setCallbackShowMessage);
  Nan::SetPrototypeMethod(tpl, "setCallbackSuicide", setCallbackSuicide);
  Nan::SetPrototypeMethod(tpl, "setCallbackWriteConsole", setCallbackWriteConsole);
  Nan::SetPrototypeMethod(tpl, "parseEval", parseEval);
  Nan::SetPrototypeMethod(tpl, "parseEvalQ", parseEvalQ);
  Nan::SetPrototypeMethod(tpl, "parseEvalQNT", parseEvalQNT);
  Nan::SetPrototypeMethod(tpl, "assign", assign);
  Nan::SetPrototypeMethod(tpl, "get", get);

  constructor_template.Reset(tpl);
  Nan::Set(target, Nan::New("session").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}


void RWrap::setCallbackShowMessage(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.This());

  Local<Function> callbackHandle = Local<Function>::Cast(info[0]);
  r->cb_ShowMessage.Reset(callbackHandle);

  info.GetReturnValue().SetUndefined();
}


void RWrap::setCallbackSuicide(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.This());

  Local<Function> callbackHandle = Local<Function>::Cast(info[0]);
  r->cb_Suicide.Reset(callbackHandle);

  info.GetReturnValue().SetUndefined();
}


void RWrap::setCallbackWriteConsole(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.This());

  Local<Function> callbackHandle = Local<Function>::Cast(info[0]);
  r->cb_WriteConsole.Reset(callbackHandle);

  info.GetReturnValue().SetUndefined();
}


void RWrap::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* w = new RWrap();

  RInside* q = w->GetWrapped();
  std::string load_command = "library(jsonlite, quietly=TRUE);";
  q->parseEvalQ(load_command);

  w->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}


void RWrap::parseEval(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.This());
  RInside* q = r->GetWrapped();

  v8::String::Utf8Value param(info[0]->ToString());
  std::string command = std::string(*param);
  std::string wrapper_before = "toJSON(";
  std::string wrapper_after = ", force=TRUE);";
  std::string full_command = wrapper_before + command + wrapper_after;

  Handle<Object> global = Nan::GetCurrentContext()->Global();
  Handle<Object> JSON = Handle<Object>::Cast(Nan::Get(global, Nan::New("JSON").ToLocalChecked()).ToLocalChecked());
  Handle<Function> parse = Handle<Function>::Cast(
	  Nan::Get(JSON, Nan::New("parse").ToLocalChecked()).ToLocalChecked()
  );

  try {
    std::string ret = q->parseEval(full_command);
    Handle<Value> ret_V8 = Nan::New( ret.c_str() ).ToLocalChecked();
    Handle<Value> result = Handle<String>::Cast(parse->Call(JSON, 1, &ret_V8));
    info.GetReturnValue().Set(result);
  } catch(std::exception& ex) {
	  std::string errorMessage(ex.what());
    Nan::ThrowTypeError( errorMessage.c_str() );
    info.GetReturnValue().Set(Nan::Undefined());
  } catch(...) {
    Nan::ThrowTypeError("Unknown error encountered");
    info.GetReturnValue().Set(Nan::Undefined());
  }
}

void RWrap::parseEvalQ(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.This());
  RInside* q = r->GetWrapped();

  v8::String::Utf8Value param(info[0]->ToString());
  std::string command = std::string(*param);

  try {
    q->parseEvalQ(command);
  }
  catch(std::exception& ex) {
	  std::string errorMessage(ex.what());
    Nan::ThrowTypeError( errorMessage.c_str() );
  } catch(...) {
    Nan::ThrowTypeError("Unknown error encountered");
  }

  info.GetReturnValue().Set(Nan::Undefined());
}

void RWrap::parseEvalQNT(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.This());
  RInside* q = r->GetWrapped();

  v8::String::Utf8Value param(info[0]->ToString());
  std::string command = std::string(*param);

  q->parseEvalQNT(command);

  info.GetReturnValue().Set(Nan::Undefined());
}

void RWrap::assign(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.This());
  RInside* q = r->GetWrapped();
  v8::String::Utf8Value param1(info[0]->ToString());
  std::string name = std::string(*param1);

  if ( info[1]->IsNumber() ) {
    double value = info[1]->NumberValue();
    q->assign(value, name);
  }
  else if ( info[1]->IsString() ) {
    v8::String::Utf8Value value(info[1]->ToString());
    std::string value_str = std::string(*value);
    q->assign(value_str, name);
  }
  else if ( info[1]->IsArray() || info[1]->IsObject() ) {
    Handle<Object> object = Handle<Object>::Cast(info[1]);

    Handle<Object> global = Nan::GetCurrentContext()->Global();
    Handle<Object> JSON = Handle<Object>::Cast(Nan::Get(global, Nan::New("JSON").ToLocalChecked()).ToLocalChecked());
    Handle<Function> stringify = Handle<Function>::Cast(
    Nan::Get(JSON, Nan::New("stringify").ToLocalChecked()).ToLocalChecked());

    Handle<Value> stringifyable[] = { object };
    Handle<String> result = Handle<String>::Cast(stringify->Call(JSON, 1, stringifyable));
    v8::String::Utf8Value value(result);
    std::string value_str = std::string(*value);
    q->assign(value_str, "JSON_container");

    std::string command_pt2 = " = fromJSON(JSON_container); rm(JSON_container);";
    std::string full_command = name + command_pt2;
    q->parseEvalQ(full_command);
  }
  info.GetReturnValue().Set(Nan::Undefined());
}

void RWrap::get(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RWrap* r = ObjectWrap::Unwrap<RWrap>(info.Holder());
  RInside* q = r->GetWrapped();

  v8::String::Utf8Value param(info[0]->ToString());
  std::string name = std::string(*param);
  std::string command_pt1 = "toJSON(";
  std::string command_pt2 = ", force=TRUE);";
  std::string full_command = command_pt1 + name + command_pt2;

  Handle<Object> global = Nan::GetCurrentContext()->Global();
  Handle<Object> JSON = Handle<Object>::Cast(Nan::Get(global, Nan::New("JSON").ToLocalChecked()).ToLocalChecked());
  Handle<Function> parse = Handle<Function>::Cast(
  Nan::Get(JSON, Nan::New("parse").ToLocalChecked()).ToLocalChecked());

  try{
    std::string ret = q->parseEval(full_command);
    Handle<Value> ret_V8 = Nan::New( ret.c_str() ).ToLocalChecked();
    Handle<Value> result = Handle<String>::Cast(parse->Call(JSON, 1, &ret_V8));
    info.GetReturnValue().Set(result);
  } catch(...) {
    Nan::ThrowTypeError("The requested variable could not be retrieved.");
    info.GetReturnValue().Set(Nan::Undefined());
  }
}
