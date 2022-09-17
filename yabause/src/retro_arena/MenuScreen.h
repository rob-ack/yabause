/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <SDL.h>
#if defined(ARCH_IS_LINUX)
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>
#endif
#undef Success 
#include "nanogui/screen.h"
#include "nanovg.h"
#include <string.h>
#include <stack>

#if defined(_MSC_VER) 
#undef snprintf
#endif
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "EventManager.h"


#include "InputConfig.h"

using namespace nanogui;

class InputManager;
class Preference;

struct PreMenuInfo {
    Widget* window = nullptr;
    Widget* button = nullptr;
};

struct PlayerConfig {
    ComboBox * cb = nullptr;
    PopupButton *player = nullptr;
};

#include <iostream>
#include <list>
#include <unordered_map>

class LRU_Cache {
public:
  // キャッシュのサイズ
  const size_t cache_size;

  // コンストラクタ
  // size: キャッシュのサイズ
  LRU_Cache(size_t size) : cache_size(size) {}

  // キャッシュに値をセットする
  // key: キー
  // value: 値
  int set(const int& key, const int& value) {
    // キャッシュが満杯の場合、最も古いデータを削除する
    int last = -1;
    if (cache.size() >= cache_size) {
      last = cache[used.back()];
      cache.erase(used.back());
      used.pop_back();
    }

    // キャッシュに値をセットし、参照順序を更新する
    used.push_front(key);
    cache[key] = value;
    return last;
  }

  // キャッシュから値を取得する
  // key: キー
  // 戻り値: 値
  int get(const int& key) {
    // キャッシュに存在しない場合は、例外を発生させる
    if (cache.find(key) == cache.end()) {
      return -1;
    }

    // 参照順序を更新する
    used.remove(key);
    used.push_front(key);

    // 値を返す
    return cache[key];
  }

private:
  // キャッシュのデータ
  std::unordered_map<int, int> cache;

  // キャッシュのデータの参照順序
  std::list<int> used;
};



class MenuScreen : public nanogui::Screen
{
public:
    //Widget *activeWindow = nullptr;
    EventManager * evm = EventManager::getInstance();
    Widget *tools = nullptr;
    std::vector<PlayerConfig> player_configs_;
/*    
    PopupButton *player1;
    PopupButton *player1;
    ComboBox * p1cb = nullptr;
    PopupButton *player2 = nullptr;
    ComboBox * p2cb = nullptr;
*/    
    Button *bAnalog = nullptr;
    Button *bCdTray = nullptr;
    bool is_cdtray_open_ = false;

    std::map<SDL_JoystickID, SDL_Joystick*> joysticks_;

    int imageid_ = 0;
    int imageid_tmp_ = 0;
    int imgw_ = 0;
    int imgh_ = 0;
    NVGpaint imgPaint_ = {};     
    void setBackGroundImage( const std::string & fname );
    void setTmpBackGroundImage( const std::string & fname );
    void setDefalutBackGroundImage();

    nanogui::Window *window;
    nanogui::Window *swindow;
    nanogui::Window *imageWindow;
    nanogui::Window *dirSelectWindow;

    LRU_Cache * imageCache = new LRU_Cache(32);
    
    
    MenuScreen( SDL_Window* pwindow, int rwidth, int rheight, const std::string & fname, const std::string & game  );
    virtual bool keyboardEvent( const std::string & keycode , int scancode, int action, int modifiers);
    virtual void draw(NVGcontext *ctx);

    std::string current_game_path_;
    void setCurrentGamePath( const char * path ){ current_game_path_ = path; }

    uint32_t repeat_ = 0;
    void setRepeatEventCode(uint32_t code) { repeat_ = code; }

    bool sendRepeatEvent( const std::string & keycode, int scancode, int action, int modifiers);

    void showInputCheckDialog( const std::string & key );

    void showFileSelectDialog( Widget * parent, Widget * toback, const std::string & base_path );

    void setupPlayerPsuhButton( int user_index, PopupButton *player, const std::string & label, ComboBox **cb );

    int onRawInputEvent( InputManager & imp, const std::string & deviceguid, const std::string & type, int id, int value );

    void setCurrentInputDevices( std::map<SDL_JoystickID, SDL_Joystick*> & joysticks );

    std::stack<PreMenuInfo> menu_stack;
    void pushActiveMenu( Widget *active,  Widget * button );
    void popActiveMenu();
    Widget * getActiveMenu();

    void setConfigFile( const std::string & fname ){
        config_file_ = fname;
    }
    void getSelectedGUID( int user_index, std::string & selguid );

    void checkGameFiles(Widget * parent, const std::string & base_path);

    //void showSaveStateDialog( Widget * parent, Widget * toback );
    void setCurrentGameId( const std::string & id ){ cuurent_game_id_ = id; }
    void showSaveStateDialog( Popup *popup );
    void showLoadStateDialog( Popup *popup );
    void showConfigDialog( PopupButton *popup );

    void setupBiosMenu(PopupButton *parent, std::shared_ptr<Preference> preference);

public:  // events
    int onBackButtonPressed();    
    int onShow();    

protected:
    std::string cuurent_deviceguid_;
    std::string current_user_;    
    std::string current_key_; 
    std::string config_file_;
    std::string cuurent_game_id_;

    GameInfoManager * gameInfoManager;

    vector<shared_ptr<GameInfo>> games;

    std::atomic<bool> bFileSearchCancled;

    void refreshGameListAsync(const vector<string> & base_path_array);

};
