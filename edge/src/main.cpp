#include "HardwareConfig.h"
#include <Arduino.h>
#include <Bounce2.h>
#include <Preferences.h>
#include <WiFiManager.h>

Preferences preferences;

// Preferencesの名前空間とキー名
const char *PREF_NAMESPACE = "colortimer";
const char *PREF_DEVICE_ID = "device_id";
const char *PREF_API_ENDPOINT = "api_endpt";
const char *PREF_API_KEY = "api_key";

// メモリ保持用変数 (初期値)
char deviceId[32] = "esp32-timer-default";
char apiEndpoint[128] =
    "https://********.execute-api.ap-northeast-1.amazonaws.com/prod/api/logs";
char apiKey[64] = "your_api_key_here";

bool shouldSaveConfig = false;

// 稼働ステート管理
enum TimerState { STATE_STANDBY, STATE_RUNNING };
TimerState currentState = STATE_STANDBY;

// ボタンのデバウンス用インスタンス
Bounce debouncer = Bounce();

// WiFiManagerの設定が保存された際に呼ばれるコールバック
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nStarting ColorTimer Next...");

  // ハードウェアピン等の初期化 (OLED, LED, Buzzer)
  initHardware();

  // 初期化リセット用ボタンの設定とBounce2へのアタッチ
  debouncer.attach(BUTTON_PIN, INPUT_PULLUP);
  debouncer.interval(25); // 25msのデバウンス

  // Preferencesの初期化 (読み書きモード)
  preferences.begin(PREF_NAMESPACE, false);

  // WiFiManagerのインスタンス生成
  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);

  // 起動時にボタン(GPIO33)がLOW(押されている状態)なら設定を初期化する
  debouncer.update();
  if (debouncer.read() == LOW) {
    displayStatus("RESETTING", "Clearing Settings");
    Serial.println("Reset button is pressed! Clearing all settings...");
    playBeep(500);
    preferences.clear();
    wm.resetSettings();
    displayStatus("CLEARED", "Please Reboot.");
    Serial.println(
        "Settings cleared. Please connect to 'ColorTimer-AP' to reconfigure.");
    while (true) {
      delay(100);
    } // 停止
  }

  // フラッシュメモリからの設定読み出し
  String savedDeviceId = preferences.getString(PREF_DEVICE_ID, deviceId);
  String savedApiEndpoint =
      preferences.getString(PREF_API_ENDPOINT, apiEndpoint);
  String savedApiKey = preferences.getString(PREF_API_KEY, apiKey);

  strlcpy(deviceId, savedDeviceId.c_str(), sizeof(deviceId));
  strlcpy(apiEndpoint, savedApiEndpoint.c_str(), sizeof(apiEndpoint));
  strlcpy(apiKey, savedApiKey.c_str(), sizeof(apiKey));

  // カスタムパラメータの設定
  WiFiManagerParameter custom_device_id("device_id", "Device ID", deviceId, 32);
  WiFiManagerParameter custom_api_endpoint("api_endpoint", "API Endpoint URL",
                                           apiEndpoint, 128);
  WiFiManagerParameter custom_api_key("api_key", "API Gateway Key", apiKey, 64);

  wm.addParameter(&custom_device_id);
  wm.addParameter(&custom_api_endpoint);
  wm.addParameter(&custom_api_key);

  displayStatus("Wi-Fi Setup", "Connecting...");

  // Wi-Fi接続の試行。接続情報がないか失敗した場合はColorTimer-APというアクセスポイントを起動する
  if (!wm.autoConnect("ColorTimer-AP", "password")) {
    displayError("Wi-Fi Failed");
    Serial.println("Failed to connect and hit timeout");
    playErrorSound();
    delay(3000);
    ESP.restart();
  }

  // 設定ポータルでSaveされた場合、新たな値をフラッシュメモリに保存
  if (shouldSaveConfig) {
    strlcpy(deviceId, custom_device_id.getValue(), sizeof(deviceId));
    strlcpy(apiEndpoint, custom_api_endpoint.getValue(), sizeof(apiEndpoint));
    strlcpy(apiKey, custom_api_key.getValue(), sizeof(apiKey));

    preferences.putString(PREF_DEVICE_ID, deviceId);
    preferences.putString(PREF_API_ENDPOINT, apiEndpoint);
    preferences.putString(PREF_API_KEY, apiKey);

    Serial.println("Configuration saved to Preferences.");
  }

  Serial.println("\nConnected to Wi-Fi!");
  displayStatus("Connected!");
  playBeep(100);
  delay(100);
  playBeep(100);

  // 初期ステート: Standby
  currentState = STATE_STANDBY;
  setLedState(LED_BLUE_BLINK);
  displayStatus("STANDBY", "Ready to Start");
}

void loop() {
  // LEDの点滅更新
  updateLedBlink();

  // ボタンの状態を更新
  debouncer.update();

  // ボタンが押された（立ち下がりエッジ）時の処理
  if (debouncer.fell()) {
    if (currentState == STATE_STANDBY) {
      // Standby -> Running
      currentState = STATE_RUNNING;
      setLedState(LED_RED);
      displayStatus("RUNNING", "Working...");
      playBeep(200); // ピッ
      Serial.println("[Action] Timer Started");
      // TODO: Issue 6でHTTP POST処理を追加 ('start')
    } else {
      // Running -> Standby
      currentState = STATE_STANDBY;
      setLedState(LED_BLUE_BLINK);
      displayStatus("STANDBY", "Task Stopped.");
      playBeep(100);
      delay(100);
      playBeep(100); // ピピッ
      Serial.println("[Action] Timer Stopped");
      // TODO: Issue 6でHTTP POST処理を追加 ('stop')
    }
  }

  delay(10);
}
