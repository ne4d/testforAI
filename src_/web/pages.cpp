#include "pages.h"

void addToMqttList(String header, String config){
  mqttTopicList += "<p><b>";
  mqttTopicList += header;
  mqttTopicList += "</b><p><br>";
  mqttTopicList += "<p>";
  mqttTopicList += config;
  mqttTopicList += "<p><br>";
}

String loginPage() {
  String entity_name = FSReadJsonString("entity_name");
  String dev_uid = FSReadJsonString("dev_uid");

  String login_page = "<html><head>";
  login_page += "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  login_page += "<title>Авторизация</title>";
  login_page += "<style>";
  login_page += "body { text-align: center; font-family: Arial, sans-serif; font-size: 20px;}";
  login_page += ".container { width: 250px; display: inline-block; text-align: left; padding: 10px; border: 2px solid #3498db; border-radius: 10px; }";
  login_page += "p, input { margin: 5px 0; }";
  login_page += "input[type='password'] { width: 250px; background-color: #FFFFFF; color: black; padding: 10px; border: 3px solid #3498db; border-radius: 5px; cursor: pointer; }";
  login_page += "input[type='submit'] { width: 250px; background-color: #3498db; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; }";
  login_page += "</style>";
  login_page += "</head><body>";
  login_page += "<center>";
  login_page += "<h3>" + entity_name + "(" + dev_uid + ")</h3>";
  login_page += "<form action='/login' method='POST'>";
  login_page += "Пароль:<br><input type='password' name='password'><br>";
  login_page += "<input type='submit' value='Войти'>";
  login_page += "</form>";
  login_page += "</center>";
  login_page += "</body></html>";

  return login_page;
}

String serverIndex() {
  String dev_uid = FSReadJsonString("dev_uid");
  String sw_ver = "FIRMWARE_VERSION";
  String ssid = FSReadJsonString("ssid");
  String ssid_pw = FSReadJsonString("ssid_pw");
  String mqtt_server = FSReadJsonString("mqtt_server");
  uint32_t mqtt_port = FSReadJsonUint64("mqtt_port");
  String mqtt_un = FSReadJsonString("mqtt_un");
  String mqtt_pw = FSReadJsonString("mqtt_pw");
  String web_pw = FSReadJsonString("web_pw");
  String entity_name = FSReadJsonString("entity_name");

  String server_index = "<html><head>";
  server_index += "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  server_index += "<style>";
  server_index += "body { text-align: center; font-family: Arial, sans-serif; font-size: 20px;}";
  server_index += ".container { width: 250px; display: inline-block; text-align: left; padding: 10px; border: 2px solid #3498db; border-radius: 10px; }";
  server_index += "p, input { margin: 5px 0; }";
  server_index += "input[type='password'], input[type='text'] { width: 250px; background-color: #FFFFFF; color: black; padding: 10px; border: 3px solid #3498db; border-radius: 5px; cursor: pointer; }";
  server_index += "input[type='submit'] { width: 100%; background-color: #3498db; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; }";
  server_index += "#btn_reset { background-color: red; }";
  server_index += "#btn_info { background-color: green; }";
  server_index += "#btn_reboot { background-color: orange; }";
  server_index += "#btn_on { background-color: green; width: 120px; display: inline-block; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; margin: 3px;}"; // Добавлен display: inline-block;
  server_index += "#btn_off { background-color: red; width: 120px; display: inline-block; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; margin: 3px;}"; // Добавлен display: inline-block;
  server_index += ".ts16 { font-size: 16px; }";
  server_index += ".button-container { display: flex; justify-content: center; gap: 10px; }"; // Новый стиль для контейнера кнопок;
  server_index += ".status-container { margin-top: 20px; padding: 10px; border: 1px solid #ccc; border-radius: 5px; }"; // Новый стиль для контейнера состояния;
  server_index += "progress { width: 100%; height: 20px; }";
  server_index += ".text-cnt { display: flex; justify-content: center; align-items: center;gap: 10px; }"; // Новый стиль для контейнера кнопок;

  // Стили для переключателей
  server_index += ".toggle-switch { position: relative; display: inline-block; width: 54px; height: 28px; margin: 5px; }";
  server_index += ".toggle-switch input { opacity: 0; width: 0; height: 0; }";
  server_index += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #a71414ff; transition: 0.4s; border-radius: 34px; }";
  server_index += ".slider:before { position: absolute; content: ''; height: 20px; width: 20px; left: 4px; bottom: 4px; background-color: white; transition: 0.4s; border-radius: 50%; }";
  server_index += "input:checked + .slider { background-color: #14a760ff; }"; // Primary color
  server_index += "input:checked + .slider:before { transform: translateX(26px); }";

  server_index += "</style>";

  server_index += "<script>";
  server_index += "function sendPost(url) {";
  server_index += "  fetch(url, { method: 'POST' })";
  server_index += "    .then(response => { if (response.ok) { console.log('Action successful'); } })"; // Обработка успеха (без перезагрузки);
  server_index += "    .catch(error => console.error('Error:', error));";
  server_index += "}";

  server_index += "function toggleSwitch(channel, isChecked) {";
  server_index += "  const url = isChecked ? '/ch' + channel + 'on' : '/ch' + channel + 'off';";
  server_index += "  sendPost(url);";
  server_index += "}";

  server_index += "document.addEventListener('DOMContentLoaded', function() {";
  server_index += "  const otaForm = document.querySelector('form[action=\"/update\"]');";
  server_index += "  if (otaForm) {";
  server_index += "    otaForm.addEventListener('submit', function(event) {";
  server_index += "      event.preventDefault();";
  server_index += "      const formData = new FormData(otaForm);";
  server_index += "      const xhr = new XMLHttpRequest();";
  server_index += "      xhr.open('POST', '/update', true);";
  server_index += "      xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');";
  server_index += "      xhr.upload.onprogress = function(e) {";
  server_index += "        if (e.lengthComputable) {";
  server_index += "          const percent = Math.round((e.loaded / e.total) * 100);";
  server_index += "          document.getElementById('progressBar').value = percent;";
  server_index += "          document.getElementById('progressPercent').textContent = percent + '%';";
  server_index += "        }";
  server_index += "      };";
  server_index += "      xhr.onloadstart = function() {";
  server_index += "        document.getElementById('ota-progress').style.display = 'block';";
  server_index += "        setTimeout(pollForReboot, 30000);"; // 30 секунд и перезагрузка;
  server_index += "      };";
  server_index += "      xhr.send(formData);";
  server_index += "    });";
  server_index += "  }";
  server_index += "});";

  server_index += "function pollForReboot() {";
  server_index += "  fetch('/', { credentials: 'include' })";
  server_index += "    .then(res => {";
  server_index += "      if (res.ok) {";
  server_index += "        location.reload();";
  server_index += "      } else {";
  server_index += "        setTimeout(pollForReboot, 1000);";
  server_index += "      }";
  server_index += "    })";
  server_index += "    .catch(() => setTimeout(pollForReboot, 1000));";
  server_index += "}";
  server_index += "</script>";

  server_index += "</head><body>";
  server_index += "<p><center><h3>(" + dev_uid + " v" + sw_ver + ")</h3></center></p>";
  server_index += "<p><center><h3>Network config</h3></center></p>";
  server_index += "<center><div class='container'>";
  server_index += "<p>Uptime: <span id='uptime'>" + String(minutesRunned()) + " min.</span></p>";
  server_index += "<br>";
  server_index += "<form action='/newwifi' method='POST'>";
  server_index += "ssid:<br><input type='text' name='wifiname' value='" + ssid + "'><br>";
  server_index += "ssid password:<br><input type='text' name='wifipwd' value='" + ssid_pw + "'><br>";
  server_index += "<input type='submit' value='Обновить настройки сети'>";
  server_index += "</form>";
  server_index += "<form action='/newmqtt' method='POST'>";
  server_index += "mqtt ip:<br><input type='text' name='mqttip' value='" + mqtt_server + "'><br>";
  server_index += "mqtt port:<br><input type='text' name='mqttport' value='" + String(mqtt_port) + "'><br>";
  server_index += "mqtt login:<br><input type='text' name='mqttlogin' value='" + mqtt_un + "'><br>";
  server_index += "mqtt password:<br><input type='text' name='mqttpassword' value='" + mqtt_pw + "'><br>";
  server_index += "<input type='submit' value='Обновить настройки mqtt'>";
  server_index += "</form>";
  
  server_index += "<form method='POST' action='/mqttInfo'>";
  server_index += "<input id='btn_info' input type='submit' value='Mqtt configuration info'>";
  server_index += "</form>";

  server_index += "<form action='/newwebpass' method='POST'>";
  server_index += "webUI password:<br><input type='text' name='webpass' value='" + web_pw + "'><br>";
  server_index += "<input type='submit' value='Обновить пароль WebUI'>";
  server_index += "</form>";
  server_index += "<form method='POST' action='/setDeviceCred'>";
  // server_index += "<label for='deviceName'>Device name:</label><br>";
  // server_index += "<input type='text' name='deviceName' value='" + dev_name + "'><br>";
  server_index += "<label for='entName'>Entity name:</label><br>";
  server_index += "<input type='text' name='entName' value='" + entity_name + "'><br>";
  server_index += "<input type='submit' value='Apply'>";
  server_index += "</form>";
  server_index += "<form action='/reboot' method='POST'>";
  server_index += "<input id='btn_reboot' type='submit' value='Reboot'>";
  server_index += "</form>";
  server_index += "</div></center>";
  server_index += "<br>";

  server_index += "<p><center><h3>Firmware</h3></center></p>";
  server_index += "<center><div class='container'>";
  server_index += "<center><p>OTA:</p>";
  server_index += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  server_index += "<input type='file' name='update'><br>";
  server_index += "<input type='submit' value='Update'>";
  server_index += "<div id='ota-progress' style='display:none;'>";
  server_index += "<progress id='progressBar' value='0' max='100'></progress>";
  server_index += "</div>";
  server_index += "</form>";

  server_index += "<form action='/hardreset' method='POST'>";
  server_index += "<input id='btn_reset' type='submit' value='Hard reset'>";
  server_index += "</form></div></center>";
  server_index += "<br>";

  server_index += "<p><center><h3>Device test</h3></center></p>";
  server_index += "<center><div class='container'>";

  // Переключатели для каналов CH1–CH4
  server_index += "<p><div class='text-cnt'>AC test (Off/Auto): <label class='toggle-switch'>";
  server_index += "<input type='checkbox' id='ch1_switch' onchange='toggleSwitch(1, this.checked)'>";
  server_index += "<span class='slider'></span></label></div></p>";

  server_index += "</div></center>";
  server_index += "</body></html>";

  return server_index;
}