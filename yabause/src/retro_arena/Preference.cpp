#include "Preference.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "common.h"
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#define LOG printf
using std::wstring;

#include <Windows.h>

std::string ConvertWideStringToMultiByteString(const std::string& utf8_str)
{
  if (utf8_str.empty()) {
    return std::string();
  }

  int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), NULL, 0);
  std::wstring wstrFrom(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), &wstrFrom[0], size_needed);

  size_needed = WideCharToMultiByte(CP_ACP, 0, wstrFrom.c_str(), (int)wstrFrom.size(), NULL, 0, NULL, NULL);
  std::string sjis_str(size_needed, 0);
  WideCharToMultiByte(CP_ACP, 0, wstrFrom.c_str(), (int)wstrFrom.size(), &sjis_str[0], size_needed, NULL, NULL);

  return sjis_str;
}

std::string ConvertMultiByteStringToWideString(const std::string& sjis_str)
{
  if (sjis_str.empty()) {
    return std::string();
  }

  int size_needed = MultiByteToWideChar(CP_OEMCP, 0, sjis_str.c_str(), (int)sjis_str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_OEMCP, 0, sjis_str.c_str(), (int)sjis_str.size(), &wstrTo[0], size_needed);

  size_needed = WideCharToMultiByte(CP_UTF8, 0, wstrTo.c_str(), (int)wstrTo.size(), NULL, 0, NULL, NULL);
  std::string utf8_str(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstrTo.c_str(), (int)wstrTo.size(), &utf8_str[0], size_needed, NULL, NULL);

  return utf8_str;
}


Preference::Preference( const std::string & filename){

  std::string input_trace_filename;

  int index = filename.find_last_of("/\\");
  if( index != string::npos ){
    input_trace_filename = filename.substr(index+1);
  }else{
    input_trace_filename = filename;
  }

  std::string home_dir;
  getHomeDir(home_dir);

  this->filename = home_dir + input_trace_filename + ".config";

  LOG("Preference: opening %s\n", this->filename.c_str() );

  if (!fs::exists(this->filename)) {
    if (filename == "default") {
      json defaults;
      defaults["frame skip"] = false;
      defaults["Aspect rate"] = 0;
      defaults["Resolution"] = 0;
      defaults["External cart"] = 7;
      defaults["Rotate screen"] = false;
      defaults["Rotate screen resolution"] = 0;
      defaults["Use compute shader"] = true;
      defaults["speed"] = 0;
      defaults["sound sync mode"] = "cpu";
      defaults["sound sync count per a frame"] = 4;
      defaults["bios file"] = home_dir + "bios.bin";
      defaults["last play game path"] = "";
      defaults["Full screen"] = true;
      defaults["Show Fps"] = false;
      defaults["target display"] = 0;
      std::vector<string> gamedirs;
      gamedirs.push_back(home_dir + "games");
      std::vector<string> warray;
      for (int i = 0; i < gamedirs.size(); i++) {
        warray.push_back(ConvertMultiByteStringToWideString(gamedirs[i]));
      }
      json j_array(warray);
      defaults["game directories"] = j_array;
      std::ofstream out(this->filename);
      out << defaults.dump(2);
      out.close();
    }
    else {
      Preference default("default");
      std::ofstream out(this->filename);
      out << default.j.dump(2);
      out.close();
    }
  }

  try{
    std::ifstream fin( this->filename );
    fin >> j;
    fin.close();
  }catch ( json::exception& e ){
  }
} 

Preference::~Preference(){
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();  
}   



int Preference::getInt( const std::string & key , int defaultv = 0  ){
  try {
    LOG("Preference: getInt %s:%d\n", key.c_str() ,j[key].get<int>() );
    return j[key].get<int>();
  }catch ( json::type_error & e ){
  }

  LOG("Preference: fail to getInt %s\n", key.c_str());
  return defaultv;
}

bool Preference::getBool( const std::string & key , bool defaultv = false ){
  try {
    LOG("Preference: getBool %s:%d\n", key.c_str(),j[key].get<bool>() );
    return j[key].get<bool>();
  }catch ( json::type_error & e ){
  }

  LOG("Preference: fail to getBool %s\n", key.c_str());
  return defaultv;
}

void Preference::setInt( const std::string & key , int value ){
  j[key] = value;
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();  
}

void Preference::setBool( const std::string & key , bool value ){
  j[key] = value;
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();     
}


std::vector<string> Preference::getStringArray(const std::string & key) {
  try {
    LOG("Preference: getStringArray %s:%d\n", key.c_str(), j[key].get<std::vector<string>>());
    auto warray = j[key].get<std::vector<string>>();
    std::vector<string> array;

    for (int i = 0; i < warray.size(); i++) {
      array.push_back(ConvertWideStringToMultiByteString(warray[i]));
    }

    return array;
  }
  catch (json::type_error & e) {
  }

  LOG("Preference: fail to getStringArray %s\n", key.c_str());
  std::vector<string> empty;
  return empty;
}



void Preference::setStringArray(const string & key, const std::vector<string> array) {

  std::vector<string> warray;
  for (int i = 0; i < array.size(); i++) {
    warray.push_back(ConvertMultiByteStringToWideString(array[i]));
  }

  json j_array(warray);
  j[key] = j_array;
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();
}

string Preference::getString(const string & key, const string & defaultv) {
  try {
    LOG("Preference: getString %s:%d\n", key.c_str(), j[key].get<string>());
    return ConvertWideStringToMultiByteString(j[key].get<string>());
  }
  catch (json::type_error & e) {
  }
  LOG("Preference: fail to getString %s\n", key.c_str());
  return defaultv;
}

void Preference::setString(const string & key, const string & value) {
  j[key] = ConvertMultiByteStringToWideString(value);
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();
}

