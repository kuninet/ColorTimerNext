#include <Arduino.h>
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

// WiFiManagerの設定が保存された際に呼ばれるコールバック
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// ... (後続のIssue
// 5で定義予定ですが、今回はリセット用にボタンのピン定義を先行追加します)
const int BUTTON_PIN = 33;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nStarting ColorTimer Next...");

  // 初期化リセット用ボタンの設定
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Preferencesの初期化 (読み書きモード)
  preferences.begin(PREF_NAMESPACE, false);

  // WiFiManagerのインスタンス生成
  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);

  // 起動時にボタン(GPIO33)がLOW(押されている状態)なら設定を初期化する
  // 誤動作防止のため、少し待ってから再度判定する
  delay(100);
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Reset button is pressed! Clearing all settings...");
    // Preferences (フラッシュメモリの独自保存データ) をクリア
    preferences.clear();
    // WiFiManager (Wi-Fi資格情報) をクリア
    wm.resetSettings();
    Serial.println(
        "Settings cleared. Please connect to 'ColorTimer-AP' to reconfigure.");
  }

  // フラッシュメモリからの設定読み出し (存在しなければデフォルト値)
  String savedDeviceId = preferences.getString(PREF_DEVICE_ID, deviceId);
  String savedApiEndpoint =
      preferences.getString(PREF_API_ENDPOINT, apiEndpoint);
  String savedApiKey = preferences.getString(PREF_API_KEY, apiKey);

  // Stringオブジェクトからchar配列へのコピー
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

  // Wi-Fi接続の試行。接続情報がないか失敗した場合は ColorTimer-AP
  // というアクセスポイントを起動する
  if (!wm.autoConnect("ColorTimer-AP", "password")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    // リセットして再試行
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

  Serial.println("");
  Serial.println("Connected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("Device ID: %s\n", deviceId);
}

void loop() {
  // メインループ(ボタン検知、LED制御、HTTPリクエスト等)はIssue5、 Issue6で実装
  delay(100);
}
