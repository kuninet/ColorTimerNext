#include "HardwareConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Bounce2.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

Preferences preferences;

// Preferencesの名前空間とキー名
const char *PREF_NAMESPACE = "colortimer";
const char *PREF_DEVICE_ID = "device_id";
const char *PREF_API_ENDPOINT = "api_endpt";
const char *PREF_API_KEY = "api_key";
const char *PREF_TIMEOUT_MIN = "timeout_min";

// メモリ保持用変数 (初期値)
char deviceId[32] = "esp32-timer-default";
char apiEndpoint[128] =
    "https://********.execute-api.ap-northeast-1.amazonaws.com/prod/api/logs";
char apiKey[64] = "your_api_key_here";
char timeoutMin[8] = "30"; // デフォルト30分

bool shouldSaveConfig = false;

// 稼働ステート管理
enum TimerState { STATE_STANDBY, STATE_RUNNING, STATE_TIMEOUT };
TimerState currentState = STATE_STANDBY;

// 長押し機能やタイムアウト計算用
unsigned long stateStartTime = 0;

// ボタンのデバウンス用インスタンス
Bounce debouncer = Bounce();

// WiFiManagerの設定が保存された際に呼ばれるコールバック
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// API通信関数 (戻り値: 成功なら true)
bool sendLogToApi(const char *action) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: WiFi not connected");
    return false;
  }

  // Set up HTTPS connection (Insecure client to avoid managing specific root CA
  // certificates for broad AWS API Gateway domains, per basic IoT usage)
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client->setInsecure();
  } else {
    Serial.println("Failed to create secure client");
    return false;
  }

  HTTPClient https;

  Serial.print("Connecting to API: ");
  Serial.println(apiEndpoint);

  if (https.begin(*client, apiEndpoint)) {
    // APIヘッダーの追加
    https.addHeader("Content-Type", "application/json");
    https.addHeader("x-api-key", apiKey);

    // JSONペイロードの生成
    JsonDocument doc;
    doc["device_id"] = deviceId;
    doc["action"] = action;
    String requestBody;
    serializeJson(doc, requestBody);

    Serial.print("Sending POST request: ");
    Serial.println(requestBody);

    // POST送信
    int httpCode = https.POST(requestBody);

    bool success = false;
    if (httpCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
        // String payload = https.getString();
        // Serial.println(payload);
        success = true;
      }
    } else {
      Serial.printf("HTTP Request failed, error: %s\n",
                    https.errorToString(httpCode).c_str());
    }

    https.end();
    delete client;
    return success;
  } else {
    Serial.println("Unable to connect to HTTPS endpoint");
    delete client;
    return false;
  }
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
  String savedTimeoutMin = preferences.getString(PREF_TIMEOUT_MIN, timeoutMin);

  strlcpy(deviceId, savedDeviceId.c_str(), sizeof(deviceId));
  strlcpy(apiEndpoint, savedApiEndpoint.c_str(), sizeof(apiEndpoint));
  strlcpy(apiKey, savedApiKey.c_str(), sizeof(apiKey));
  strlcpy(timeoutMin, savedTimeoutMin.c_str(), sizeof(timeoutMin));

  // カスタムパラメータの設定
  WiFiManagerParameter custom_device_id("device_id", "Device ID", deviceId, 32);
  WiFiManagerParameter custom_api_endpoint("api_endpoint", "API Endpoint URL",
                                           apiEndpoint, 128);
  WiFiManagerParameter custom_api_key("api_key", "API Gateway Key", apiKey, 64);
  WiFiManagerParameter custom_timeout_min("timeout_min", "Timeout (minutes)",
                                          timeoutMin, 8);

  wm.addParameter(&custom_device_id);
  wm.addParameter(&custom_api_endpoint);
  wm.addParameter(&custom_api_key);
  wm.addParameter(&custom_timeout_min);

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
    strlcpy(timeoutMin, custom_timeout_min.getValue(), sizeof(timeoutMin));

    preferences.putString(PREF_DEVICE_ID, deviceId);
    preferences.putString(PREF_API_ENDPOINT, apiEndpoint);
    preferences.putString(PREF_API_KEY, apiKey);
    preferences.putString(PREF_TIMEOUT_MIN, timeoutMin);

    Serial.println("Configuration saved to Preferences.");
  }

  Serial.println("\nConnected to Wi-Fi!");
  displayStatus("Connected!");
  playBeep(100);
  delay(100);
  playBeep(100);

  // 初期ステート: Standby
  currentState = STATE_STANDBY;
  setLedState(LED_RED); // 待機＝赤点灯
  displayStatus("STANDBY", "Ready to Start");
  displayStatus("STANDBY", "Ready to Start");
}

void loop() {
  // LEDの点滅更新
  updateLedBlink();

  // 5秒ごとに現在のステータスをシリアルへ出力
  static unsigned long lastStatusLog = 0;
  if (millis() - lastStatusLog >= 5000) {
    lastStatusLog = millis();
    if (currentState == STATE_RUNNING) {
      Serial.println("[STATUS] 状態: 作業中 (RUNNING) | LED: 青点灯");
    } else if (currentState == STATE_TIMEOUT) {
      Serial.println(
          "[STATUS] 状態: タイムアウト (TIMEOUT) | LED: 青点滅 & アラーム");
    } else {
      Serial.println("[STATUS] 状態: 休み中 (STANDBY) | LED: 赤点灯");
    }
  }

  // タイムアウト検知処理
  if (currentState == STATE_RUNNING) {
    long timeoutMs = atol(timeoutMin) * 60 * 1000;
    if (millis() - stateStartTime >= timeoutMs) {
      currentState = STATE_TIMEOUT;
      setLedState(LED_BLUE_BLINK); // タイムアウト＝青点滅
      displayStatus("TIMEOUT", "Time's up!");
      Serial.println("[Action] Timer Timeout Reached");
    }
  }

  // タイムアウト状態ならカラータイマー音を非同期で鳴らす
  if (currentState == STATE_TIMEOUT) {
    playColorTimerSound();
  }

  // ボタンの状態を更新
  debouncer.update();

  static unsigned long buttonPressStartTime = 0;
  static bool beep3sDone = false;
  static bool beep5sDone = false;

  // ボタンが押された（立ち下がりエッジ）時の処理
  if (debouncer.fell()) {
    buttonPressStartTime = millis();
    beep3sDone = false;
    beep5sDone = false;
  }

  // ボタンが押されている間の処理（長押し検知・ビープ音鳴動）
  if (!debouncer.read()) { // !HIGH (LOW)
                           // の時、ボタンが押されていると仮定(INPUT_PULLUP)
    if (buttonPressStartTime > 0) {
      unsigned long pressedDuration = millis() - buttonPressStartTime;
      // 3秒経過
      if (pressedDuration >= 3000 && pressedDuration < 5000 && !beep3sDone) {
        playBeep(100); // 3秒ピッ
        displayStatus("Hold: 3s", "Release for AP");
        beep3sDone = true;
      }
      // 5秒経過
      else if (pressedDuration >= 5000 && !beep5sDone) {
        playBeep(100);
        delay(100);
        playBeep(100); // 5秒ピピッ
        displayStatus("Hold: 5s", "Release to Reset");
        beep5sDone = true;
      }
    }
  }

  // ボタンが離された（立ち上がりエッジ）時の処理
  if (debouncer.rose()) {
    unsigned long pressedDuration = millis() - buttonPressStartTime;
    buttonPressStartTime = 0;

    if (pressedDuration >= 5000) {
      // 5秒以上長押し -> 完全初期化
      Serial.println("[Action] Reset to default settings");
      displayStatus("RESETTING...", "Clearing WiFi");
      WiFiManager wm;
      wm.resetSettings();
      delay(1000);
      ESP.restart(); // 再起動して初期状態へ
    } else if (pressedDuration >= 3000) {
      // 3秒〜5秒長押し -> 設定ポータル起動（オンデマンドAP）
      Serial.println("[Action] Starting Config Portal...");
      displayStatus("CONFIG MODE", "Connect to AP");
      setLedState(LED_BLUE_BLINK); // 設定モード中は青点滅

      WiFiManager wm;

      // 保存時に shouldSaveConfig が呼ばれるようにセット
      wm.setSaveConfigCallback(saveConfigCallback);
      shouldSaveConfig = false; // 初期化しておく

      // Saveボタン押下後、WiFi接続試行の完了を待たずに強制的にポータルを抜ける
      wm.setBreakAfterConfig(true);

      // カスタムパラメータを再セットしてAP開始
      WiFiManagerParameter custom_device_id("device_id", "Device ID", deviceId,
                                            32);
      WiFiManagerParameter custom_api_endpoint(
          "api_endpoint", "API Endpoint URL", apiEndpoint, 128);
      WiFiManagerParameter custom_api_key("api_key", "API Gateway Key", apiKey,
                                          64);
      WiFiManagerParameter custom_timeout_min(
          "timeout_min", "Timeout (minutes)", timeoutMin, 8);
      wm.addParameter(&custom_device_id);
      wm.addParameter(&custom_api_endpoint);
      wm.addParameter(&custom_api_key);
      wm.addParameter(&custom_timeout_min);

      // ポータル開始 (Break設定により必ずfalseで戻るか抜ける)
      wm.startConfigPortal("ColorTimer-AP");

      // 保存ボタンが押され、ポータルを抜けた場合はここに来る
      if (shouldSaveConfig) {
        strlcpy(deviceId, custom_device_id.getValue(), sizeof(deviceId));
        strlcpy(apiEndpoint, custom_api_endpoint.getValue(),
                sizeof(apiEndpoint));
        strlcpy(apiKey, custom_api_key.getValue(), sizeof(apiKey));
        strlcpy(timeoutMin, custom_timeout_min.getValue(), sizeof(timeoutMin));

        // グローバルのpreferencesは既にbegin()されているのでそのまま書き込む
        preferences.putString(PREF_DEVICE_ID, deviceId);
        preferences.putString(PREF_API_ENDPOINT, apiEndpoint);
        preferences.putString(PREF_API_KEY, apiKey);
        preferences.putString(PREF_TIMEOUT_MIN, timeoutMin);
        shouldSaveConfig = false;

        Serial.println("[Action] Updated settings, restarting...");
        displayStatus("SAVED", "Restarting...");
      } else {
        Serial.println(
            "[Action] Config portal closed without saving, restarting...");
        displayStatus("CANCELED", "Restarting...");
      }
      delay(1000);
      ESP.restart();
    } else {
      // --- 短押し (3秒未満) の場合は本来のAPI通信処理 ---
      if (currentState == STATE_STANDBY) {
        // Standby -> Running (Start API Request)
        displayStatus("Sending...", "Start Timer");
        if (sendLogToApi("start")) {
          currentState = STATE_RUNNING;
          stateStartTime = millis(); // 開始時間を記録
          setLedState(LED_BLUE);     // 作業中＝青点灯
          displayStatus("RUNNING", "Working...");
          playBeep(200); // 成功音ピッ
          Serial.println("[Action] Timer Started");
        } else {
          displayError("API Send Failed");
          playErrorSound();
          delay(2000);
          displayStatus("STANDBY", "Ready to Start");
        }

      } else if (currentState == STATE_RUNNING ||
                 currentState == STATE_TIMEOUT) {
        // Running/Timeout -> Standby (Stop API Request)
        displayStatus("Sending...", "Stop Timer");
        if (sendLogToApi("stop")) {
          currentState = STATE_STANDBY;
          setLedState(LED_RED); // 待機＝赤点灯に戻る
          displayStatus("STANDBY", "Task Stopped.");
          playBeep(100);
          delay(100);
          playBeep(100); // 終了音ピピッ
          Serial.println("[Action] Timer Stopped");
        } else {
          displayError("API Send Failed");
          playErrorSound();
          delay(2000);
          displayStatus("RUNNING", "Working...");
        }
      }
    }
  }

  delay(10);
}
