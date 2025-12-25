#include "esp_http_server.h"
#include "cJSON.h"
#include "app_system.h"
#include "esp_log.h"
#include <stdio.h> 
#include <stdlib.h>

static const char *TAG = "WEB";

const char *html_page = 
"<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Control FAN</title>"
"<style>"
":root { --primary: #007AFF; --bg: #F2F2F7; --card: #FFFFFF; --text: #1C1C1E; --success: #34C759; --danger: #FF3B30; --warn: #FF9500; }"
"body { font-family: sans-serif; background-color: var(--bg); color: var(--text); margin: 0; padding: 20px; display: flex; justify-content: center; }"
".container { width: 100%; max-width: 400px; display: flex; flex-direction: column; gap: 20px; }"
".card { background: var(--card); border-radius: 20px; padding: 20px; box-shadow: 0 4px 12px rgba(0,0,0,0.05); }"
"h1 { font-size: 22px; text-align: center; margin: 0 0 15px 0; }"
".sensor-box { display: flex; justify-content: space-around; text-align: center; margin-bottom: 10px; }"
".sensor-val { font-size: 26px; font-weight: 700; color: var(--primary); }"
".sensor-label { font-size: 13px; opacity: 0.6; }"
".row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }"
".switch { position: relative; display: inline-block; width: 50px; height: 28px; }"
".switch input { opacity: 0; width: 0; height: 0; }"
".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }"
".slider:before { position: absolute; content: ''; height: 24px; width: 24px; left: 2px; bottom: 2px; background-color: white; transition: .4s; border-radius: 50%; }"
"input:checked + .slider { background-color: var(--success); }"
"input:checked + .slider:before { transform: translateX(22px); }"
".speed-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-top: 10px; }"
".btn { border: none; padding: 12px; border-radius: 12px; font-weight: 600; cursor: pointer; transition: 0.2s; background: #E5E5EA; color: var(--text); }"
".btn.active { background: var(--primary); color: white; }"
".btn-off { grid-column: span 3; background: #FFE5E5; color: var(--danger); }"
".input-group { display: flex; gap: 10px; margin-bottom: 10px; align-items: center; font-size: 14px; }"
"input[type=number] { width: 60px; padding: 8px; border-radius: 8px; border: 1px solid #ccc; text-align: center; }"
".btn-save { width: 100%; background: var(--primary); color: white; padding: 10px; border: none; border-radius: 10px; font-weight: 600; cursor: pointer; }"
".timer-display { text-align: center; font-size: 13px; color: var(--warn); font-weight: 600; margin-top: 8px; height: 18px; }"
"</style></head><body>"

"<div class='container'>"
"  <div class='card'>"
"    <h1>Control FAN by A2D</h1>"
"    <div class='sensor-box'>"
"      <div><div class='sensor-val'><span id='t'>--</span>&deg;C</div><div class='sensor-label'>Temperature</div></div>"
"      <div><div class='sensor-val'><span id='h'>--</span>%</div><div class='sensor-label'>Humidity</div></div>"
"    </div>"
"  </div>"

"  <div class='card'>"
"    <div class='row'>"
"      <strong>MODE: <span id='mode_txt'>MANUAL</span></strong>"
"      <label class='switch'><input type='checkbox' id='mode_sw' onclick='togMode()'><span class='slider'></span></label>"
"    </div>"
"    <hr style='border:0; border-top:1px solid #eee; margin:10px 0;'>"
"    <div class='speed-grid'>"
"      <button class='btn' id='btn1' onclick='spd(1)'>LOW</button>"
"      <button class='btn' id='btn2' onclick='spd(2)'>MID</button>"
"      <button class='btn' id='btn3' onclick='spd(3)'>HIGH</button>"
"      <button class='btn btn-off' onclick='spd(0)'>POWER OFF</button>"
"    </div>"
"  </div>"

"  <div class='card'>"
"    <strong>MODE AUTO Settings</strong><br><br>"
"    <div class='input-group'><label>Level 1 (&gt;):</label><input type='number' id='th1' step='0.5'>&deg;C</div>"
"    <div class='input-group'><label>Level 2 (&gt;):</label><input type='number' id='th2' step='0.5'>&deg;C</div>"
"    <div class='input-group'><label>Level 3 (&gt;):</label><input type='number' id='th3' step='0.5'>&deg;C</div>"
"    <button class='btn-save' onclick='saveThresh()'>Save Config</button>"
"  </div>"

"  <div class='card'>"
"    <strong>Timer Countdown POWER OFF</strong><br><br>"
"    <div class='input-group'>"
"      <input type='number' id='timer_val' placeholder='Mins' style='flex:1'>"
"      <button class='btn-save' style='width:auto; background:var(--warn)' onclick='setTimer()'>Start</button>"
"    </div>"
"    <div class='timer-display' id='timer_status'></div>"
"  </div>"
"</div>"

"<script>"
"function req(url){ fetch(url + '&t=' + Date.now()); setTimeout(upd, 200); }"
"function spd(v){ req('/ctrl?cmd=speed&val='+v); }"
"function togMode(){ req('/ctrl?cmd=mode'); }"
"function saveThresh(){"
"  let t1 = document.getElementById('th1').value;"
"  let t2 = document.getElementById('th2').value;"
"  let t3 = document.getElementById('th3').value;"
"  req('/ctrl?cmd=thresh&t1='+t1+'&t2='+t2+'&t3='+t3);"
"  alert('Saved!');"
"}"
"function setTimer(){"
"  let m = document.getElementById('timer_val').value;"
"  req('/ctrl?cmd=timer&val='+m);"
"}"
"function upd(){"
"  fetch('/status?t=' + Date.now()).then(r=>r.json()).then(d=>{"
"    if(d.t) document.getElementById('t').innerText=d.t.toFixed(1);"
"    if(d.h) document.getElementById('h').innerText=d.h.toFixed(1);"
"    document.getElementById('mode_sw').checked=d.auto;"
"    document.getElementById('mode_txt').innerText=d.auto?'AUTO':'MANUAL';"
"    [1,2,3].forEach(i=>document.getElementById('btn'+i).className= (d.spd==i && !d.auto)?'btn active':'btn');"
"    if(document.activeElement.tagName!='INPUT') {"
"       document.getElementById('th1').value=d.th1;"
"       document.getElementById('th2').value=d.th2;"
"       document.getElementById('th3').value=d.th3;"
"    }"
"    let ts = document.getElementById('timer_status');"
"    if(d.tm > 0) ts.innerText = 'Off in ' + d.tm + ' mins';"
"    else ts.innerText = '';"
"  }).catch(e => console.log('Connect error'));"
"}"
"setInterval(upd, 1000);"
"upd();"
"</script></body></html>";

static esp_err_t root_get(httpd_req_t *req) {
    return httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_get(httpd_req_t *req) {
    char resp[256];
    xSemaphoreTake(xMutexState, portMAX_DELAY);
    sprintf(resp, "{\"t\":%.1f,\"h\":%.1f,\"spd\":%d,\"auto\":%d,\"th1\":%.1f,\"th2\":%.1f,\"th3\":%.1f,\"tm\":%d}", 
            g_state.current_temp, g_state.current_hum, g_state.speed_level, g_state.is_auto,
            g_state.thresh_1, g_state.thresh_2, g_state.thresh_3, g_state.timer_minutes);
    xSemaphoreGive(xMutexState);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t control_get(httpd_req_t *req) {
    char buf[200];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(buf, "cmd", param, sizeof(param)) == ESP_OK) {
            app_command_t cmd;

            if (strcmp(param, "speed") == 0) {
                if (httpd_query_key_value(buf, "val", param, sizeof(param)) == ESP_OK) {
                    cmd.type = CMD_WEB_SET_SPEED;
                    cmd.value = atoi(param);
                    xQueueSend(xQueueCmd, &cmd, 0);
                }
            }
            else if (strcmp(param, "mode") == 0) { 
                cmd.type = CMD_WEB_SET_MODE;
                xQueueSend(xQueueCmd, &cmd, 0);
            }
            else if (strcmp(param, "timer") == 0) {
                if (httpd_query_key_value(buf, "val", param, sizeof(param)) == ESP_OK) {
                    cmd.type = CMD_WEB_SET_TIMER;
                    cmd.value = atoi(param);
                    xQueueSend(xQueueCmd, &cmd, 0);
                }
            }
            else if (strcmp(param, "thresh") == 0) {
                char val_buf[10];
                cmd.type = CMD_WEB_SET_THRESH;
                if(httpd_query_key_value(buf, "t1", val_buf, sizeof(val_buf)) == ESP_OK) {
                    cmd.value = 1; cmd.f_value = atof(val_buf); xQueueSend(xQueueCmd, &cmd, 0);
                }
                if(httpd_query_key_value(buf, "t2", val_buf, sizeof(val_buf)) == ESP_OK) {
                    cmd.value = 2; cmd.f_value = atof(val_buf); xQueueSend(xQueueCmd, &cmd, 0);
                }
                if(httpd_query_key_value(buf, "t3", val_buf, sizeof(val_buf)) == ESP_OK) {
                    cmd.value = 3; cmd.f_value = atof(val_buf); xQueueSend(xQueueCmd, &cmd, 0);
                }
            }
        }
    }
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

void app_webserver_init(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = root_get };
        httpd_uri_t uri_status = { .uri = "/status", .method = HTTP_GET, .handler = status_get };
        httpd_uri_t uri_ctrl = { .uri = "/ctrl", .method = HTTP_GET, .handler = control_get };
        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_status);
        httpd_register_uri_handler(server, &uri_ctrl);
        ESP_LOGI(TAG, "Web Server Started on port 80");
    }
}