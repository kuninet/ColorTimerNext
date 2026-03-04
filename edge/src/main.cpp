#include <Arduino.h>
#include <WiFiManager.h>
#include <Preferences.h>

Preferences preferences;

// Preferencesの名前空間とキー名
const char* PREF_NAMESPACE = "colortimer";
const char* PREF_DEVICE_ID = "device_id";
const char* PREF_API_ENDPOINT = "api_endpt";
const char* PREF_API_KEY = "api_key";

// メモリ保持用変数 (初期値)
char deviceId[32] = "esp32-timer-default";
char apiEndpoint[128] = "https://********.execute-api.ap-northeast-1.amazonaws.com/prod/api/logs";
char apiKey[64] = "your_api_key_here";

bool shouldSaveConfig = false;

// WiFiManagerの設定が保存された際に呼ばれるコールバック
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nStarting ColorTimer Next...");

  // Preferencesの初期化 (読み書きモード)
  preferences.begin(PREF_NAMESPACE, false);
  
  // フラッシュメモリからの設定読み出し (存在しなければ第2引数のデフォルト値が使われる)
  String savedDeviceId = preferences.getString(PREF_DEVICE_ID, deviceId);
  String savedApiEndpoint = preferences.getString(PREF_API_ENDPOINT, apiEndpoint);
  String savedApiKey = preferences.getString(PREF_API_KEY, apiKey);

  // Stringオブジェクトからchar配列へのコピー
  strlcpy(deviceId, savedDeviceId.c_str(), sizeof(deviceId));
  strlcpy(apiEndpoint, savedApiEndpoint.c_str(), sizeof(apiEndpoint));
  strlcpy(apiKey, savedApiKey.c_str(), sizeof(apiKey));

  // WiFiManagerのインスタンス生成
  WiFiManager wm;
  
  // デバッグ用: 設定を消去したい場合はコメントアウトを外す
  // wm.resetSettings();

  wm.setSaveConfigCallback(saveConfigCallback);

  // カスタムパラメータの設定
  WiFiManagerParameter custom_device_id("device_id", "Device ID", deviceId, 32);
  WiFiManagerParameter custom_api_endpoint("api_endpoint", "API Endpoint URL", apiEndpoint, 128);
  WiFiManagerParameter custom_api_key("api_key", "API Gateway Key", apiKey, 64);

  wm.addParameter(&custom_device_id);
  wm.addParameter(&custom_api_endpoint);
  wm.addParameter(&custom_api_key);

  // Wi-Fi接続の試行。接続情報がないか失敗した場合はColorTimer-APというアクセスポイントを起動する
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
