#include "Preference.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "common.h"
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#define LOG printf

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
  home_dir += "/.yabasanshiro/";

  this->filename = home_dir + input_trace_filename + ".config";

  LOG("Preference: opening %s\n", this->filename.c_str() );

  if (!fs::exists(this->filename)) {
    if (filename == "") {
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
      defaults["bios file"] = "";
      defaults["last play game path"] = "";
      std::vector<string> gamedirs;
      gamedirs.push_back(home_dir);
      json j_array(gamedirs);
      defaults["game directories"] = j_array;
      std::ofstream out(this->filename);
      out << defaults.dump(2);
      out.close();
    }
    else {
      Preference default("");
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
    return j[key].get<std::vector<string>>();
  }
  catch (json::type_error & e) {
  }

  LOG("Preference: fail to getStringArray %s\n", key.c_str());
  std::vector<string> empty;
  return empty;
}


void Preference::setStringArray(const string & key, const std::vector<string> array) {
  json j_array(array);
  j[key] = j_array;
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();
}

string Preference::getString(const string & key, const string & defaultv) {
  try {
    LOG("Preference: getString %s:%d\n", key.c_str(), j[key].get<string>());
    return j[key].get<string>();
  }
  catch (json::type_error & e) {
  }
  LOG("Preference: fail to getString %s\n", key.c_str());
  return defaultv;
}

void Preference::setString(const string & key, const string & value) {
  j[key] = value;
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();
}

